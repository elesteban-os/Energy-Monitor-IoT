from fastapi import APIRouter, HTTPException, status
from models.measurement_model import Measurement, save_measurement_logic
from models.device_model import get_device_info_by_token

router = APIRouter(tags=["Ingesta de Datos"])

@router.post("/readings", status_code=status.HTTP_201_CREATED)
def receive_measurement(data: Measurement):
    """
    Endpoint POST para recibir los datos de energía. 
    Valida token, verifica estado y guarda en MongoDB.
    """
    token = data.token
    
    #Autenticación y chequeo de estado (La lógica esta en models/device_model.py)
    try:
        device_info = get_device_info_by_token(token)
    except Exception:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Fallo al consultar el servicio de autenticación."
        )

    if not device_info:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Token inválido o dispositivo no registrado.")

    if not device_info['active']:
        # Verificar que un dispositivo esté activo.
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Dispositivo inactivo. Lectura rechazada.")

    # Guardado de la lectura (la lógica esta en models/measurement_model.py)
    try:
        save_measurement_logic(device_info, data)
        return {"message": "Data stored successfully"}
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Fallo al guardar la lectura: {e}"
        )