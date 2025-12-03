import random
from datetime import datetime, timedelta, timezone

from pymongo.mongo_client import MongoClient
from pymongo.server_api import ServerApi

# -----------------------------
# Configuración de MongoDB Atlas
# -----------------------------


MONGO_URI = (
    "mongodb+srv://jasonalvarado5539_db_user:3j84DDCbL6hmX1tg"
    "@cluster0.hhe4ooq.mongodb.net/MonitoreoEnergetico?appName=Cluster0"
)

DB_NAME = "MonitoreoEnergetico"
COLLECTION_NAME = "measurements"

# Cliente usando la API estable v1 (como sugiere Atlas)
client = MongoClient(MONGO_URI, server_api=ServerApi("1"))
db = client[DB_NAME]
col = db[COLLECTION_NAME]


def generar_medicion(device_id: str, ts: datetime) -> dict:
    """
    Genera una medición DENTRO de los rangos definidos:

    - voltage: 80.0 a 260.0 V
    - current: 0.000 a 100.000 A
    - act_power: 0.0 a 23.0 kW
    - fact_power: 0.00 a 1.00
    - freq: 45.0 a 65.0 Hz
    - act_energy: 0 a 9999.99 kWh
    - alarm: True / False
    """

    # Valores "razonables" simulados
    voltage = random.uniform(110.0, 125.0)       # V
    current = random.uniform(0.0, 20.0)          # A
    freq = random.uniform(59.5, 60.5)            # Hz

    # Potencia activa en kW (aprox P = V * I / 1000)
    act_power_kw = (voltage * current) / 1000.0  # kW

    # Factor de potencia entre 0.8 y 1.0
    fact_power = random.uniform(0.8, 1.0)

    # Energía acumulada simulada (kWh)
    act_energy = random.uniform(0.0, 50.0)

    # Alarma si la potencia pasa 3 kW (ejemplo)
    alarm = act_power_kw > 3.0

    return {
        "device_id": device_id,
        "timestamp": ts,  # datetime en UTC
        "voltage": round(voltage, 2),
        "current": round(current, 3),
        "act_power": round(act_power_kw, 3),
        "fact_power": round(fact_power, 2),
        "freq": round(freq, 2),
        "act_energy": round(act_energy, 2),
        "alarm": alarm,
    }


def seed_datos():
    """
    Borra datos antiguos de pruebas, crea índice y carga datos nuevos.
    """
    # LIMPIA la colección (solo para pruebas)
    col.delete_many({})

    # Índice para consultar por dispositivo y por tiempo
    col.create_index([("device_id", 1), ("timestamp", -1)])

    now = datetime.now(timezone.utc)
    devices = ["esp32_A", "esp32_B"]

    # 10 minutos de datos, 1 muestra por segundo
    total_segundos = 10 * 60

    documentos = []
    for device in devices:
        ts = now - timedelta(seconds=total_segundos)
        for _ in range(total_segundos):
            documentos.append(generar_medicion(device, ts))
            ts += timedelta(seconds=1)

    if documentos:
        col.insert_many(documentos)

    print(f"Insertadas {len(documentos)} mediciones de prueba.")


if __name__ == "__main__":
    seed_datos()
