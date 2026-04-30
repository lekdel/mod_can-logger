# mod_can-logger

Ce repo sert d'endpoint API sur ESP32 pour exposer l'information du CAN bus auquel il est relié.
Il fournit les donnees decodees (via DBC) par HTTP, et inclut un dashboard web pour voir ces donnees en temps reel.

Projet ESP-IDF pour ESP32-S3:
- lecture CAN (TWAI)
- decode des trames via schema genere depuis DBC
- API HTTP locale (endpoint ESP32)

## Configuration

Le projet utilise `config.h`.

1. Copier `config_exemple.h` vers `config.h`
2. Modifier:
- `WIFI_STA_SSID`
- `WIFI_STA_PASSWD`
- `CAN_BITRATE_KBPS` (125/250/500/1000)

## Changer de DBC (sans toucher l'API/CAN app)

1. Remplacer le fichier DBC source:
- `dbc/*.dbc`

2. Regenerer le schema C:
```bash
cd mod_can-logger
python3 ./tools/gen_dbc_schema.py
```

3. Rebuild/flash:
```bash
. $HOME/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## Endpoints

- `GET /` -> dashboard live des signaux
- `GET /api/health`
- `GET /api/meta` -> nom DBC + liste des signaux
- `GET /api/can/latest` -> dernier snapshot des valeurs decodees

## Acces reseau

- par IP DHCP: `http://<ip_esp32>/`
