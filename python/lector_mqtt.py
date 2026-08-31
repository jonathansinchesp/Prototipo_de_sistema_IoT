import paho.mqtt.client as mqtt
import json
import csv
import os

# Configuración
MQTT_HOST = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "edificio/planta1/sensores"
CSV_FILE_PATH = "data/datos_sensor.csv"

# Asegurar que la carpeta 'data' exista antes de escribir
os.makedirs(os.path.dirname(CSV_FILE_PATH), exist_ok=True)

# Escribir encabezados si el archivo es nuevo o está vacío
if not os.path.exists(CSV_FILE_PATH) or os.stat(CSV_FILE_PATH).st_size == 0:
    with open(CSV_FILE_PATH, mode="w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["Contador", "Gas_PPM", "Aire_PPM", "Alarma", "Lectura_MQ2", "Lectura_MQ135"])

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Conectado a MQTT Broker")
        client.subscribe(MQTT_TOPIC)
        print(f"📡 Suscrito al tópico: {MQTT_TOPIC}")
        print("Esperando datos...\n")
    else:
        print(f"❌ Error de conexión: {rc}")

def on_message(client, userdata, msg):
    try:
        # Decodificar el mensaje JSON
        payload = msg.payload.decode('utf-8')
        data = json.loads(payload)
        
        calidad_aire = data.get('calidad_aire', 'Desconocida')
        
        # Mostrar datos formateados en la terminal
        print("=" * 50)
        print(f" 💨 Gas: {data.get('gas_ppm', 'N/A')} ppm")
        print(f" 🌬️ Aire: {data.get('aire_ppm', 'N/A')} ppm")
        print(f" 📊 Calidad de aire: {calidad_aire}")
        print(f" 🚨 Alarma: {'ACTIVA' if data.get('alarma', False) else 'INACTIVA'}")
        print(f" 📦 Datos completos: {payload}")
        print("=" * 50)

        # ====== GUARDAR DATOS EN EL ARCHIVO CSV ======
        # Abrimos el archivo en modo "append" (añadir)
        with open(CSV_FILE_PATH, mode="a", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)

            writer.writerow([
                data.get('contador', 'N/A'),
                data.get('gas_ppm', 'N/A'),
                data.get('aire_ppm', 'N/A'),
                'ACTIVA' if data.get('alarma', False) else 'INACTIVA',
                data.get('lectura_mq2', 'N/A'),
                data.get('lectura_mq135', 'N/A')
            ])
            f.flush()
        print("💾 Guardado en CSV de manera inmediata.")
        
    except json.JSONDecodeError:
        print(f"❌ Error al decodificar JSON: {msg.payload}")
    except Exception as e:
        print(f"❌ Error al guardar en CSV: {e}")

# Configurar cliente MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Conectar al broker
try:
    client.connect(MQTT_HOST, MQTT_PORT, 60)
    print("🔌 Conectando a MQTT...")
    client.loop_forever()
except Exception as e:
    print(f"❌ Error: {e}")
    print("Verifica que Mosquitto esté corriendo.")
