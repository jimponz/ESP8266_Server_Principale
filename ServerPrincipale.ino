
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <espnow.h>

#include <WiFiUdp.h>

#include <WiFiClientSecure.h>
#include <NTPClient.h>

#include <EMailSender.h>
#include <Servo.h>
#include <DHT.h>
#include <CTBot.h>

#define STASSID "" // Insert Wi-Fi SSID (The wifi's name)
#define STAPSK  "" // Insert Wi-Fi password

#define IP_SERVER_LETTO "192.168.1.24"
#define IP_SERVER_BAGNO "192.168.1.89"
#define IP_SERVER_NAS "192.168.1.150"
#define IP_SERVER_SCRIVANIA "192.168.1.170"

#define THRESHOLD_LUMINOSITY 700
#define DEBUG true

//#define JSON_SIZE 512

#define photoResistorPin A0
#define dhtPin 5 //D1
#define relayPin 4 //D2
#define servoPin 2 //D4
#define pirCamera 14 //D5
#define pirScale 12 //D6  
#define ledPirSca 13 //D7  

#define BotToken "" // Insert Telegram Bot Token
int64_t CHAT_ID = ; // Insert Chat ID

const char* streamUsername = ""; // Insert here your WebServer login Username
const char* streamPassword = ""; // Insert here your WebServer login Password

const char* streamRealm = "Inserire i dati di accesso:";
const char* authFailResponse = "Autenticazione Fallita!";

CTBot myBot;
WiFiClient client;
HTTPClient http;

int threshold_luminosity = THRESHOLD_LUMINOSITY;
int luminosita = 0;

int contRiavvio = 0;

const char* ssid = STASSID;
const char* password = STAPSK;

bool stateCam = false;
bool stateBagno = false;
bool stateLetto = false;

bool stateLuceCamera = false;
//bool stateLuceScrivania = false;

EMailSender emailSend("Insert here your email", "Insert here your email's password");
EMailSender::EMailMessage message;
EMailSender::Response resp;

ESP8266WebServer server(80);
Servo servo;

int pos = 91;
int posrip = 0;

unsigned long timeStamp = 0;
unsigned long timeStampInterval = 0;

int contRequest = 0;
int contScalda = 0;
int contRaffredda = 0;

String IPPub = "";
String MAC = "";
String IPServerLetto = IP_SERVER_LETTO;
String IPServerBagno = IP_SERVER_BAGNO;
String IPServerNas = IP_SERVER_NAS;
String IPServerScrivania = IP_SERVER_SCRIVANIA;


String MQ135Data = "";

String oraServerOnline = "";

//String ultmovstanza = "";
//String ultmovbagno = "";
    
unsigned long tempoAcc;
unsigned long tempoFinale;
int tempoTot;

DHT dht(dhtPin, DHT11);

bool TCVstate = false;

int contRipe = 0;

int temp = -1;
int tempimp = -1;

int oraimp = -1;
int minuimp = -1;
int oraimpspe = -1;
int minuimpspe = -1;
int mostra = -1;
int mostraspe = -1;
int mostratemp = -1;
int controllaacc = -1;
int controllaspe = -1;
int controllatemp = -1;
int aggiorna = 1;

int scrittaacc = 1;
int scrittaspe = 1;

bool controlloAutomatico = true;
bool controlloAutomaticoLuminosita = true;
bool controlloAutomaticoLetto = true;

bool stateLuceStanza = false;
bool stateLuceBagno = false;
bool stateLuceLetto = false;
bool stateScaldabagno = false;

bool stateAntifurto = false;
bool stateAntifurtoBagno = false;

int statePirCam = LOW;
int valPirCam = 0;
int statePirSca = LOW;
int valPirSca = 0;

String ultmovcam = "";
String fineultmovcam = "";
String ultmovsca = "";
String fineultmovsca = "";

String lastAction = "";

//const long utcOffsetInSeconds = 3600;
const long utcOffsetInSeconds = 7200;
String daysOfTheWeek[] = {"Domenica", "Luned&igrave;", "Marted&igrave;",
                          "Mercoled&igrave;", "Gioved&igrave;", "Venerd&igrave;", "Sabato"};
                         
String months[12] = {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// ################################# VARIABILI SERVER SCRIVANIA #################
float CO = 0.0;
float ALCOL = 0.0;
float CO2 = 0.0;
float toluene = 0.0;
float NH4 = 0.0;
float acetone = 0.0;

bool stateLuceScrivania = false;
bool controlloAutomaticoScrivania = false;
bool controlloAutomaticoScrivaniaLuminosita = false;
int timerLuceScrivania = 1;
// ##############################################################################      
  
// MAC SERVER LETTO CC:50:E3:63:B0:57
uint8_t broadcastAddressLetto[] = {0xCC, 0x50, 0xE3, 0x63, 0xB0, 0x57};

//// MAC SERVER SCRIVANIA 
//uint8_t broadcastAddressScrivania[] = {0x, 0x, 0x, 0x, 0x, 0x};

// MAC SERVER BAGNO 84:0D:8E:A9:B3:64
uint8_t broadcastAddressBagno[] = {0x84, 0x0D, 0x8E, 0xA9, 0xB3, 0x64};

const long intervalEspNow = 1000; 
unsigned long previousMillisEspNow = 0;

typedef struct struct_message_letto {
    bool controlloAutomatico;
    bool stateLuceLetto;
   
} struct_message_letto;
struct_message_letto infoServerLetto;

typedef struct struct_message_bagno {
    bool stateLuceBagno;
    bool stateLuceStanza;
    bool stateScaldabagno;
   
} struct_message_bagno;
struct_message_bagno infoServerBagno;

typedef struct struct_sent_message {
    bool stateLuceCamera;
    int luminosita;
   
} struct_sent_message;
struct_sent_message infoServerPrincipale;

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);

  int i = 0;
  bool equalLetto = false;
  for (i = 0; i < 6; i++){
    if (mac[i] != broadcastAddressLetto[i]){
      equalLetto = false;
      break;
    }
    else{
      equalLetto = true;
    }
  }

  i = 0;
  bool equalBagno = false;
  for (i = 0; i < 6; i++){
    if (mac[i] != broadcastAddressBagno[i]){
      equalBagno = false;
      break;
    }
    else{
      equalBagno = true;
    }
  }
  
  if(equalLetto){
    memcpy(&infoServerLetto, incomingData, sizeof(infoServerLetto));
    
    controlloAutomaticoLetto = infoServerLetto.controlloAutomatico;
    stateLuceLetto = infoServerLetto.stateLuceLetto;
  }
  else if(equalBagno) {
    memcpy(&infoServerBagno, incomingData, sizeof(infoServerBagno));
 
    stateLuceBagno = infoServerBagno.stateLuceBagno;
    stateLuceStanza = infoServerBagno.stateLuceStanza;
    stateScaldabagno = infoServerBagno.stateScaldabagno;

  }
}

const int JSON_SIZE = 512;
StaticJsonDocument<JSON_SIZE> jsonDocument;

void addJsonObjectGeneric( String tag, float value) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["name"] = tag;
  obj["value"] = value;
}

void addJsonObject( String tag, float value) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj[tag] = value;
}

void getJsonServerPrincipale() {
  String buffer1 = "";
  jsonDocument.clear();
  
  addJsonObject("STATO_LUCE_CAMERA", stateLuceCamera);
  addJsonObject("STATO_LUCE_LETTO", stateLuceLetto);
  addJsonObject("STATO_STUFETTA", TCVstate);
  addJsonObject("TEMPERATURA", dht.readTemperature());
  addJsonObject("UMIDITA", dht.readHumidity());
  addJsonObject("LUMINOSITA", luminosita);
  addJsonObject("CONTROLLO_AUTOMATICO", controlloAutomatico);
  
  serializeJson(jsonDocument, buffer1);

  server.send(200,"application/json",buffer1);
  
}

String requestJsonServerScrivania(){
  Serial.println("requestJsonServerScrivania");
  http.begin(client, "http://"+IPServerScrivania+"/getJsonServerScrivania");
  int httpCode = http.GET(); 
  String result = ""; 
  
  if (httpCode == 200) {
     result = http.getString();;
  }
  else { Serial.println("Error on HTTP request");}
  http.end();
  
  return result;
}

