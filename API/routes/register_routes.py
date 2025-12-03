from fastapi import APIRouter, HTTPException, status
from models.device_model import DeviceRegister, register_device_logic

router = APIRouter(tags=["Registro"])

@router.post("/register", status_code=status.HTTP_201_CREATED)
def register_device(device: DeviceRegister):
    """
    Endpoint para que el ESP32 se registre por primera vez. 
    Recibe el serial y devuelve un token de autenticación.
    """
    try:
        token, status_type = register_device_logic(device)
        
        response_code = status.HTTP_201_CREATED if status_type == "new" else status.HTTP_200_OK
        message = "Device registered successfully" if status_type == "new" else "Device already registered"
        
        return {
            "message": message,
            "token": token
        }
        
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Error interno del servidor: {e}"
        )