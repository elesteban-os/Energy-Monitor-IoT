FROM python:3.11-slim

# Directorio de trabajo en el contenedor
WORKDIR /app

# Instalamos dependencias primero para aprovechar caché
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiamos el código de la API
COPY . .

# Puerto expuesto por la API
EXPOSE 8000

# Comando de arranque
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]