void readJsonServerScrivania(String inputBuffer){
  if(DEBUG) Serial.println("readJsonServerScrivania");
  if (inputBuffer != ""){
    if(DEBUG) Serial.println("inputBuffer: " + inputBuffer);
        
    jsonDocument.clear();
    deserializeJson(jsonDocument, inputBuffer);

    JsonArray arr = jsonDocument.as<JsonArray>();
    for (JsonObject obj : arr) {
           
      if( obj["name"] == "CO"){
        CO = obj["value"];
      }
      else if( obj["name"] == "ALCOL"){
        ALCOL = obj["value"];
      }
      else if( obj["name"] == "CO2"){
        CO2 = obj["value"];
      }
      else if( obj["name"] == "TOLUENE"){
        toluene = obj["value"];
      }
      else if( obj["name"] == "METANO"){
        NH4 = obj["value"];
      }
      else if( obj["name"] == "ACETONE"){
        acetone = obj["value"];
      }
      else if( obj["name"] == "STATE_LUCE_SCRIVANIA"){
        stateLuceScrivania = obj["value"];
      }
      else if( obj["name"] == "CONTROLLO_AUTOMATICO_SCRIVANIA"){
        controlloAutomaticoScrivania = obj["value"];
      }
      else if( obj["name"] == "CONTROLLO_AUTOMATICO_SCRIVANIA_LUMINOSITA"){
        controlloAutomaticoScrivaniaLuminosita = obj["value"];
      }
      else if( obj["name"] == "TIMER_LUCE_SCRIVANIA"){
        timerLuceScrivania = obj["value"];
      }
    } 

    if(DEBUG){
        Serial.println(CO);
        Serial.println(ALCOL);
        Serial.println(CO2);
        Serial.println(toluene);
        Serial.println(NH4);
        Serial.println(acetone);
        Serial.println(stateLuceScrivania);
        Serial.println(controlloAutomaticoScrivania);
        Serial.println(controlloAutomaticoScrivaniaLuminosita);
        Serial.println(timerLuceScrivania);
      }  
  }
  else if(DEBUG) Serial.println("Input vuoto");
}

void setup_ota(){

  ArduinoOTA.setHostname("Esp8266_Principale");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = F("sketch");
    } else { // U_FS
      type = F("filesystem");
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      Serial.println(F("End Failed"));
    }
  });
  ArduinoOTA.begin();
}

void setup_wifi(){
  Serial.println("Connecting to ");
  Serial.print(ssid);

  String newHostname = "ESP_Principale";
  WiFi.hostname(newHostname.c_str());
  
  Serial.println("prima di wifi connect");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("dopo wifi connect");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    //WiFi.begin(ssid, password);
    Serial.print(".");
  }

  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());
    
  MAC = WiFi.macAddress();
  oraServerOnline = "Server online da: " + getGiornoAndOra();
}

void setup_esp_now(){
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_add_peer(broadcastAddressLetto, ESP_NOW_ROLE_COMBO, 1, NULL, 0); 

  //esp_now_add_peer(broadcastAddressScrivania, ESP_NOW_ROLE_COMBO, 1, NULL, 0); 
  
  esp_now_register_recv_cb(OnDataRecv);
  
 // esp_now_register_send_cb(OnDataSent);
}

void setup_bot(){
  myBot.setTelegramToken(BotToken);
  if (myBot.testConnection()) {
    myBot.sendMessage(CHAT_ID, "Server avviato e bot attivo");
  }
}

void setup_pins(){
  pinMode(relayPin, OUTPUT);
  pinMode(pirCamera, INPUT);
  pinMode(pirScale, INPUT);
  pinMode(ledPirSca, OUTPUT);
  pinMode(photoResistorPin, INPUT);
  pinMode(dhtPin, INPUT);
}

void setup_servo(){
  servo.attach(servoPin);
  delay(100);
  // UTILE PER PREPARARE UN SERVO PONENDOLO IN POSIZIONE CENTRALE RIAVVIANDO IL SERVER
  servo.write(90);
  delay(300);
}

void setup_routing(){
  server.on("/", handle_OnConnect);
  server.on("/Pannello", SendPannello);
  server.on("/On", On);
  server.on("/Off", Off);
  server.on("/AccendiLuceStanza", AccendiLuceStanza);
  server.on("/SpegniLuceStanza", SpegniLuceStanza);
  server.on("/AccendiLuceBagno", AccendiLuceBagno);
  server.on("/SpegniLuceBagno", SpegniLuceBagno);

  server.on("/accendiLuceScrivania", accendiLuceScrivania);
  server.on("/spegniLuceScrivania", spegniLuceScrivania);
  
  server.on("/AccendiLuceLetto", AccendiLuceLetto);
  server.on("/SpegniLuceLetto", SpegniLuceLetto);
  
  server.on("/AccendiScaldabagno", AccendiScaldabagno);
  server.on("/SpegniScaldabagno", SpegniScaldabagno);
  server.on("/Scalda", Scalda);
  server.on("/Raffredda", Raffredda);
  server.on("/Impostazioni", Impostazioni);
  server.on("/Streaming", Streaming);
  server.on("/Bagno", Bagno);
  server.on("/Letto", Letto);
  server.on("/ImpostaTemp", ImpostaTemp);
  server.on("/Imposta", Imposta);
  server.on("/ImpostaOff", ImpostaOff);
  server.on("/AnnullaTemp", AnnullaTemp);
  server.on("/Annulla", Annulla);
  server.on("/AnnullaOff", AnnullaOff);
  server.on("/ImpostaTemperatura", ImpostaTemperatura);
  server.on("/ImpostaOraAccensione", ImpostaOraAccensione);
  server.on("/ImpostaOraSpegnimento", ImpostaOraSpegnimento);
  server.on("/AttivaAntifurto", AttivaAntifurto);
  server.on("/DisattivaAntifurto", DisattivaAntifurto);
  server.on("/Riavvia", Riavvia);

  server.on("/AbilitaControlloAutomatico", AbilitaControlloAutomatico);
  server.on("/DisabilitaControlloAutomatico", DisabilitaControlloAutomatico);

  server.on("/AbilitaControlloAutomaticoLuminosita", AbilitaControlloAutomaticoLuminosita);
  server.on("/DisabilitaControlloAutomaticoLuminosita", DisabilitaControlloAutomaticoLuminosita);
  
  server.on("/AbilitaControlloAutomaticoLetto", AbilitaControlloAutomaticoLetto);
  server.on("/DisabilitaControlloAutomaticoLetto", DisabilitaControlloAutomaticoLetto);

  server.on("/abilitaControlloAutomaticoScrivania", abilitaControlloAutomaticoScrivania);
  server.on("/disabilitaControlloAutomaticoScrivania", disabilitaControlloAutomaticoScrivania);
  
  server.on("/getLuminosita", getLuminosita);
  server.on("/getTemperatura", getTemperatura);
  server.on("/getJsonServerData", getJsonServerData);

  server.on("/getJsonServerPrincipale", getJsonServerPrincipale);
  
  server.onNotFound(handle_NotFound);
  server.begin();
}

void getIpPubFromInternet(){
   while (IPPub == "") {
    http.begin(client, "http://api.ipify.org");

    int httpCode = http.GET();
    if (httpCode > 0) {
      IPPub = http.getString();
      Serial.println("IP Pubblico" + IPPub);
    }
    else {
      Serial.println(F("Error on HTTP request"));
    }
    http.end();
  }
}

void sendIpPubMail(){
  message.subject = "IP Pubblico: " + IPPub;
  message.message = "Nuovo IP server camera: " + IPPub + "\nhttp://" + IPPub;
  resp = emailSend.send("gianmarco.pomponi@gmail.com", message);
  Serial.println("Sending status: ");
  Serial.println(resp.desc);
}

void sendIpPubTelegram(){
  String IPtxt = "Server principale riavviato: ";
  IPtxt += "\nhttp://" + IPPub;
  myBot.sendMessage(CHAT_ID, IPtxt);
}

void setup_objects(){
  dht.begin();
  timeClient.begin();
  
}

