import pymysql
from config import MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD, MYSQL_DATABASE, USE_MOCK_DB
from unittest.mock import MagicMock
import sys
import os

# Configuramos un token y dispositivo fijo para el modo Mock
MOCK_DEVICE_INFO = {
    "serial": "ESP32_MOCK_01",
    "active": True
}
MOCK_TOKEN = "e87f1b2c-6a7e-40f4-8a9d-5b6c7d8e9f0a"

def get_mysql_connection():
    #MODO MOCK
    if USE_MOCK_DB:
        print("Conexión simulada a MySQL.")
        # Simula un objeto de conexión que se puede usar, pero no hace nada real
        mock_conn = MagicMock() 
        
        # Simulamos que la consulta de token devuelve info del mock device
        return mock_conn

    #MODO REAL
    try:
        connection = pymysql.connect(
            host=MYSQL_HOST,
            user=MYSQL_USER,
            password=MYSQL_PASSWORD,
            database=MYSQL_DATABASE,
            cursorclass=pymysql.cursors.DictCursor
        )
        return connection
    except Exception as e:
        print(f"Falló la conexión a MySQL: {e}", file=sys.stderr)
        #la API debe parar si no puede conectar
        os._exit(1)