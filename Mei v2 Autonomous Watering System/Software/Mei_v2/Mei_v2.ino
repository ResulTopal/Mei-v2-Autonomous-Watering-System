#include <Wire.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <DHT.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ThingSpeak.h> 

// 1. TELEGRAM AND NETWORK SETTINGS
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN_HERE" 
#define CHAT_ID_1 "YOUR_CHAT1_ID_HERE"
#define CHAT_ID_2 "YOUR_CHAT2_ID_HERE"

WiFiClientSecure secured_client; 
WiFiClient client;            
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// --- THINGSPEAK SETTINGS ---
unsigned long myChannelNumber = 0000000; 
const char * myWriteAPIKey = "YOUR_THINGSPEAK_API_KEY"; 

unsigned long lastTimeSentToCloud = 0;
const unsigned long postingInterval = 15L * 60L * 1000L; 

unsigned long lastTimeBotRan;
int botRequestDelay = 2000; 
bool emptyTankMsgSent = false;
bool wifiErrorIconVisible = false;

// 2. HARDWARE AND PIN CONFIGURATIONS
#define TFT_SCLK  14 
#define TFT_MOSI  13  
#define TFT_RST   12
#define TFT_DC    11
#define TFT_CS    10 
const int blkPin = 9; 

#define DHTPIN          4
#define DHTTYPE         DHT22
#define FLOAT_SWITCH_PIN 46  

#define MOISTURE_PIN_PURPLE 5 
#define PUMP_PIN_PURPLE     1  
#define MOISTURE_PIN_WHITE  6 
#define PUMP_PIN_WHITE      40  

// 3. SYSTEM PARAMETERS
int AirValue = 4095; 
int WaterValue = 1000; 
int WATERING_DURATION = 2000;   
long COOLDOWN = 400050;     

int targetHumidity = 33;        
int screenBrightness = 80;

unsigned long actionTimerPurple = 0;
bool isWateringPurple = false;
unsigned long actionTimerWhite = 0;
bool isWateringWhite = false;

unsigned long prevM = 0;
int purplePumpCount = 0;
int whitePumpCount = 0;

// Custom Color Definitions
#define COLOR_PURPLE 0x781F 
#define COLOR_WHITE  0xFFFF
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_YELLOW 0xFFE0

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHTPIN, DHTTYPE);

// 4. CUSTOM EMOJI AND DRAWING FUNCTIONS
void drawRose(int x, int y) {
  tft.fillCircle(x, y, 4, ST77XX_RED);
  tft.fillCircle(x-4, y-4, 4, ST77XX_RED);      
  tft.fillCircle(x+4, y-4, 4, ST77XX_RED);      
  tft.fillCircle(x-4, y+4, 4, ST77XX_RED);      
  tft.fillCircle(x+4, y+4, 4, ST77XX_RED);      
  tft.fillCircle(x, y, 2, 0xF81F);
  tft.drawLine(x, y+6, x, y+14, COLOR_GREEN);    
}

void drawSmileyFace(int x, int y) {
  tft.fillCircle(x, y, 8, COLOR_YELLOW);           
  tft.drawPixel(x-3, y-2, ST77XX_BLACK);
  tft.drawPixel(x-2, y-2, ST77XX_BLACK);
  tft.drawPixel(x+3, y-2, ST77XX_BLACK);        
  tft.drawPixel(x+2, y-2, ST77XX_BLACK);

  tft.drawPixel(x-4, y+2, ST77XX_BLACK);
  tft.drawPixel(x+4, y+2, ST77XX_BLACK);
  tft.drawPixel(x-2, y+3, ST77XX_BLACK);
  tft.drawPixel(x+2, y+3, ST77XX_BLACK);
  tft.drawPixel(x, y+4, ST77XX_BLACK); 
  
  tft.drawLine(x-4, y+2, x-2, y+3, ST77XX_BLACK);
  tft.drawLine(x-2, y+3, x+2, y+3, ST77XX_BLACK);
  tft.drawLine(x+2, y+3, x+4, y+2, ST77XX_BLACK);
}