void setup() {
  Serial.begin(115200);
  digitalWrite(relayPin, LOW); // Spegne la stufetta all'avvio (per sicurezza in caso di riavvio del server)
  
  setup_ota();
  setup_wifi();
  setup_esp_now();
  setup_bot();
  setup_pins();
  setup_servo();  
  setup_routing();
  setup_objects();
      
  getIpPubFromInternet();
  //sendIpPubMail();
  //sendIpPubTelegram();

  sendLogToCloud(getGiornoAndOra() + String(" Server avviato!<br>"));
  
  //Serial.println(F("HTTP server started"));
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  if (millis() - timeStampInterval > 2000) {
    // serve per evitare di bloccare l'interfaccia wifi durante la lettura analogica
    yield();
    
    luminosita = analogRead(photoResistorPin);
    
    //CHECK controllo automatico LUMINOSITA
    if (controlloAutomaticoLuminosita){
      if(stateLuceCamera || stateLuceLetto){
        threshold_luminosity = THRESHOLD_LUMINOSITY + 100;
      }
      else{
        threshold_luminosity = THRESHOLD_LUMINOSITY;
      }
        
      if(luminosita <= threshold_luminosity ){
          AbilitaControlloAutomatico();
      }
      else if(luminosita > threshold_luminosity ){
        if (stateLuceCamera){
           Off();
        }
        DisabilitaControlloAutomatico();
      } 
    }   

    if (temp > 27){
      Raffredda();
      sendLogToCloud(getGiornoAndOra() + String("Spengo i riscaldamenti a causa del superamento della temperatura massima (27&#176C)<br>"));
    }
      
    if (controllatemp == 1) {
      temp = (int) dht.readTemperature();
      if (temp < tempimp) {
        contRaffredda = 0;
        if (contScalda == 0) {
          Scalda();
          contScalda++;
        }
      }
      if (temp > tempimp) {
        contScalda = 0;
        if (contRaffredda == 0) {
          Raffredda();
          contRaffredda++;
        }
      }
    }
    timeStampInterval = millis();
  }
  
  if (contRequest == 0) {
    timeStamp = millis();
    contRequest++;
  }
  if (millis() - timeStamp > 60000) {
    verificaOra();
    if (stateLuceCamera && controlloAutomatico) {
      Off();
    }
    contRequest = 0;
  }


  valPirCam = digitalRead(pirCamera);
  if (valPirCam == HIGH) {
    if (statePirCam == LOW) {
      timeClient.update();
      ultmovcam = daysOfTheWeek[timeClient.getDay()] + ", " +
                  timeClient.getHours() + ":" + timeClient.getMinutes() +
                  ":" + timeClient.getSeconds();

      //Serial.print(F("Rilevato movimento in camera: "));
      //Serial.print(ultmovcam);
      //      ultmovgiocamera = timeClient.getDay();
      //      ultmovoracamera = timeClient.getHours();
      //      ultmovmincamera = timeClient.getMinutes();
      //      ultmovseccamera = timeClient.getSeconds();
      //        epomovcam = timeClient.getEpochTime();
      if (stateAntifurto) {
        String strBot = F("Rilevato movimento in CAMERA alle ore ");
        strBot += timeClient.getFormattedTime();
        strBot += "";
        strBot += " Vai su http://";
        strBot += IPPub;
        strBot += ":82";
        myBot.sendMessage(CHAT_ID, strBot);
      }
      if (stateLuceCamera == false && controlloAutomatico) {
        On();
      }
      statePirCam = HIGH;
    }
  }
  else {
    if (statePirCam == HIGH) {
      timeClient.update();
      fineultmovcam = daysOfTheWeek[timeClient.getDay()] + ", " +
                      timeClient.getHours() + ":" + timeClient.getMinutes() +
                      ":" + timeClient.getSeconds();
      //Serial.print(F("Fine movimento in camera alle: "));
      //Serial.print(fineultmovcam);
      //        ultmovgioscale = timeClient.getDay();
      //        ultmovorascale = timeClient.getHours();
      //        ultmovminscale = timeClient.getMinutes();
      //        ultmovsecscale = timeClient.getSeconds();
      statePirCam = LOW;
    }
  }

  valPirSca = digitalRead(pirScale);
  if (valPirSca == HIGH) {
    if (statePirSca == LOW) {
      timeClient.update();
      ultmovsca = daysOfTheWeek[timeClient.getDay()] + ", " +
                  timeClient.getHours() + ":" + timeClient.getMinutes() +
                  ":" + timeClient.getSeconds();
      //Serial.print(F("Rilevato movimento nelle scale: "));
      //Serial.print(ultmovsca);
      //     epomovsca = timeClient.getEpochTime();
      if (stateAntifurto) {
        String strBot = F("Rilevato movimento nelle SCALE alle ore ");
        strBot += timeClient.getFormattedTime();
        myBot.sendMessage(CHAT_ID, strBot);
      }
      for (int i = 0; i < 30; i++) {
        digitalWrite(ledPirSca, HIGH);
        delay(30);
        digitalWrite(ledPirSca, LOW);
        delay(30);
      }
      statePirSca = HIGH;
    }
  }
  else {
    if (statePirSca == HIGH) {
      timeClient.update();
      fineultmovsca = daysOfTheWeek[timeClient.getDay()] + ", " +
                      timeClient.getHours() + ":" + timeClient.getMinutes() +
                      ":" + timeClient.getSeconds();
      //Serial.print(F("Fine movimento nelle scale alle: "));
      //Serial.print(fineultmovsca);
      statePirSca = LOW;
    }
  }

  // OGNI SECONDO
  if (millis() - previousMillisEspNow >= intervalEspNow) {
    previousMillisEspNow = millis();

    infoServerPrincipale.stateLuceCamera = stateLuceCamera;
    infoServerPrincipale.luminosita = luminosita;
      
    esp_now_send(broadcastAddressLetto, (uint8_t *) &infoServerPrincipale, sizeof(infoServerPrincipale));

    //esp_now_send(broadcastAddressScrivania, (uint8_t *) &infoServerPrincipale, sizeof(infoServerPrincipale));

    String jsonRequest = requestJsonServerScrivania();
    readJsonServerScrivania(jsonRequest);
  }
  
}

void checkLogRemainingSpaceAndClear() {
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  if (fs_info.totalBytes - fs_info.usedBytes < 100) {
    SPIFFS.format();
    ESP.restart();

  }
}

void appendLog(String message) {
  //checkLogRemainingSpaceAndClear();
  File Log = SPIFFS.open("/log.txt", "a");
  if (!Log) Serial.println("file open failed");
  else {
    Log.println(message);
    Log.close();
  }
}

void sendLogToCloud(String message) {
  lastAction = "Ultimo evento: " + message;
  if (message.indexOf(" ") != -1) {
    message.replace(" ", "%20");
  }
  http.begin(client, "http://" + IPServerNas + "/LogData?message=" + message);
  int httpCode = http.GET();
  if (httpCode == 200) {
     //IPPub = http.getString();
    Serial.println("Messaggio scritto con successo! "+ message);
  }
  else { Serial.println(F("Error on HTTP request"));}
  http.end();
}

String readLog() {
  String readLog;
  File f = SPIFFS.open("/log.txt", "r");
  if (!f) Serial.println("file open failed");
  else {
    while (f.available()) {
      String line = f.readStringUntil('\n');
      readLog += line;
      Serial.print("  ");
      Serial.println(line);
    }
    f.close();
    return readLog;
  }
}

//void readJsonServerDataLetto(String json]){
//  StaticJsonDocument<JSON_SIZE> doc;
//  DeserializationError error = deserializeJson(doc, json);
//
//  const char* type = doc["type"];
//  float value = doc["value"];
//
//  stateLuceCamera = doc["STATO_LUCE_CAMERA"];
//
//
//  
//  stateLuceLetto = doc["STATO_LUCE_LETTO"];
//  
//  temperatura = doc["TEMPERATURA"] = dht.readTemperature();
//  doc["UMIDITA"] = dht.readHumidity();
//  doc["LUMINOSITA"] = luminosita;
//  doc["CONTROLLO_AUTOMATICO"] = controlloAutomatico;
//  
//  Serial.println(type);
//  Serial.println(value);
//}

void getJsonServerData() {
  StaticJsonDocument<JSON_SIZE> doc;
  char buffer[JSON_SIZE];
    
  doc["STATO_LUCE_CAMERA"] = stateLuceCamera;
  doc["STATO_LUCE_LETTO"] = stateLuceLetto;
  doc["TEMPERATURA"] = dht.readTemperature();
  doc["UMIDITA"] = dht.readHumidity();
  doc["LUMINOSITA"] = luminosita;
  doc["CONTROLLO_AUTOMATICO"] = controlloAutomatico;
  
  serializeJson(doc, buffer);
  server.send(200, "text/plain", String(buffer));
}

void SendPannello() {

  static const char page[] PROGMEM = R"=====(
<!DOCTYPE html> <html>
<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">
<title>Pannello</title>
<script src=\"https://kit.fontawesome.com/7870b58320.js\" crossorigin=\"anonymous\"></script>
<script src=\"https://code.jquery.com/jquery-3.5.1.min.js\" integrity=\"sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=\"crossorigin=\"anonymous\"></script>
<link href='https://stackpath.bootstrapcdn.com/bootswatch/4.5.2/slate/bootstrap.min.css' rel='stylesheet'>
</head><body>

<a href="/"><button class="btn rounded-pill btn-success button-border-white"><strong>Server Principale</strong></button></a>
  


<br><br><h1 class='display-4 text-center'>Pannello di controllo</h1><br>
<div class='d-inline-flex' id='iframe'>
<div>
<iframe src='http://192.168.1.251' width='650' height='800'></iframe><br>
</div>
<div style='margin-left:40px;'>
<iframe src='http://192.168.1.89' width='650' height='800'></iframe><br>
</div>
</div>
<div class='d-inline-flex' id='iframe2' style='margin-top:40px;'>
<iframe src='http://192.168.1.24' width='650' height='800'></iframe>
</div></body></html>
)=====";

