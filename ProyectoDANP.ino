/*
 * Desarrollado por Brenda Alisson Huarcaya Zapana
 * Projecto: IOT 
 * Coneccion: AWS Iot Core
 * Placa: NodeMCU ESP8266
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//DATOS PARA LA CONECCION CON WIFI
const char * ssid = "Pochi";
const char * password = "#en@nos2022#";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//VARIABLES PLACA
const int sensorLDR = A0;
const int led = D1;
int limite = 500;
bool encendido = false;
bool sensor = true;
int potenciaLed = 0;

//AWS_endpoint, MQTT broker ip
const char * AWS_endpoint = "abfc4wygs0zfl-ats.iot.us-east-1.amazonaws.com";

//RECEPCION DEL MENSAJE
void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  int valor = atoi((char*) payload);
  Serial.print("El sensor esta ");
  if(valor == 1){
    encendido = true;
    Serial.print("encendido: ");
    Serial.print(valor);
    }
  else if(valor == 0){
    encendido = false;
    Serial.print("apagado: ");
    Serial.print(valor);
      }
  else{
    potenciaLed = valor;
    sensor = false;
    Serial.print("siendo controlado con potenciador");
    }
}

//CONECCION AL WIFI
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  delay(10);
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}

//RECONECCION SI ES NECESARIA
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      client.publish("outTopic", "De Brenda: Hola mundo");
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}


void setup(){
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  
  //INICIALIZACION DE SENSOR Y LED
  pinMode(sensorLDR, INPUT);
  pinMode(led, OUTPUT);
  
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  
  //SUBIMOS LOS CERTIFICADOS
  File cert = SPIFFS.open("/cert.der", "r");
  if (!cert) {
    Serial.println("Failed to open cert file");
  } else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
    
  //SUBIMOS LLAVE PRIVADA
  File private_key = SPIFFS.open("/private.der", "r"); 
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  } else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
    
  //SUBIMOS CA
  File ca = SPIFFS.open("/ca.der", "r"); 
  if (!ca) {
    Serial.println("Failed to open ca ");
  } else
    Serial.println("Success to open ca");
  delay(1000);
  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");
    
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

unsigned long before = 0;
unsigned long intervalo = 5000;

void loop(){
  if (!client.connected()) {
    reconnect();
  }
  
  //LECTURA DL SENSOR
  int rawData = analogRead(sensorLDR);
  client.loop();
  delay(1000);
  //IMPRESION DEL SENSOR EN EL MONITOR
  Serial.println(rawData);
  
  //EL FOCO PRENDE DEPENDIENDO DE SI SE ENVIA POR EL POTENCIADOR
  if(sensor){
    if(encendido){
    //EL FOCO PRENDE DEPENDIENDO DEL VALOR DEL SENSOR
      if(rawData > 700){
        analogWrite(led, 0);
      }
      else if(rawData >= 300 && rawData <= 700){
        analogWrite(led, 25);
        }
      else {
        analogWrite(led, 100);
        }
      }
    else {
      analogWrite(led, 0);
      }
  }else {
      analogWrite(led, potenciaLed);
   }
  
  //DEPENDIENDO DEL VALOR QUE SE RECIBE EN CALLBACK SE VERA SI EL CODIGO FUNCIONA O NO
  
  //MENSAJE
  char mensaje[50] = "";
  char valorSensor[4];
  sprintf(valorSensor, "%d", rawData);
  strcat(mensaje, valorSensor);

  
  //COMUNICACION PERIODICA
   if (millis() - before >= intervalo) {
      before = millis();
      
      //SE PUBLICA EL MENSAJE
      client.publish("outTopic", mensaje);
      
      //IMPRESION DEL MENSAJE
      Serial.println(mensaje);
   }
 
  //RECEPCION DEL MENSAJE
  char recivedMsg = client.subscribe("inTopic",1);
  Serial.println(recivedMsg);
}
