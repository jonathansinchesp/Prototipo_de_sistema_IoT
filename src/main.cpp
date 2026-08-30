#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// PINES DE LOS COMPONENTES 
#define PIN_MQ2 34
#define PIN_MQ135 35
#define PIN_BUZZER 13
#define PIN_LED_VERDE 12
#define PIN_LED_ROJO 14

// NUEVA CONFIGURACIÓN DE UMBRALES 
const int UMBRAL_CRITICO_ANALOGICO = 2457; 

// ===== RED =====
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* MQTT_SERVER = "://hivemq.com";  // 

// ===== OBJETOS MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;
int contador = 0;

// ===== VARIABLES DE CONTROL SECUENCIAL =====
int cicloSecuencia = 0; 

// ===== CONEXIÓN WiFi =====
void setup_wifi() {
  delay(10);
  Serial.println("\n🔌 Conectando a WiFi Wokwi-GUEST...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Conectado!");
    Serial.print("📶 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Error: No se pudo conectar a WiFi");
  }
}

// ===== RECONEXIÓN MQTT =====
void reconnect() {
  int intentos = 0;
  while (!client.connected() && intentos < 5) {
    Serial.print("📡 Conectando a MQTT... ");
    if (client.connect("ESP32_Gas_Node")) {
      Serial.println("✅ CONECTADO!");
    } else {
      Serial.print("❌ Falló, código: ");
      Serial.print(client.state());
      Serial.println(" Reintentando en 3s...");
      intentos++;
      delay(3000);
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("   SISTEMA DE MONITOREO DE GAS");
  Serial.println("=================================\n");
  
  pinMode(PIN_MQ2, INPUT);
  pinMode(PIN_MQ135, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);

  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_ROJO, LOW);
  noTone(PIN_BUZZER);
  
  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
}

// ===== LOOP PRINCIPAL =====
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // ===== ENVIAR DATOS CADA 6 SEGUNDOS =====
  if (millis() - ultimoEnvio > 6000) {
    ultimoEnvio = millis();
    contador++;
    
    int lecturaMQ2 = 0;
    int lecturaMQ135 = 0;
    bool alarma = false;

    if (cicloSecuencia < 3) {
      lecturaMQ2 = random(200, 2400);   
      lecturaMQ135 = random(200, 2400);
    } else {
      lecturaMQ2 = random(2500, 4095);  
      lecturaMQ135 = random(2500, 4095);
    }

    cicloSecuencia++;
    if (cicloSecuencia >= 6) {
      cicloSecuencia = 0; 
    }

    // CONVERSIÓN A PPM
    int ppmGas = map(lecturaMQ2, 0, 4095, 0, 2000);
    int ppmAire = map(lecturaMQ135, 0, 4095, 0, 2000);

    // Topes de seguridad
    if (ppmGas > 2000) ppmGas = 2000;
    if (ppmGas < 0) ppmGas = 0;
    if (ppmAire > 2000) ppmAire = 2000;
    if (ppmAire < 0) ppmAire = 0;

    // ===== LÓGICA DE CALIDAD DE AIRE =====
    String calidadAire = "";
    if (ppmAire >= 0 && ppmAire <= 800) {
      calidadAire = "Normal";
    } else if (ppmAire > 800 && ppmAire <= 1500) {
      calidadAire = "Se requiere ventilación";
    } else {
      calidadAire = "Ambiente Toxico (Como tu Ex)"; 
    }

    // ===== LÓGICA DE ALARMA =====
    if (ppmGas > 600 || ppmAire > 600) {
      alarma = true;
    } else {
      alarma = false;
    }

    // ===== CONTROL DE ACTUADORES =====
    if (alarma) {
      digitalWrite(PIN_LED_VERDE, LOW);
      digitalWrite(PIN_LED_ROJO, HIGH);
      tone(PIN_BUZZER, 1000); 
    } else {
      digitalWrite(PIN_LED_VERDE, HIGH);
      digitalWrite(PIN_LED_ROJO, LOW);
      noTone(PIN_BUZZER);      
    }
    
    // ===== MOSTRAR EN MONITOR SERIAL =====
    Serial.println("========================================");
    Serial.printf("Contador: [%d]\n", contador);
    Serial.printf("Gas: %d ppm\n", ppmGas);
    Serial.printf("Aire: %d ppm\n", ppmAire);
    Serial.printf("Calidad de aire: %s\n", calidadAire.c_str());
    Serial.printf("Alarma: %s\n", alarma ? "ACTIVA" : "INACTIVA");
    Serial.println("========================================\n");
    
    // ===== CREAR JSON =====
    JsonDocument doc;
    doc["contador"] = contador;
    doc["gas_ppm"] = ppmGas;
    doc["aire_ppm"] = ppmAire;
    doc["calidad_aire"] = calidadAire;
    doc["alarma"] = alarma;
    doc["lectura_mq2"] = lecturaMQ2;
    doc["lectura_mq135"] = lecturaMQ135;
    
    String outputJSON;
    serializeJson(doc, outputJSON);
    
    // Publicar por MQTT
    if (client.connected()) {
      if (client.publish("edificio/planta1/sensores", outputJSON.c_str())) {
        Serial.println("✅ MQTT Publicado!");
        Serial.print("📦 ");
        Serial.println(outputJSON);
      } else {
        Serial.println("❌ Error al publicar MQTT");
      }
    } else {
      Serial.println("❌ MQTT NO conectado");
    }
    Serial.println("----------------------------------------\n");
  }
  
  delay(20); 
}