server.send_P(200,"text/html",page);
}

void On(){ 
  pos = 0;
  posrip = pos+40; 
  servo.write(pos);
  delay(300);
  servo.write(posrip);
  delay(50);

  sendLogToCloud(getGiornoAndOra() + String(" Luce camera ACCESA!<br>"));
  //appendLog( getGiornoAndOra() + String(" Luce camera ACCESA!<br>"));
  stateLuceCamera = true;
  server.send(200,"text/html",SendHTML());
}

void Off(){
  pos = 180;
  posrip = pos-40; 
  servo.write(pos);
  delay(300);
  servo.write(posrip);
  delay(50);
  sendLogToCloud(getGiornoAndOra() + String(" Luce camera SPENTA!<br>"));
  //appendLog( getGiornoAndOra() + String(" Luce camera SPENTA!<br>"));
  stateLuceCamera = false;
  server.send(200,"text/html",SendHTML());
}

void Scalda(){
  TCVstate = true;
  digitalWrite(relayPin,HIGH);
  sendLogToCloud(getGiornoAndOra() + String(" Riscaldamenti ACCESI!<br>"));
  //appendLog( getGiornoAndOra() + String(" Riscaldamenti ACCESI!<br>"));
  tempoAcc = millis();
  server.send(200,"text/html",SendHTML());
  }
  
void Raffredda(){
  TCVstate = false;
  digitalWrite(relayPin,LOW);
  tempoFinale = millis() - tempoAcc;
  tempoTot = tempoFinale /1000/60;
  sendLogToCloud(getGiornoAndOra() + String(" Riscaldamenti SPENTI! ")+String("Tempo: ")+String(tempoTot)+ String(" minuti<br>"));
  //appendLog( getGiornoAndOra() + String(" Riscaldamenti SPENTI! ")+String("Tempo: ")+String(tempoTot)+ String(" minuti<br>"));
  server.send(200,"text/html",SendHTML());
  }
  
void Streaming(){
  //stateCam = true;
  sendLogToCloud(getGiornoAndOra() + String(" Streaming AVVIATO!<br>"));
  //appendLog( getGiornoAndOra() + String(" Streaming AVVIATO!<br>"));
  server.send(200, "text/html", SendStream());
}

void Bagno(){
  sendLogToCloud(getGiornoAndOra() + String(" Accesso Server Bagno!<br>"));
  server.send(200,"text/html",SendBagno());
}

void Letto(){
  sendLogToCloud(getGiornoAndOra() + String(" Accesso Server Letto!<br>"));
  server.send(200,"text/html",SendLetto());
}

void Impostazioni(){
  sendLogToCloud(getGiornoAndOra() + String(" Accesso Impostazioni IPCam!<br>"));
  //appendLog( getGiornoAndOra() + String(" Accesso Impostazioni IPCam!<br>"));
  server.send(200,"text/html",SendImpostazioni());
}

String SendBagno(){
 String ptr = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Bagno</title><meta http-equiv = \"refresh\" content = \"0; url = http://"+ IPServerBagno +"\"/></head></html>";
 return ptr;
}

String SendLetto(){
 String ptr = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Letto</title><meta http-equiv = \"refresh\" content = \"0; url = http://"+ IPServerLetto +"\"/></head></html>";
 return ptr;
}

String SendImpostazioni(){
 String ptr = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Impostazioni</title><meta http-equiv = \"refresh\" content = \"0; url = http://"+IPPub+":81"+"\"/></head></html>";
 return ptr;
}

void verificaOra(){
  timeClient.update();
  int ora = timeClient.getHours();
  int minu = timeClient.getMinutes();
  Serial.println("Richiedo orario");
  if(controllaacc == 1){
    if (ora == oraimp){
       Serial.println("Controllo ora");
      if(minu == minuimp){
        Serial.println("Controllo minuti");
        Scalda();
        Serial.println("E' arrivata l'ora di accendere la stufetta!");
        controllaacc = 0;
        oraimp = -1;
        minuimp = -1;
        server.send(200,"text/html",SendHTML());  
      }
    } 
  }
  if(controllaspe == 1){
    if (ora == oraimpspe){
       Serial.println("Controllo ora");
      if(minu == minuimpspe){
        Serial.println("Controllo minuti");
        Raffredda();
        Serial.println(F("E' arrivata l'ora di spegnere la stufetta!"));
        controllaspe = 0;
        oraimpspe = -1;
        minuimpspe = -1;
        server.send(200,"text/html",SendHTML());            
      }
    } 
  }
  
}  

void Annulla(){
  controllaacc = 0;
  oraimp = -1;
  minuimp = -1;
  aggiorna = 1;
  Serial.println("Annullato");
  server.send(200,"text/html",SendHTML());
}

void AnnullaOff(){
  controllaspe = 0;
  oraimpspe = -1;
  minuimpspe = -1;
  aggiorna = 1;
  Serial.println("Annullato");
  server.send(200,"text/html",SendHTML());
}

void AnnullaTemp(){
  Raffredda();
  contScalda = 0;
  contRaffredda = 0;
  controllatemp = 0;
  tempimp = -1;
  aggiorna = 1;
  Serial.println("Annullato");
  sendLogToCloud(getGiornoAndOra() + String(" Mantenimento temperatura ANNULLATO!<br>"));
  //appendLog( getGiornoAndOra() + String(" Mantenimento temperatura ANNULLATO!<br>"));
  server.send(200,"text/html",SendHTML());
}

void ImpostaOraAccensione() {

 oraimp = server.arg("ora").toInt(); 
 minuimp = server.arg("minuti").toInt(); 

 //Serial.print("Ora impostata:");
 Serial.println(oraimp);
 //Serial.print("Minuti:");
 Serial.println(minuimp);
 controllaacc = 1;
 //Serial.print("Controlla vale: ");
 Serial.println(controllaacc);
 mostra = 0;
 aggiorna = 1;
 scrittaacc= 1;

 server.send(200,"text/html",SendHTML());
}
void ImpostaOraSpegnimento(){
  
 oraimpspe = server.arg("ora").toInt(); 
 minuimpspe = server.arg("minuti").toInt(); 

 //Serial.print("Ora impostata spegnimento:");
 Serial.println(oraimpspe);
 //Serial.print("Minuti:");
 Serial.println(minuimpspe);
 controllaspe = 1;
 //Serial.print("Controlla vale: ");
 Serial.println(controllaspe);
 mostraspe = 0;
 aggiorna = 1;
 scrittaspe = 1;
 server.send(200,"text/html",SendHTML());
}
void ImpostaTemperatura() {
  
 tempimp = server.arg("temp").toInt(); 
 //Serial.print("Temperatura impostata:");
 Serial.println(tempimp);

 contScalda = 0; // altrimenti non si riaccende
 contRaffredda = 0; // altrimenti non si riaccende
 
 controllatemp = 1;
 //Serial.print("Controlla vale: ");
 Serial.println(controllatemp);

 mostratemp = 0;
 aggiorna = 1;

 sendLogToCloud(getGiornoAndOra() + String("Controllo automatico temperatura attivato, impostati " + String(tempimp) +"&#176C<br>"));
 
 server.send(200,"text/html",SendHTML());
}

void AbilitaControlloAutomatico() {
 controlloAutomatico = true;
 server.send(200,"text/html",SendHTML());
}

void DisabilitaControlloAutomatico() {
 controlloAutomatico = false; 
 server.send(200,"text/html",SendHTML());
}

void AbilitaControlloAutomaticoLuminosita() {
 controlloAutomaticoLuminosita = true;
 server.send(200,"text/html",SendHTML());
}

void DisabilitaControlloAutomaticoLuminosita() {
 controlloAutomaticoLuminosita = false; 
 server.send(200,"text/html",SendHTML());
}

void getLuminosita(){
  server.send(200,"text/plain",(String) luminosita);
}

void getTemperatura(){
  server.send(200,"text/plain", (String) dht.readTemperature());
}

void Riavvia(){
  server.send(200,"text/html",SendHTML());
  contRiavvio++;
  if (contRiavvio == 1){ ESP.restart(); }
  }
  