void drawExclamation(int x, int y) {
  tft.fillCircle(x, y, 8, ST77XX_RED);
  tft.fillRect(x-1, y-5, 3, 6, ST77XX_WHITE);   
  tft.fillRect(x-1, y+3, 3, 3, ST77XX_WHITE);
}

void drawNoWiFi(int x, int y) {
  tft.drawCircle(x, y + 2, 9, ST77XX_WHITE); 
  tft.drawCircle(x, y + 2, 6, ST77XX_WHITE);
  tft.drawCircle(x, y + 2, 3, ST77XX_WHITE); 
  tft.fillRect(x - 10, y + 4, 21, 12, ST77XX_BLACK);
  tft.fillCircle(x, y + 10, 2, ST77XX_WHITE); 
  tft.drawLine(x - 7, y - 3, x + 7, y + 13, ST77XX_RED);
  tft.drawLine(x - 6, y - 3, x + 8, y + 13, ST77XX_RED);
}

// 5. WIFI SETUP AND TELEGRAM FUNCTIONS
void configModeCallback (WiFiManager *myWiFiManager) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(42, 5); 
  tft.println("WIFI KURULUMU");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 28);
  tft.println("1. Telefon WiFi ac");
  tft.setCursor(0, 48);
  tft.println("2. Su Aga Baglan:");
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(18, 68); 
  tft.println("> Al Sana Cicke <");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 88);
  tft.println("3. WiFi Config Tikla"); 
  tft.setCursor(0, 108);
  tft.println("4. Ev Wifine Baglan");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    
    // Security check for unauthorized access
    if (chat_id != CHAT_ID_1 && chat_id != CHAT_ID_2) {
      bot.sendMessage(chat_id, "Yetkisiz erişim denemesi tespit edildi.", "");
      continue;
    }
    
    String text = bot.messages[i].text;
    
    if (text == "/help" || text == "/start") {
      String helpMessage = "✮⋆˙  Mei v2 Sulama Sistemi  ˙⋆✮\n\n";
      helpMessage += "Kullanabileceğin Komutlar:\n\n";
      helpMessage += "📊 /status - Sensörleri ve depo durumunu gösterir.\n\n";
      helpMessage += "💧 /nem - Hedef sulama nemini ayarlar.\n    (Örn: /nem 40)\n\n";
      helpMessage += "💡 /light - Ekran parlaklığını ayarlar.\n    (Örn: /light 50)\n\n";
      helpMessage += "⏳ /time - Sulama süresini ayarlar (Saniye).\n    (Örn: /time 3)\n\n";
      helpMessage += "⏱️ /wait - Bekleme süresini ayarlar (Dakika).\n    (Örn: /wait 10)\n\n";
      helpMessage += "❓ /help - Bu komut listesini gösterir.";
      bot.sendMessage(chat_id, helpMessage, "");
    }
    
    else if (text.startsWith("/light ")) {
      String valueStr = text.substring(7);
      int newBrightness = valueStr.toInt();    
      
      if (newBrightness >= 0 && newBrightness <= 100) {
        screenBrightness = newBrightness;
        int pwmValue = map(screenBrightness, 0, 100, 0, 255); 
        analogWrite(blkPin, pwmValue);
        bot.sendMessage(chat_id, "💡 Ekran parlaklığı başarıyla %" + String(screenBrightness) + " olarak ayarlandı!", "");
      } else {
        bot.sendMessage(chat_id, "⚠️ Lütfen 0 ile 100 arasında bir parlaklık değeri girin.\nÖrnek kullanım: /light 80", "");
      }
    }
    
    else if (text.startsWith("/nem ")) {
      String valueStr = text.substring(5);
      int newValue = valueStr.toInt();    
      
      if (newValue >= 20 && newValue <= 75) {
        targetHumidity = newValue;
        tft.fillRect(174, 5, 42, 16, ST77XX_BLACK); 
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_YELLOW);            
        tft.setCursor(175, 5);                      
        tft.print("%");
        tft.print(targetHumidity);
        bot.sendMessage(chat_id, "✅ Hedef nem başarıyla %" + String(targetHumidity) + " olarak ayarlandı!", "");
      } else {
        bot.sendMessage(chat_id, "⚠️ Lütfen 20 ile 75 arasında bir değer girin.\nÖrnek kullanım: /nem 45", "");
      }
    }
    
    else if (text == "/status") {
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      int moisturePurple = constrain(map(analogRead(MOISTURE_PIN_PURPLE), AirValue, WaterValue, 0, 100), 0, 100);
      int moistureWhite = constrain(map(analogRead(MOISTURE_PIN_WHITE), AirValue, WaterValue, 0, 100), 0, 100);
      
      String statusMsg = "🌱 ORKİDE DURUM RAPORU 🌱\n\n";
      statusMsg += "🌡️ Sıcaklık: " + String(t, 1) + " °C\n";
      statusMsg += "💧 Hava Nemi: " + String(h, 0) + " %\n\n";
      statusMsg += "💡 Ekran Işığı: %" + String(screenBrightness) + "\n";
      statusMsg += "🎯 Hedef Nem: " + String(targetHumidity) + " %\n";
      statusMsg += "⏱️ Sulama: " + String(WATERING_DURATION / 1000) + " sn\n";
      statusMsg += "⏳ Bekleme: " + String(COOLDOWN / 60000) + " dk\n\n";
      statusMsg += "💜 Mor Orkide: " + String(moisturePurple) + " %\n";
      statusMsg += "🤍 Beyaz Orkide: " + String(moistureWhite) + " %\n\n";
      
      if(digitalRead(FLOAT_SWITCH_PIN) == HIGH) {
         statusMsg += "⚠️ SU DEPOSU BOŞ ⚠️";
      } else {
         statusMsg += "✅ Su Deposu: Dolu";
      }
      bot.sendMessage(chat_id, statusMsg, "");
    }

    else if (text.startsWith("/time ")) {
      String valueStr = text.substring(6);
      int newDuration = valueStr.toInt();    
      
      // Güvenlik: 1-10 saniye arası
      if (newDuration >= 1 && newDuration <= 10) {
        WATERING_DURATION = newDuration * 1000;
        bot.sendMessage(chat_id, "✅ Motor çalışma süresi başarıyla " + String(newDuration) + " saniye olarak ayarlandı!", "");
      } else {
        bot.sendMessage(chat_id, "⚠️ Lütfen 1 ile 10 saniye arasında güvenli bir değer girin.\nÖrnek: /time 3", "");
      }
    }

    else if (text.startsWith("/wait ")) {
      String valueStr = text.substring(6);
      int newCooldown = valueStr.toInt();    
      
      // Güvenlik: 1-60 dakika arası
      if (newCooldown >= 1 && newCooldown <= 60) {
        COOLDOWN = newCooldown * 60L * 1000L;
        bot.sendMessage(chat_id, "✅ Sulamalar arası bekleme süresi başarıyla " + String(newCooldown) + " dakika olarak ayarlandı!", "");
      } else {
        bot.sendMessage(chat_id, "⚠️ Lütfen 1 ile 60 dakika arasında bir değer girin.\nÖrnek: /wait 10", "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(blkPin, OUTPUT);
  analogWrite(blkPin, map(screenBrightness, 0, 100, 0, 255));
  
  pinMode(PUMP_PIN_PURPLE, OUTPUT);
  pinMode(PUMP_PIN_WHITE, OUTPUT);
  pinMode(FLOAT_SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(PUMP_PIN_PURPLE, LOW); 
  digitalWrite(PUMP_PIN_WHITE, LOW); 

  tft.init(135, 240);
  tft.setRotation(1); 
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(4); 
  tft.setCursor(72, 25); 
  tft.println("HOLA");
  tft.setTextSize(3);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(75, 70); 
  tft.println("ROSE"); 
  delay(3500); 

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_BLUE);
  tft.setCursor(84, 40); 
  tft.println("WiFi");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(36, 80); 
  tft.println("Baglaniliyor..");

  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  if (!wm.autoConnect("Al Sana Cicke")) { ESP.restart(); }

  secured_client.setInsecure();
  dht.begin();
  ThingSpeak.begin(client); // BULUT BAĞLANTISINI BAŞLAT
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(39, 40); 
  tft.println("Baglandi!");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(12, 85); 
  tft.println("Sistem Basliyor...");
  
  String startupMsg = "Otonom Sulama Sistemi Başlatıldı.\nKomutları görmek için /help yazabilirsin. 🌼";
  bot.sendMessage(CHAT_ID_1, startupMsg, "");
  if(String(CHAT_ID_2) != "" && String(CHAT_ID_2) != "0") {
     bot.sendMessage(CHAT_ID_2, startupMsg, "");
  }
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawLine(0, 25, 240, 25, ST77XX_WHITE);
  tft.drawLine(0, 95, 240, 95, ST77XX_WHITE);
  drawRose(225, 10); 

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);            
  tft.setCursor(175, 5);                      
  tft.print("%");
  tft.print(targetHumidity);
  
  lastTimeSentToCloud = millis() - postingInterval + 15000;
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    if (!wifiErrorIconVisible) { 
      drawNoWiFi(145, 7);
      wifiErrorIconVisible = true;
    }
  } else {
    if (wifiErrorIconVisible) { 
      tft.fillRect(135, 0, 25, 24, ST77XX_BLACK);
      wifiErrorIconVisible = false;
    }
  }

  unsigned long now = millis();
  
  // Handle Incoming Telegram Messages
  if (now - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      yield(); 
      delay(100);
    }
    lastTimeBotRan = millis(); 
  }

  // Sensor Reading and Display Update (Every 2 Seconds)
  if (now - prevM >= 2000) {
    prevM = now;
    
    // DHT Calibration adjustment
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    int moisturePurple = constrain(map(analogRead(MOISTURE_PIN_PURPLE), AirValue, WaterValue, 0, 100), 0, 100);
    int moistureWhite = constrain(map(analogRead(MOISTURE_PIN_WHITE), AirValue, WaterValue, 0, 100), 0, 100);
    bool isTankEmpty = (digitalRead(FLOAT_SWITCH_PIN) == HIGH); 

    tft.setTextSize(2);
    
    tft.setTextColor(COLOR_YELLOW, ST77XX_BLACK);
    tft.setCursor(0, 5);
    tft.print(t, 1); tft.print("C "); 
    tft.setTextColor(COLOR_BLUE, ST77XX_BLACK);
    tft.print(h, 0); tft.print(" % ");

    tft.setTextColor(COLOR_PURPLE, ST77XX_BLACK);
    tft.setCursor(0, 35);
    tft.print("MOR: ");
    if(moisturePurple < 100) tft.print(" ");
    if(moisturePurple < 10) tft.print(" ");
    tft.print(moisturePurple);
    tft.print(" %");
    
    tft.setCursor(125, 35);
    if(isTankEmpty) {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK); 
      tft.print("DEPO BOS ");
    } else {
      tft.setTextColor(COLOR_PURPLE, ST77XX_BLACK);   
      if(isWateringPurple) tft.print("SULANIYOR");
      else if (moisturePurple < targetHumidity) tft.print("BEKLIYOR ");
      else tft.print("HAZIR    ");
    }

    tft.setTextColor(COLOR_WHITE, ST77XX_BLACK);
    tft.setCursor(0, 65);
    tft.print("BYZ: ");
    if(moistureWhite < 100) tft.print(" ");
    if(moistureWhite < 10) tft.print(" ");
    tft.print(moistureWhite);
    tft.print(" %");
    
    tft.setCursor(125, 65);
    if(isTankEmpty) {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK); 
      tft.print("DEPO BOS ");
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
      if(isWateringWhite) tft.print("SULANIYOR");
      else if (moistureWhite < targetHumidity) tft.print("BEKLIYOR ");
      else tft.print("HAZIR    ");
    }

    tft.setCursor(0, 105);
    if(isTankEmpty) {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
      tft.print("!! DEPO TUKENDI !!");
      
      drawExclamation(225, 113);

      digitalWrite(PUMP_PIN_PURPLE, LOW);
      digitalWrite(PUMP_PIN_WHITE, LOW);
      isWateringPurple = false;
      isWateringWhite = false;
      
      if(!emptyTankMsgSent) {
         bot.sendMessage(CHAT_ID_1, "🚨 KRİTİK UYARI: Su deposu boşaldı, sistem durduruldu.", "");
         if(String(CHAT_ID_2) != "" && String(CHAT_ID_2) != "0") {
            bot.sendMessage(CHAT_ID_2, "🚨 KRİTİK UYARI: Su deposu boşaldı, sistem durduruldu.", "");
         }
         emptyTankMsgSent = true;
      }
    } else {
      tft.setTextColor(COLOR_GREEN, ST77XX_BLACK);
      tft.print("SISTEM AKTIF      ");
      emptyTankMsgSent = false; 

      drawSmileyFace(225, 113);
      
      if(isWateringPurple) {
         if(now - actionTimerPurple >= WATERING_DURATION || moisturePurple >= targetHumidity) {
            digitalWrite(PUMP_PIN_PURPLE, LOW);
            isWateringPurple = false;
            actionTimerPurple = now; 
         }
      } else {
         if(moisturePurple < targetHumidity && (now - actionTimerPurple >= COOLDOWN || actionTimerPurple == 0)) {
            digitalWrite(PUMP_PIN_PURPLE, HIGH);
            isWateringPurple = true;
            actionTimerPurple = now;
            purplePumpCount++;
         }
      }

      if(isWateringWhite) {
         if(now - actionTimerWhite >= WATERING_DURATION || moistureWhite >= targetHumidity) {
            digitalWrite(PUMP_PIN_WHITE, LOW);
            isWateringWhite = false;
            actionTimerWhite = now; 
         }
      } else {
         if(moistureWhite < targetHumidity && (now - actionTimerWhite >= COOLDOWN || actionTimerWhite == 0)) {
            digitalWrite(PUMP_PIN_WHITE, HIGH);
            isWateringWhite = true;
            whitePumpCount++;
            actionTimerWhite = now;
         }
      }
    }

    // --- THINGSPEAK CLOUD DATA LOGGING (Every 15 Minutes) ---
 
    if (now - lastTimeSentToCloud >= postingInterval) {
      
      if (!isnan(t) && !isnan(h)) {
        ThingSpeak.setField(1, t);
        ThingSpeak.setField(2, h);
        ThingSpeak.setField(3, moisturePurple);
        ThingSpeak.setField(4, moistureWhite);
        ThingSpeak.setField(5, targetHumidity);
        ThingSpeak.setField(7, purplePumpCount); 
        ThingSpeak.setField(8, whitePumpCount);

        int tankStatus = (digitalRead(FLOAT_SWITCH_PIN) == HIGH) ? 0 : 100;
        ThingSpeak.setField(6, tankStatus);

        int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
        
        if (httpCode == 200) {
          Serial.println("ThingSpeak Guncellemesi Basarili. ✅");
          purplePumpCount = 0;
          whitePumpCount = 0;

        } else {
          Serial.println("Buluta gonderilemedi. HTTP Hata Kodu: " + String(httpCode));
        }
      } else {
        Serial.println("Sensörden bozuk veri (NaN) geldi, bulut atlatildi!");
      }
      
      lastTimeSentToCloud = millis();
    }
  } 
  
}
