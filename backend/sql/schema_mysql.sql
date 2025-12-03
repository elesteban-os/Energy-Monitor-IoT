-- Crea la base y la tabla de dispositivos para la API
CREATE DATABASE IF NOT EXISTS energy_devices;
USE energy_devices;

-- Tabla de dispositivos registrados
CREATE TABLE IF NOT EXISTS devices (
  id INT AUTO_INCREMENT PRIMARY KEY,
  serial VARCHAR(100) NOT NULL UNIQUE,
  token VARCHAR(200) NOT NULL,
  active BOOLEAN NOT NULL DEFAULT TRUE,
  location VARCHAR(100) DEFAULT 'unknown',
  type VARCHAR(50) DEFAULT 'energy_meter',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