void AttivaAntifurto(){
  stateAntifurto = true;
  Serial.println("Antifurto attivato!"); 
  http.begin(client, "http://"+IPServerBagno+"/AttivaAntifurto");
 
  int httpCode = http.GET();  
  if (httpCode == 200) {
     //IPPub = http.getString();
     stateLuceStanza = true;
  }
  else { Serial.println("Error on HTTP request");}
  sendLogToCloud(getGiornoAndOra() + String(" Antifurto ATTIVATO!<br>"));
  //appendLog( getGiornoAndOra() + String(" Antifurto ATTIVATO!<br>"));
  server.send(200,"text/html",SendHTML());
}
void DisattivaAntifurto(){
  stateAntifurto = false;
  Serial.println("Antifurto disattivato!");
  sendLogToCloud(getGiornoAndOra() + String(" Antifurto DISATTIVATO!<br>"));
  //appendLog( getGiornoAndOra() + String(" Antifurto DISATTIVATO!<br>"));
  
  
  http.begin(client, "http://"+IPServerBagno+"/DisattivaAntifurto");

  //http.begin("http://"+IPServerBagno+"/DisattivaAntifurto"); 
  int httpCode = http.GET();  
  if (httpCode == 200) {
     //IPPub = http.getString();
     stateLuceStanza = true;
  }
  else { Serial.println("Error on HTTP request");} 
  server.send(200,"text/html",SendHTML());
}
void Imposta(){
  mostra = 1;
  aggiorna = 0;
  scrittaacc = 0;
  server.send(200,"text/html",SendHTML());
}
void ImpostaOff(){
  mostraspe = 1;
  aggiorna = 0;
  scrittaspe = 0;
  server.send(200,"text/html",SendHTML());
}
void ImpostaTemp(){
  mostratemp = 1;
  aggiorna = 0;
  server.send(200,"text/html",SendHTML());
}

void accendiLuceScrivania(){
  
  http.begin(client, "http://" + IPServerScrivania + "/accendiLuceScrivania");
  int httpCode = http.GET();  
  if (httpCode == 200) {
    //IPPub = http.getString();
    stateLuceScrivania = true;
  }
  else { Serial.println("Error on HTTP request");}
  http.end();
  sendLogToCloud(getGiornoAndOra() + String(" Accesa luce scrivania!<br>"));
  server.send(200,"text/html",SendHTML());
}
void spegniLuceScrivania(){
  
  http.begin(client, "http://" + IPServerScrivania + "/spegniLuceScrivania");
  int httpCode = http.GET();  
  if (httpCode == 200) {
     //IPPub = http.getString();
     stateLuceScrivania = false;
  }
  else { Serial.println("Error on HTTP request");}
  http.end();
  sendLogToCloud(getGiornoAndOra() + String(" Spenta luce stanza!<br>"));
  server.send(200,"text/html",SendHTML());
}

void AccendiLuceStanza(){
  
  http.begin(client, "http://"+IPServerBagno+"/AccendiLuceStanza");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      //IPPub = http.getString();
      stateLuceStanza = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Accesa luce stanza!<br>"));
   server.send(200,"text/html",SendHTML());
}

void SpegniLuceStanza(){
  
   http.begin(client, "http://"+IPServerBagno+"/SpegniLuceStanza");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      //IPPub = http.getString();
      stateLuceStanza = false;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Spenta luce stanza!<br>"));
   server.send(200,"text/html",SendHTML());
}

