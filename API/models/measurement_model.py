from pydantic import BaseModel, Field
from database.mongo_db import get_mongo_collection
from config import USE_MOCK_DB
import datetime
import sys
from models.mock_memory import mock_devices

# Modelos Pydantic (Entrada de Datos)

class Measurement(BaseModel):
    
    token: str = Field(..., description="Token de autenticación del dispositivo.")
    
    # rangos para la validación automática 
    voltage: float = Field(..., ge=80.0, le=260.0, description="Voltaje (V)")
    current: float = Field(..., ge=0.0, le=100.0, description="Corriente (A)")
    act_power: float = Field(..., ge=0.0, description="Potencia activa (kW)")
    fact_power: float = Field(..., ge=0.0, le=1.0, description="Factor de potencia")
    freq: float = Field(..., ge=45.0, le=65.0, description="Frecuencia (Hz)")
    act_energy: float = Field(..., ge=0.0, description="Energía acumulada (kWh)")
    alarm: bool = Field(..., description="Alarma de sobrecarga")

#Lógica de MongoDB

def save_measurement_logic(device_info: dict, data: Measurement):
    # La información del dispositivo viene de MySQL, el resto de Pydantic.
    # Convertimos el objeto Pydantic a diccionario, excluyendo el token
    measurement_doc = data.model_dump(exclude={'token'})
    # Añadimos metadata esencial para el análisis en Grafana
    document = {
        "device_serial": device_info['serial'],
        "timestamp": datetime.datetime.utcnow(),
        **measurement_doc
    }

    #MODO MOCk
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