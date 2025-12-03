import uuid
# Genera un token único para cada dispositivo utilizando UUID v4
def generate_token():
    return str(uuid.uuid4())