void AccendiLuceBagno(){
  
   http.begin(client, "http://"+IPServerBagno+"/AccendiLuceBagno");
   //http.begin("http://"+IPServerBagno+"/AccendiLuceBagno"); 
   int httpCode = http.GET();  
   if (httpCode == 200) {
      stateLuceBagno = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Accesa luce bagno!<br>"));
   //appendLog( getGiornoAndOra() + String(" Accesa luce bagno!<br>"));
   server.send(200,"text/html",SendHTML());
}
void SpegniLuceBagno(){
   
   http.begin(client, "http://"+IPServerBagno+"/SpegniLuceBagno");
   //http.begin("http://"+IPServerBagno+"/SpegniLuceBagno"); 
   int httpCode = http.GET();  
   if (httpCode == 200) {
      stateLuceBagno = false;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
    sendLogToCloud(getGiornoAndOra() + String(" Spenta luce bagno!<br>"));
   //appendLog( getGiornoAndOra() + String(" Spenta luce bagno!<br>"));
   server.send(200,"text/html",SendHTML());
}

void AccendiLuceLetto(){
   String serverIp = "192.168.1.24";
   //http.begin(client, "http://"+IPServerBagno+"/AccendiLuceBagno");
   http.begin(client, "http://"+serverIp+"/AccendiLuceLetto");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      stateLuceLetto = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Accesa luce letto!<br>"));
   //appendLog( getGiornoAndOra() + String(" Accesa luce bagno!<br>"));
   server.send(200,"text/html",SendHTML());
}
void SpegniLuceLetto(){
  // AGGIORNA 
   String serverIp = "192.168.1.24";
   //http.begin(client, "http://"+IPServerBagno+"/AccendiLuceBagno");
   http.begin(client, "http://"+serverIp+"/AccendiLuceLetto");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      stateLuceLetto = false;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
    sendLogToCloud(getGiornoAndOra() + String(" Spenta luce letto!<br>"));
   //appendLog( getGiornoAndOra() + String(" Spenta luce bagno!<br>"));
   server.send(200,"text/html",SendHTML());
}

void AbilitaControlloAutomaticoLetto(){
  
   http.begin(client, "http://"+IPServerLetto+"/AbilitaControlloAutomatico");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      controlloAutomaticoLetto = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Abilitato controllo automatico letto!<br>"));

   server.send(200,"text/html",SendHTML());
}

void DisabilitaControlloAutomaticoLetto(){
   
   http.begin(client, "http://"+IPServerLetto+"/DisabilitaControlloAutomatico");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      controlloAutomaticoLetto = false;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Disabilitato controllo automatico letto!<br>"));

   server.send(200,"text/html",SendHTML());
}

void abilitaControlloAutomaticoScrivania(){
  
   http.begin(client, "http://"+IPServerScrivania+"/abilitaControlloAutomatico");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      controlloAutomaticoScrivania = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Abilitato controllo automatico scrivania!<br>"));
   server.send(200,"text/html",SendHTML());
}

void disabilitaControlloAutomaticoScrivania(){
  
   http.begin(client, "http://"+IPServerScrivania+"/disabilitaControlloAutomatico");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      controlloAutomaticoScrivania = false;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Disabilitato controllo automatico scrivania!<br>"));
   server.send(200,"text/html",SendHTML());
}

void AccendiScaldabagno(){
  
   http.begin(client, "http://"+IPServerBagno+"/AccendiScaldaBagno");
   //http.begin("http://"+IPServerBagno+"/AccendiScaldaBagno"); 
   int httpCode = http.GET();  
   if (httpCode == 200) {
      //IPPub = http.getString();
      stateLuceStanza = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Scaldabagno ACCESO!<br>"));
   //appendLog( getGiornoAndOra() + String(" Scaldabagno ACCESO!<br>"));
   server.send(200,"text/html",SendHTML());
}
void SpegniScaldabagno(){
   
   http.begin(client, "http://"+IPServerBagno+"/SpegniScaldaBagno");
   //http.begin("http://"+IPServerBagno+"/SpegniScaldaBagno"); 
   int httpCode = http.GET();  
   if (httpCode == 200) {
      //IPPub = http.getString();
      stateLuceStanza = true;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   sendLogToCloud(getGiornoAndOra() + String(" Scaldabagno SPENTO!<br>"));
   //appendLog( getGiornoAndOra() + String(" Scaldabagno SPENTO!<br>"));
   server.send(200,"text/html",SendHTML());
}

String getMQ135Data(){
  
  http.begin(client, "http://"+IPServerScrivania+"/getStringMQ135Table");
   int httpCode = http.GET();  
   if (httpCode == 200) {
      //IPPub = http.getString();
      MQ135Data = http.getString();;
   }
   else { Serial.println("Error on HTTP request");}
   http.end();
   //sendLogToCloud(getGiornoAndOra() + String(" Accesa luce stanza!<br>"));
   //server.send(200,"text/html",SendHTML());
   return MQ135Data;
}

String getGiornoAndOra(){
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  String dayName = daysOfTheWeek[timeClient.getDay()];
  String currentHour = String(timeClient.getHours());
  currentHour += ":";
  currentHour += String(timeClient.getMinutes());
  currentHour += ":";
  currentHour += String(timeClient.getSeconds());
  currentHour += " ";
  
  String ret = dayName;
  ret+=", ";
  ret+=currentHour;
  return ret;
}

String generateMQ135Table(){

  String dati = "<table class=\"table table-hover\">";
  dati += "<tr class=\"table-default\">";
  dati += "<th><b>Gas</b></th>";
  dati += "<th><b>Valore rilevato</b></th>";
  dati += "</tr>";
  
  dati += "<tr class=\"table-warning\">";
  dati += "<td><b>CO:</b></td>";
  dati += "<td><b>";
  dati += CO;
  dati += "</b></td> ";
  dati += "</tr>";

  dati += "<tr class=\"table-dark\">";
  dati += "<td><b>ALCOL:</b></td>";
  dati += "<td><b>";
  dati += ALCOL;
  dati += "</b></td> ";
  dati += "</tr>";

  dati += "<tr class=\"table-danger\">";
  dati += "<td><b>CO2:</b></td>";
  dati += "<td><b>";
  dati += CO2;
  dati += "</b></td> ";
  dati += "</tr>";

  dati += "<tr class=\"table-success\">";
  dati += "<td><b>Toluene:</b></td>";
  dati += "<td><b>";
  dati += toluene;
  dati += "</b></td> ";
  dati += "</tr>";

  dati += "<tr class=\"table-warning\">";
  dati += "<td><b>Metano:</b></td>";
  dati += "<td><b>";
  dati += NH4;
  dati += "</b></td> ";
  dati += "</tr>";

  dati += "<tr class=\"table-info\">";
  dati += "<td><b>Acetone:</b></td>";
  dati += "<td><b>";
  dati += acetone;
  dati += "</b></td> ";
  dati += "</tr>";
  
  dati += "</table>";

  if (CO >= 4.0){

    dati += "<br><b>STAI FUMANDO VERO?! :) </b><br>";
  }
  else{
    dati += "<br>";
  }

  if (CO2 >= 5.0 || CO > 5.0){
    dati += "<b>APRI LA FINESTRA!!</b>";
  }
  if (CO2 >= 4.0 && CO2 < 5.0){
    
    dati += "<b>Aria scadente!</b>";
  }
  if (CO2 >= 3.0 && CO2 < 4.0){
    
    dati += "<b>Aria mediocre!</b>";
  }
  if (CO2 < 3.0 && CO < 3.0){
  
    dati += "<b>Aria ottima!</b>";
  }
  
  return dati;
}
  
void handle_OnConnect() {
  if(!server.authenticate(streamUsername, streamPassword)) {
    Serial.println(F("STREAM auth required, sending request"));
    return server.requestAuthentication(BASIC_AUTH, streamRealm, authFailResponse);
  }
    
  server.send(200, "text/html", SendHTML());
  //server.send(200, "text/html", SendTestHTML());
}
void handle_NotFound(){
  server.send(404, "text/json", "Not found");
}


String SendStream(){
 String ptr = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Streaming</title><meta http-equiv = \"refresh\" content = \"0; url = http://"+IPPub+":82"+"\"/></head></html>";
 return ptr;
}

String SendTestHTML(){
  String ptr = "<!DOCTYPE html> <html> <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Controllo luce</title>\n";

  ptr +="</head><body>\n";
  ptr += "ok";
  ptr += "</body></html>\n";
  return ptr;
}

String SendHTML(){
  
  String ptr = "<!DOCTYPE html> <html> <head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Controllo luce</title>\n";

  ptr += "<script src=\"https://code.jquery.com/jquery-3.5.1.min.js\" integrity=\"sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=\"crossorigin=\"anonymous\"></script>";
  ptr += "<script src='https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.bundle.min.js'></script>";
  ptr += "<link href='https://stackpath.bootstrapcdn.com/bootswatch/4.5.2/slate/bootstrap.min.css' rel='stylesheet'>"; 
  
  ptr += "<style>"; 
  ptr += ".button-border-white{"; 
  ptr += " border:2px solid white;"; 
  ptr += "}"; 
  
  ptr += "</style>"; 
  
  ptr += "<script>\n"; 
  ptr += "function reboot(){if(confirm(\"Sei sicuro di voler riavviare il server?\")){\n";
  ptr += "var xmlHttp = new XMLHttpRequest();\n";
  ptr += "xmlHttp.open( \"GET\", \"/Riavvia\", true );\n"; // synchronous request
  ptr += "xmlHttp.send( null );\n";
  ptr += "alert(\"Server riavviato!\");}}";
  
  ptr += "function stopTimer(){\n";
  ptr += "clearInterval(interId);}\n";
 
  ptr += "function startTimer(){interId = setInterval(loadDoc,1000);}\n";
  
  ptr += "function toggle_visibility(id) {\n";
  ptr += "var e = document.getElementById(id);\n";
  ptr += "if(e.style.display == 'block')e.style.display = 'none';\n";
  ptr += "else e.style.display = 'block';}\n";
  ptr += "</script>\n";
   
  if (aggiorna == 1){

    ptr +="<script> var interId = setInterval(loadDoc,1000);\n";
    ptr +="function loadDoc() { var xhttp = new XMLHttpRequest();\n";
    ptr +="xhttp.onreadystatechange = function() {\n";
    ptr +="if (this.readyState == 4 && this.status == 200) {\n";
    ptr +="document.getElementById(\"webpage\").innerHTML =this.responseText}};\n";
    ptr +="xhttp.open(\"GET\", \"/\", true);\n";
    ptr +="xhttp.send();}</script>\n";
    
  }
  ptr +="</head><body>\n";
    
  ptr +="<div class =\"text-center\" style=\"display:none\" id=\"info\">\n";
  ptr +="<br><a href=\"http://";
  ptr +=IPPub;
  ptr +="\"<h6>IP: \n";
  ptr += IPPub;
  ptr +="</a><br> MAC: \n";
  ptr += MAC;
  ptr +="</h6></div>\n";
  
  ptr +="<div class =\"text-center m-3\" id=\"webpage\">\n";
  ptr += "<br>";
  
  ptr += "<button type=\"button\" onclick =\"\" class=\"btn rounded-pill btn-success text-white button-border-white \"><strong>";
  ptr += oraServerOnline;
  ptr += "</strong></button>\n";
  
  ptr += "<button type=\"button\" onclick =\"\" class=\"btn rounded-pill btn-dark text-white button-border-white \" ><strong>";
  ptr += lastAction;
  ptr += "</strong></button>\n";
  ptr += "<br>";
  ptr += "<br>";

  
  if (stateCam){ptr +="<a href=\"Streaming\"><button class=\"btn rounded-pill btn-success button-border-white\"><strong>Attiva IPcam</strong></button></a>\n";}
  else {ptr +="<a href=\"Streaming\"><button class=\"btn rounded-pill btn-warning button-border-white\"><strong>Attiva IPcam</strong></button></a>\n";}
  
  ptr += "<a href=\"Impostazioni\"><button class=\"btn rounded-pill btn-info button-border-white\"><strong>Impostazioni</strong></button></a>\n";
      
  if (stateBagno){ptr += "<a href=\"Bagno\"><button class=\"btn rounded-pill btn-success button-border-white\"><strong>Server Bagno</strong></button></a>\n";}
  else {ptr +="<a href=\"Bagno\"><button class=\"btn rounded-pill btn-warning button-border-white\"><strong>Server Bagno</strong></button></a>\n";}

  if (stateLetto){ptr += "<a href=\"Letto\"><button class=\"btn rounded-pill btn-success button-border-white\"><strong>Server Letto</strong></button></a>\n";}
  else {ptr +="<a href=\"Letto\"><button class=\"btn rounded-pill btn-warning button-border-white\"><strong>Server Letto</strong></button></a>\n";}
  
  ptr += "<a href='Pannello' > <button type=\"button\" onclick =\"move()\" class=\"btn rounded-pill btn-light button-border-white\"><strong>";
  ptr += getGiornoAndOra()+ "</strong></button></a>\n";
  if (!stateAntifurto){ ptr+= "<a href=\"AttivaAntifurto\"><button type=\"button\" class=\"btn rounded-pill btn-danger button-border-white\"><strong>Attiva antifurto</strong></button></a>\n";}
  else { ptr+= "<a href=\"DisattivaAntifurto\"><button type=\"button\" class=\"btn rounded-pill btn-success button-border-white \"><strong>Disattiva antifurto</strong></button></a>\n";}

  ptr += "<button class=\"btn rounded-pill btn-info button-border-white \" id =\"btnInfo\" onclick=\"toggle_visibility('info')\"><strong>Info</strong></button>\n";
  ptr += "<button type=\"button\" onclick = \"reboot()\" class=\"btn rounded-pill btn-danger button-border-white \"><strong>Riavvia</strong></button>\n";
  
  if (stateScaldabagno){ptr += "<a href=\"SpegniScaldabagno\"><button title =\"Clicca per spegnerlo\" class=\"btn rounded-pill btn-danger button-border-white \"><strong>Scaldabagno ACCESO</strong></button></a>\n";}
  else {ptr += "<a href=\"AccendiScaldabagno\"><button title =\"Clicca per accenderlo\"class=\"btn rounded-pill btn-success button-border-white \"><strong>Scaldabagno spento</strong></button></a>\n";}
  
  ptr += "<br><br><h1 class=\"display-4\">Pannello di controllo luci</h1>\n";
  ptr+= "<div class=\"jumbotron\"><div class=\"d-inline-flex\">";
  
  if (stateLuceCamera) { ptr+= "<div class=\"card rounded-pill text-white bg-warning mb-3 border-white\" style=\"max-width: 15rem;\">\n";  }
  else{ ptr += "<div class=\"card rounded-pill text-white bg-info mb-3 border-white\" style=\"max-width: 15rem;\">\n";  }
  ptr += "<div class=\"card-body \"> <h4 class=\"card-title\">Camera</h4>";
  
  if (stateLuceCamera) { ptr+= "<span class=\"badge badge-pill badge-light\"><b>Luce accesa</b></span>\n"; }
  else{ ptr+= "<span class=\"badge badge-pill badge-light\"><b>Luce spenta</b></span>\n"; }
  
  if (!controlloAutomatico){
    ptr+= "<br><br><a href=\"On\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm \"><b>Accendi</b></button></a>\n";
    ptr+= "<a href=\"Off\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm \"><b>Spegni</b></button></a>\n";
    ptr+= "<a href=\"AbilitaControlloAutomatico\"><button type=\"button\" class=\"btn rounded-pill btn-primary btn-sm \"><b>Automatizza</b></button></a> </div></div>\n";
  }
  else{
    ptr+= "<a href=\"DisabilitaControlloAutomatico\"><button type=\"button\" class=\"btn rounded-pill btn-light btn-sm \"><b>Disabilita controllo automatico</b></button></a> </div></div>\n";
  }
  
  if(stateLuceLetto) { ptr+= "<div class=\"card rounded-pill text-white bg-warning mb-3 border-white\" style=\"max-width: 15rem;\">\n"; }
  else{ ptr += "<div class=\"card rounded-pill text-white bg-info mb-3 border-white \" style=\"max-width: 15rem;\">\n"; }
  ptr += "<div class=\"card-body\"> <h4 class=\"card-title\">Letto</h4>";
  
  if(stateLuceLetto) { ptr+= "<span class=\"badge badge-pill badge-light\"><b>Luce accesa</b></span>\n"; }
  else{ ptr+= "<span class=\"badge badge-pill badge-light\"><b>Luce spenta</b></span>\n"; }
  
  if (!controlloAutomaticoLetto){
    ptr+= "<br><br><a href=\"On\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm \"><b>Accendi</b></button></a>\n";
    ptr+= "<a href=\"Off\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm \"><b>Spegni</b></button></a>\n";
    ptr+= "<a href=\"AbilitaControlloAutomaticoLetto\"><button type=\"button\" class=\"btn rounded-pill btn-primary btn-sm \"><b>Automatizza</b></button></a> </div></div>\n";
  }
  else{
    ptr+= "<a href=\"DisabilitaControlloAutomaticoLetto\"><button type=\"button\" class=\"btn rounded-pill btn-light btn-sm \"><b>Disabilita controllo automatico</b></button></a> </div></div>\n";
  }


  if (stateLuceScrivania) {ptr+= "<div class=\"card rounded-pill text-white bg-warning mb-3\" style=\"max-width: 15rem;\">\n";}
  else {ptr += "<div class=\"card rounded-pill text-white bg-info mb-3\" style=\"max-width: 15rem;\">\n";}
  ptr += "<div class=\"card-body\"> <h4 class=\"card-title\">Scrivania</h4>";
  
  if (stateLuceScrivania) {ptr+= "<span class=\"badge badge-pill badge-light\">Luce accesa</span>\n";}
  else {ptr+= "<span class=\"badge badge-pill badge-light\">Luce spenta</span>\n";}

  if (!controlloAutomaticoScrivania){
    ptr+= "<br><br><a href=\"accendiLuceScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm \"><b>Accendi</b></button></a>\n";
    ptr+= "<a href=\"spegniLuceScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm \"><b>Spegni</b></button></a>\n";
    ptr+= "<a href=\"abilitaControlloAutomaticoScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-primary btn-sm \"><b>Automatizza</b></button></a> </div></div>\n";
  }
  else{
    ptr+= "<a href=\"disabilitaControlloAutomaticoScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-light btn-sm \"><b>Disabilita controllo automatico</b></button></a> </div></div>\n";
  }
  
//  ptr += "<br><br><a href=\"accendiLuceScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm text-white button-border-white\">Accendi</button></a>\n";
//  ptr += "<a href=\"spegniLuceScrivania\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm text-white button-border-white \">Spegni</button></a></div></div>\n";

  if (stateLuceStanza) {ptr+= "<div class=\"card rounded-pill text-white bg-warning mb-3\" style=\"max-width: 15rem;\">\n";}
  else {ptr += "<div class=\"card rounded-pill text-white bg-info mb-3\" style=\"max-width: 15rem;\">\n";}
  ptr += "<div class=\"card-body\"> <h4 class=\"card-title\">Studio</h4>";
  
  if (stateLuceStanza) {ptr+= "<span class=\"badge badge-pill badge-light\">Luce accesa</span>\n";}
  else {ptr+= "<span class=\"badge badge-pill badge-light\">Luce spenta</span>\n";}
  
  ptr += "<br><br><a href=\"AccendiLuceStanza\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm text-white button-border-white\">Accendi</button></a>\n";
  ptr += "<a href=\"SpegniLuceStanza\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm text-white button-border-white \">Spegni</button></a></div></div>\n";
  
  if (stateLuceBagno) {ptr+= "<div class=\"card rounded-pill text-white bg-warning mb-3\" style=\"max-width: 15rem;\">\n";}
  else {ptr += "<div class=\"card rounded-pill text-white bg-info mb-3\" style=\"max-width: 15rem;\">\n";}
  ptr += "<div class=\"card-body\"> <h4 class=\"card-title\">Bagno</h4>";
  if (stateLuceBagno) {ptr+= "<span class=\"badge badge-pill badge-light\">Luce accesa</span>\n";}
  else {ptr+= "<span class=\"badge badge-pill badge-light\">Luce spenta</span>\n";}

  ptr += "<br><br><a href=\"AccendiLuceBagno\"><button type=\"button\" class=\"btn rounded-pill btn-danger btn-sm text-white button-border-white\">Accendi</button></a>\n";
  ptr += "<a href=\"SpegniLuceBagno\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm text-white button-border-white \">Spegni</button></a>\n";
  
  ptr += "</div></div><br>"; // serve
  
  ptr += "</div></div>"; //chiude il jumbotron


//  ptr += "<div class=\"d-inline-flex\"><div class=\"card text-white bg-primary mb-3\" style=\"max-width: 17rem;\">\n";
//  ptr += "  <div class=\"card-header\"><h4>Monitor Qualità aria</h4></div>\n";
//  ptr += "  <div class=\"card-body\" id=\"dati\">\n";

  //ptr += getMQ135Data();
  
  //ptr += generateMQ135Table();
  
//  ptr += "<script> var interDataId = setInterval(loadData,1000);\n";
//  ptr += "function loadData() { var xhttp = new XMLHttpRequest();\n";
//  ptr += "xhttp.onreadystatechange = function() {\n";
//  ptr += "if (this.readyState == 4 && this.status == 200) {\n";
//  ptr += "document.getElementById(\"dati\").innerHTML =this.responseText}};\n";
//  ptr += "xhttp.open(\"GET\", \"http://" + IPServerScrivania + "/getStringMQ135Table\", true);\n";
//  ptr += "xhttp.send();}</script>\n";
    
  ptr += "</div></div>\n";

  
  ptr += "<div class=\"d-inline-flex\"><div class=\"card text-white bg-primary mb-3\" style=\"max-width: 17rem;\">\n";
  ptr += "  <div class=\"card-header\"><h4>Info sensori</h4></div>\n";
  ptr += "  <div class=\"card-body\">\n";

//  ptr+= "<p class=\"text-info lead\"><b>STUDIO: \n";
////  if (ultmovorastanza > -1){
////    ptr+= daysOfTheWeek[ultmovgiostanza];
////    ptr+= ", ";
////    ptr+= String(ultmovorastanza);
////    ptr+= ":";
////    ptr+= String(ultmovminstanza);
////    ptr+= ":";
////    ptr+= String(ultmovsecstanza);
////  }
//  ptr+= "</b></p>\n";

//  ptr+= "<p class=\"text-light lead\"><b>BAGNO: \n";
////  if (ultmovorabagno > -1){
////      ptr+= daysOfTheWeek[ultmovgiobagno];
////      ptr+= ", ";
////      ptr+= String(ultmovorabagno);
////      ptr+= ":";
////      ptr+= String (ultmovminbagno);
////      ptr+= ":";
////      ptr+= String(ultmovsecbagno);
////    }
//  ptr+= "</b></p>\n";

  ptr += "<h3>Luminosit&#225: \n";
  ptr += luminosita;
  ptr += "</h3>\n";
  ptr += "<br>";
  
  if (!controlloAutomaticoLuminosita){
    ptr+= "<a href=\"AbilitaControlloAutomaticoLuminosita\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm \"><b>Automatizza</b></button></a> \n";
  }
  else{
    ptr+= "<a href=\"DisabilitaControlloAutomaticoLuminosita\"><button type=\"button\" class=\"btn rounded-pill btn-light btn-sm \"><b>Disabilita controllo luminosit&#225</b></button></a> \n";
  }
  ptr += "<br>";

  ptr += "<br><br>";
  ptr += "<h4>Ultimi movimenti: </h4>";
  ptr += "<br>";
  ptr += "<p class=\"text-success lead\"><b>CAMERA: \n";
  ptr += ultmovcam;
  ptr += "</b></p>\n";

  ptr += "<p class=\"text-warning lead\"><b>SCALE: \n";
  ptr += ultmovsca;
  ptr += "</b></p>\n";

//  ptr += "<p class=\"text-info lead\"><b>STANZA: \n";
//  ptr += ultmovstanza;
//  ptr += "</b></p>\n";
//
//  ptr += "<p class=\"text-light lead\"><b>BAGNO: \n";
//  ptr += ultmovbagno;
//  ptr += "</b></p>\n";
  
  ptr += "</div></div>\n";
      
  ptr += "<div class=\"card text-white bg-primary mb-3\" style=\"max-width: 25rem;\">\n";
  ptr += "<div class=\"card-header\"><h4>Riscaldamento  ";
  
  if(TCVstate){ ptr+= "<span class=\"badge badge-pill badge-danger\"><b>Acceso</b></span>\n"; }
  else{ ptr+= "<span class=\"badge badge-pill badge-success\"><b>Spento</b></span>\n"; }
  
  ptr += "</h4></div>\n";
  ptr += "  <div class=\"card-body\">\n";

  ptr += "<h3>Temperatura: \n";
  //ptr += "<div id=\"labelTemperatura\">";
  ptr += dht.readTemperature();
  //ptr += "</div>";
  ptr += "&#176C</h3>\n";
  
  ptr += "<h3>Umidit&#225: \n";
  ptr += dht.readHumidity();
  ptr += "%</h3>\n";
  
//  ptr += "<h3>Luminosit&#225: \n";
//  ptr += luminosita;
//  ptr += "</h3>\n";
//
//  if (!controlloAutomaticoLuminosita){
//    ptr+= "<a href=\"AbilitaControlloAutomaticoLuminosita\"><button type=\"button\" class=\"btn rounded-pill btn-success btn-sm \"><b>Automatizza</b></button></a> \n";
//  }
//  else{
//    ptr+= "<a href=\"DisabilitaControlloAutomaticoLuminosita\"><button type=\"button\" class=\"btn rounded-pill btn-light btn-sm \"><b>Disabilita controllo luminosit&#225</b></button></a> \n";
//  }
//  ptr += "<br><br>";
  
  if(TCVstate){ ptr+= "<a href=\"Raffredda\"><button type=\"button\" class=\"btn btn-success btn-sm \"><b>Spegni</b></button></a>\n"; }
  else{ ptr+= "<a href=\"Scalda\"><button type=\"button\" class=\"btn btn-danger btn-sm \"><b>Accendi</b></button></a>\n"; }

  ptr += "<br><br><div class=\"dropdown btn-group\">\n";
  if (tempimp < 1){ ptr += "<button class=\"btn btn-secondary dropdown-toggle\" onmouseleave=\"startTimer()\" onmouseover=\"stopTimer()\" id=\"menu1\" type=\"button\" data-toggle=\"dropdown\"><b>Temperatura da mantenere: </b><strong>nessuna</strong>";}
  if (tempimp > 0){
    ptr += "<button class=\"btn btn-success dropdown-toggle\" onmouseleave=\"startTimer()\" onmouseover=\"stopTimer()\" id=\"menu1\" type=\"button\" data-toggle=\"dropdown\">Sto mantenendo: <strong>";
    ptr += tempimp;
    ptr += " °C</strong>";
  }
  ptr += "<span class=\"caret\"></span></button>\n";
  ptr += "<ul class=\"dropdown-menu\" role=\"menu\" aria-labelledby=\"menu1\">\n";
  ptr += "<a class=\"dropdown-item\" href=\"/ImpostaTemperatura?temp=25\">25°C</a>\n";
  ptr += "<a class=\"dropdown-item\" href=\"/ImpostaTemperatura?temp=24\">24°C</a>\n";
  ptr += "<a class=\"dropdown-item\" href=\"/ImpostaTemperatura?temp=23\">23°C</a>\n";
  ptr += "<a class=\"dropdown-item\" href=\"/ImpostaTemperatura?temp=22\">22°C</a>\n";
  
  ptr += "<a class=\"dropdown-item\" href=\"/AnnullaTemp\">Annulla</a>\n";
  ptr += "</ul></div>\n";

  if (mostra == 1){
    ptr+= "<form action=\"/ImpostaOraAccensione\">\n";
    ptr+= "<info>Inserisci l'ora a cui vuoi accendere il riscaldamento:</info><br>\n";
    ptr+= "<br><input type=\"text\" name=\"ora\" ><br>\n";
    ptr+= "<info>Inserisci i minuti:</info><br>\n";
    ptr+= "<br><input type=\"text\" name=\"minuti\" >\n";
    ptr+= "<br><br>\n";
    ptr+= "<input type=\"submit\" value=\"Programma accensione\">\n";
    ptr+= "</form> ";
  }
  ptr+= "<br><br><risc><b>Ora accensione: </b></risc>\n";
  ptr+= "<spe>\n";
  if (oraimp == -1){
                    ptr+= "<spe><b>nessuna</b></spe>\n";
                    ptr+= "<a href=\"Imposta\"><button type=\"button\" class=\"btn rounded-pill btn-info \"><b>Imposta</b></button></a>\n";}
  else { ptr += oraimp;
          ptr += ":\n";}

  if (minuimp == -1){ ptr+=""; }
  else { ptr += minuimp; }
  ptr+= "</spe>\n";
  if (oraimp != -1){ ptr+= "<a href=\"Annulla\"><button type=\"button\" class=\"btn rounded-pill btn-info \"><b>Annulla</b></button><a/>"; }
  ptr+= "<br>\n";

  if (mostraspe == 1){
    ptr+= "<form action=\"/ImpostaOraSpegnimento\">\n";
    ptr+= "<br><info>Inserisci l'ora a cui vuoi spegnere il riscaldamento:</info><br>\n";
    ptr+= "<br><input type=\"text\" name=\"ora\" ><br>\n";
    ptr+= "<br><info>Inserisci i minuti:</info><br>\n";
    ptr+= "<br><input type=\"text\" name=\"minuti\" >\n";
    ptr+= "<br><br>\n";
    ptr+= "<input type=\"submit\" value=\"Programma spegnimento\">\n";
    ptr+= "</form> ";
  }
  if (scrittaspe == 1){
    ptr+= "<br><risc><b>Ora spegnimento: </b></risc>\n";
    if (oraimpspe == -1) ptr+= "<spe><b>nessuna </b></spe><a href=\"ImpostaOff\"><button type=\"button\" class=\"btn rounded-pill btn-info \"><b>Imposta</b></button></a>\n";
    else  ptr+=oraimpspe + String(":\n");
    if (minuimpspe != -1) ptr += minuimpspe;  
    if (oraimpspe != -1){ ptr += "<a href=\"AnnullaOff\"><button type=\"button\" class=\"btn rounded-pill btn-info \"><b>Annulla</b></button><a/>";}
    ptr+= "<br>\n";
  }
  ptr += "<br>\n";
  //ptr += readLog();
  ptr += "</div></div></div></div>\n"; //servono tutti
  
  ptr += "</body></html>\n";
  return ptr;
}
