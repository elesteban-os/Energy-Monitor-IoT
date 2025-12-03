from pydantic import BaseModel, Field
from database.mongo_db import get_mongo_collection
from config import USE_MOCK_DB
import datetime
import sys
from models.mock_memory import mock_devices

# Modelo de entrada ajustado al formato definitivo del sensor
class Measurement(BaseModel):
    token: str = Field(..., description="Token de autenticación del dispositivo.")
    sensor_name: str = Field(..., alias="sensor_name", description="Nombre del sensor (ej. PZEM004T).")
    voltaje: float = Field(..., alias="Voltaje", description="Voltaje (V)")
    corriente: float = Field(..., alias="Corriente", description="Corriente (A)")
    potencia: float = Field(..., alias="Potencia", description="Potencia (W)")
    energia: float = Field(..., alias="Energia", description="Energía acumulada (Wh)")
    frecuencia: float = Field(..., alias="Frecuencia", description="Frecuencia (Hz)")
    factor_potencia: float = Field(..., alias="Factor Potencia", description="Factor de potencia (0-1)")

    class Config:
        populate_by_name = True  # permite usar los alias al volcar el modelo

# Lógica de MongoDB

def save_measurement_logic(device_info: dict, data: Measurement):
    # Volcamos el payload con los alias originales, excluyendo el token
    measurement_doc = data.model_dump(exclude={'token'}, by_alias=True)
    # Añadimos metadata para trazabilidad
    document = {
        "device_id": device_info['serial'],
        "timestamp": datetime.datetime.utcnow(),
        **measurement_doc
    }

    # MODO MOCK
    if USE_MOCK_DB:
        print(f"Lectura mock de {device_info['serial']} almacenada (simulada).")
        return True

    # MODO REAL 
    try:
        collection = get_mongo_collection()
        collection.insert_one(document)
        return True
    except Exception as e:
        print(f"Error al guardar en MongoDB: {e}", file=sys.stderr)
        raise Exception("Fallo al insertar lectura en MongoDB.")
