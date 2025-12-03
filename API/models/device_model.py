from pydantic import BaseModel, Field
from database.mysql_db import get_mysql_connection
from utils.token_generator import generate_token
from config import USE_MOCK_DB
import sys
import uuid

# Mocks para pruebas 
mock_device ={}

#Modelos Pydantic
class DeviceRegister(BaseModel):
    serial: str = Field(..., description="Serial del ESP32")
    # Agregamos estos campos para que se guarden si se especifican
    location: str = "unknown"
    type: str = "energy_meter" 

class Device(BaseModel):
    id: int
    serial: str
    token: str
    active: bool

#Lógica de separación de responsabilidades

def register_device_logic(device_data: DeviceRegister):
    #MODO MOCK
    if USE_MOCK_DB:
        print("Ejecutando lógica de registro simulada.")
        if device_data.serial in mock_device:
            return mock_device[device_data.serial], "existe"
        #registro nuevo (genera y guarda el token)
        new_token = str(uuid.uuid4())
        mock_device[device_data.serial] = new_token
        return new_token, "nuevo"


    #MODO REAL
    connection = get_mysql_connection()
    try:
        with connection.cursor() as cursor:
            #Revisamos si el dispositivo ya existe
            cursor.execute("SELECT token FROM devices WHERE serial=%s", (device_data.serial,))
            existing = cursor.fetchone()

            if existing:
                return existing["token"], "exists"

            #Si no existe, generamos un token nuevo y registramos
            token = generate_token()
            cursor.execute(
                "INSERT INTO devices (serial, token, active, location, type) VALUES (%s, %s, %s, %s, %s)",
                (device_data.serial, token, True, device_data.location, device_data.type)
            )
            connection.commit()
            return token, "new"
    except Exception as e:
        connection.rollback()
        print(f"Error en la lógica de registro: {e}", file=sys.stderr)
        raise Exception("Fallo al registrar dispositivo en MySQL.")
    finally:
        connection.close()


def get_device_info_by_token(token: str):
    #MODO MOCK
    if USE_MOCK_DB:
        for serial, saved_token in mock_device.items():
            if saved_token == token:
                return {
                    "serial": serial,
                    "active": True,
                    "location": "LAB_SIMULADO" #cambiar estas cosas jeje
                }
        return None 

    #MODO REAL
    connection = get_mysql_connection()
    try:
        with connection.cursor() as cursor:
            cursor.execute("SELECT serial, active FROM devices WHERE token=%s", (token,))
            return cursor.fetchone()
    except Exception as e:
        print(f"Error en la búsqueda de token: {e}", file=sys.stderr)
        raise Exception("Fallo de autenticación en MySQL.")
    finally:
        connection.close()