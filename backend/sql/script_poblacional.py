import os
import random
import time
from typing import Dict

import requests

# Configuracion
API_BASE = os.getenv("API_BASE", "http://localhost:8000")
TOKENS: Dict[str, str] = {
    "ESP32_A": "69046c31-ad33-4f81-af86-fb80ed183250",
    "ESP32_B": "7d0a4bfe-cfd2-4007-bf5d-3646412b1763",
}
# Asignamos un tipo de sensor a cada dispositivo
SENSOR_NAME = {
    "ESP32_A": "PZEM004T",
    "ESP32_B": "ZMPT+ACS712",
}

INTERVAL_SEC = 5      # cada 5 segundos
DURATION_SEC = 120    # durante 2 minutos


def build_payload(token: str, sensor_name: str) -> dict:
    """Genera una lectura aleatoria dentro de rangos razonables."""
    voltaje = round(random.uniform(210, 235), 2)
    corriente = round(random.uniform(0.1, 8.0), 3)
    potencia = round(voltaje * corriente, 2)
    energia = round(random.uniform(0.0, 200.0), 2)
    frecuencia = round(random.uniform(59.5, 60.5), 2)
    factor_potencia = round(random.uniform(0.85, 0.99), 2)

    return {
        "token": token,
        "sensor_name": sensor_name,
        "Voltaje": voltaje,
        "Corriente": corriente,
        "Potencia": potencia,
        "Energia": energia,
        "Frecuencia": frecuencia,
        "Factor Potencia": factor_potencia,
    }


def send_reading(payload: dict) -> None:
    """Envía una lectura al endpoint /readings."""
    url = f"{API_BASE}/readings"
    try:
        resp = requests.post(url, json=payload, timeout=5)
        status = resp.status_code
        print(f"[{payload.get('sensor_name')}] {status} -> {resp.text.strip()}")
    except Exception as exc:
        print(f"[{payload.get('sensor_name')}] Error de envío: {exc}")


def main():
    iterations = DURATION_SEC // INTERVAL_SEC
    print(f"Enviando {iterations} lecturas por dispositivo (cada {INTERVAL_SEC}s) a {API_BASE}")
    for _ in range(iterations):
        for serial, token in TOKENS.items():
            sensor = SENSOR_NAME.get(serial, "PZEM004T")
            payload = build_payload(token, sensor)
            send_reading(payload)
        time.sleep(INTERVAL_SEC)
    print("Finalizado.")


if __name__ == "__main__":
    main()
