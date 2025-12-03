# Manual de despliegue y pruebas

## 1. Requisitos

- Docker y Docker Compose.
- Python 3.11 (para scripts).
- Arduino IDE (para cargar sketches ESP32).
- Acceso a MongoDB Atlas (URI).

## 2. Variables de entorno (API/.env)

Ejemplo:

```
USE_MOCK_DB=False
MYSQL_HOST=mysql
MYSQL_USER=energy_user
MYSQL_ROOT_PASSWORD=CEbasesSuperPasswor
MYSQL_DATABASE=energy_devices
MONGO_URI=mongodb+srv://<user>:<pass>@cluster/MonitoreoEnergetico?appName=Cluster0
MONGO_DATABASE=MonitoreoEnergetico
COLLECTION_NAME=measurements
```

## 3. Dependencias (scripts)

```bash
python -m pip install -r backend/requirements.txt
```

## 4. Levantar contenedores

Desde la raíz:

```bash
docker-compose --env-file API/.env up -d
```

Servicios: `api` (http://localhost:8000) y `mysql` (host 3307).

## 5. Esquema MySQL

```bash
docker exec -i mysql-energy mysql -h localhost -uroot -p<clave> < backend/sql/schema_mysql.sql
```

Listar devices:

```bash
docker exec -it mysql-energy mysql -h localhost -uroot -p<clave> -e "USE energy_devices; SELECT * FROM devices;"
```

## 6. Pruebas API

- Registro: `POST http://localhost:8000/register`

```json
{ "serial": "ESP32_TEST", "location": "LAB", "type": "energy_meter" }
```

- Lectura: `POST http://localhost:8000/readings`

```json
{
  "token": "<TOKEN>",
  "sensor_name": "PZEM004T",
  "Voltaje": 213.1,
  "Corriente": 0.11,
  "Potencia": 3580.08,
  "Energia": 106.5,
  "Frecuencia": 60.0,
  "Factor Potencia": 0.96
}
```

Swagger: http://localhost:8000/docs

## 7. ESP32 (sketches)

Archivos:

- `sensors_read/pzem/pzem_data/pzem_data.ino` (PZEM004T)
- `sensors_read/ind/sensor_read/sensor_read1/sensor_read1.ino` (ZMPT101B + ACS712)

Configurar en cada sketch:

- `WIFI_SSID`, `WIFI_PASS`
- `API_BASE` (ej. `http://<IP_PC>:8000`; no usar localhost)
- `SERIAL_ID` fijo `"ESP32"`
- `sensor_name`: ya definido en cada sketch

Flujo en el ESP32:

1. Conecta WiFi.
2. Si no hay token en NVS: `POST /register`, guarda token.
3. Cada ~5 s: `POST /readings` (simula si no hay sensor real).
4. Si 403: borra token y registra de nuevo.

## 8. Script poblacional (2 dispositivos)

Archivo: `backend/sql/script_poblacional.py` (usa tokens fijos `ESP32_A` y `ESP32_B`).
Ejecutar:

```bash
cd backend/sql
python script_poblacional.py
```

Envía lecturas cada 5 s por 2 min a `API_BASE` (configurable por env).

## 9. Grafana

- Conectar datasource a MongoDB Atlas (`MonitoreoEnergetico`, colección `measurements`).
- Crear paneles comparando `device_id` entre dispositivos.

## 10. Problemas comunes

- No abre `:8000` desde otra máquina: abrir puerto 8000 en firewall de Windows; usar IP LAN (no localhost) en `API_BASE`.
- Token inválido (403): volver a registrar; borrar token en NVS o limpiar tabla `devices` (TRUNCATE).
- Puertos/USB en Arduino: instalar driver CP210x/CH340, placa `ESP32 Dev Module`, seleccionar COM correcto.
- Lecturas en 0 o NaN: verificar cableado y calibración; eliminar simulación si se requiere fallo explícito.

## Tecnologías utilizadas

- FastAPI + Uvicorn (API)
- Python 3.11
- MySQL 8 (Docker)
- MongoDB Atlas
- Docker / Docker Compose
- Arduino/ESP32 (WiFi + HTTPClient)
- Grafana (dashboards sobre MongoDB)
