#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>
#include <SimpleDHT.h> 
#include <time.h>

/* ############################################################### */

// Config Firebase
#define FIREBASE_HOST "smartfarm-3b9d1.firebaseio.com"
#define FIREBASE_AUTH "hXe1j4OWA6EDGjdwQtFXHU9QkoBpDjynTCMnC9o6"
#define WIFI_SSID "iPhone"   
#define WIFI_PASSWORD "0865275915"

/* ############################################################### */

const int R1 = 25;
const int R2 = 26;
const int R3 = 27;                                     
int DHTPIN = 32;
SimpleDHT22 dht22;              //ระบุรุ่นเซ็นเซอร์รุ่น DHT11
byte temperature = 0;           //กำหนดตัวแปรเก็บค่าอุณหภูมิ
byte humidity = 0;              //กำหนดตัวแปรเก็บค่าความชื้นสัมสัทธ์
int LDRSENSOR = 34; 
int soils = 35;           //กำหนดตัวแปรเก็บค่าความชื้นสัมสัทธ์

char ntp_server1[20] = "time.navy.mi.th";
char ntp_server2[20] = "clock.nectec.or.th";
char ntp_server3[20] = "th.pool.ntp.org";

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(50);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  configTime(7 * 3600, 0, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(50);
  }
  Serial.println();
  Serial.println("Now: " + NowString());

 
  pinMode(R1, OUTPUT);    
  pinMode(R2, OUTPUT);    
  pinMode(R3, OUTPUT);    

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
   Serial.println("Start stream led control");
  Firebase.stream("/Command", [](FirebaseStream stream) {
    Serial.println("Evant: " + stream.getEvent());
    Serial.println("Path: " + stream.getPath());
    Serial.println("Data: " + stream.getDataString());
    if (stream.getEvent() == "put") {
      if (stream.getPath() == "/ID1") {
         digitalWrite(R1, stream.getDataInt());
      }
       else if (stream.getPath() == "/ID2") {
         digitalWrite(R2, stream.getDataInt());
      }
       else if (stream.getPath() == "/ID3") {
         digitalWrite(R3, stream.getDataInt());
      }
    }
   
  });
  
}

void loop() {


  digitalWrite(R1, Firebase.getInt("Command/ID1"));
  digitalWrite(R2, Firebase.getInt("Command/ID2"));
  digitalWrite(R3, Firebase.getInt("Command/ID3"));
  
  
  dht22.read(DHTPIN, &temperature, &humidity, NULL);  //อ่านค่าจากเซ็นเซอร์
  
  
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C ");
  Serial.println();
  
   int s1 = analogRead(35); //อ่านค่าความชื้นดิน
   if (s1 < 1000) {                            //ให่ค่าที่อ่านได้ต่ำกว่า 1000 เท่ากับ 1000 (4 เท่าของ 250)
    s1 = 1000;
    } else if (s1 > 3840) {                   //ให่ค่าที่อ่านได้สูงกว่า 3840 เท่ากับ 3840 (4 เท่าของ 960)
    s1 = 3840;
    }
  Serial.println(s1);  
  int PercentValue = map(s1, 1000, 3840, 0, 100);    //หาเปอร์เซนต์ 
  PercentValue = 100 - PercentValue;
  Serial.print("Soil: ");
  Serial.println(PercentValue); 

  
   int L1 = analogRead(34);     //อ่านค่าความเข้มแสง
   if (L1 < 30) {               //ให่ค่าที่อ่านได้ต่ำกว่า 30 เท่ากับ 30 (สว่าง)
    L1 = 30;
    } else if (L1 > 900) {     //ให่ค่าที่อ่านได้สูงกว่า 900 เท่ากับ 900 (มืด)
    L1 = 900;
    }
  Serial.println(L1);  
  int PercentValue2 = map(L1, 30, 900, 0, 100);    //หาเปอร์เซนต์ ให้ค่าต่ำสุดคือ 30 และสูงสุดคือ 900
  PercentValue2 = 100 - PercentValue2;                 //นำ 100 ลบเพื่อกลับค่าเปอร์เซนต์
  Serial.print("Light: ");
  Serial.println(PercentValue2);
 
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = temperature;
  root["humidity"] = humidity;
  root["soil"] = PercentValue;
  root["light"] = PercentValue2;
  root["time"] = NowString();
  
// append a new value to /logDHT
  String name = Firebase.push("Data", root);
  
// handle error
  if (Firebase.failed()) {
      Serial.print("pushing /logDHT failed:");
      Serial.println(Firebase.error());  
      return; 
  }
  
  Serial.print("pushed: /logDHT/");
  Serial.println(name);
  
  delay(50);
}

String NowString() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);

  String tmpNow = "";
  tmpNow += String(newtime->tm_year + 1900);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mon + 1);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mday);
  tmpNow += " ";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  tmpNow += ":";
  tmpNow += String(newtime->tm_sec);
  return tmpNow;
}



  

