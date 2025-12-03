from fastapi import FastAPI
from dotenv import load_dotenv
import uvicorn
import os
import sys
from fastapi.middleware.cors import CORSMiddleware

# Asegura que los imports funcionen al ejecutar 
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

#carga lo que esta en el .env
load_dotenv()

from config import USE_MOCK_DB

# Importar routers
from routes.register_routes import router as register_router
from routes.measurement_routes import router as measurement_router


# inicialización de servicios (MySQL,Mongo,Mock)

def startup_services():
    """
    Esta función SOLO imprime el estado actual del sistema.
    No intenta conectar a MySQL/Mongo en modo MOCK.
    """
    print("============================================")
    print("ENERGY API")
    print("============================================")

    if USE_MOCK_DB:
        print("MODO MOCK ACTIVADO")
    else:
        print("MODO REAL ACTIVADO")
        print("Conexiones a MySQL y MongoDB habilitadas.")

    print("============================================\n")


# se crea la API 

app = FastAPI(
    title="Energy Monitoring API",
    description="Servicio para registro de ESP32 y almacenamiento de mediciones..",
    version="1.0.0"
)


# Hook de inicio del servidor
@app.on_event("startup")
def on_startup():
    startup_services()


# Registrar rutas
app.include_router(register_router)
app.include_router(measurement_router)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Endpoint raíz para verificar estado
@app.get("/", tags=["Estado"])
def root():
    return {"message": "Energy API running", "mock_mode": USE_MOCK_DB}


# Ejecutar servidor
if __name__ == "__main__":
    uvicorn.run(
        app, 
        host="0.0.0.0",
        port=8000,
    )
