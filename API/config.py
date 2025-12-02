import os
from dotenv import load_dotenv

# Cargamos las variables del archivo .env
load_dotenv()

#modo mock para hacer pruebas 
USE_MOCK_DB = os.getenv("USE_MOCK_DB", "True").lower() in ('true', '1', 't') 

# Variables para configuración de MySQL 
MYSQL_HOST = os.getenv("MYSQL_HOST", "localhost")
MYSQL_USER = os.getenv("MYSQL_USER", "root")
MYSQL_PASSWORD = os.getenv("MYSQL_PASSWORD", "")
MYSQL_DATABASE = os.getenv("MYSQL_DATABASE", "energy_devices")

# Variables de configuración para MongoDB
MONGO_URI = os.getenv("MONGO_URI", "mongodb://localhost:27017")
MONGO_DATABASE = os.getenv("MONGO_DATABASE", "energy_data")
