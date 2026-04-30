// ATTENTION: Ce fichier est un exemple.
// Copier vers config.h et y mettre vos vraies valeurs.

#ifndef MOD_CAN_LOGGER_CONFIG_EXEMPLE_H
#define MOD_CAN_LOGGER_CONFIG_EXEMPLE_H

// WiFi STA (reseau auquel l'ESP32 se connecte)
#define WIFI_STA_SSID       "NOM_DU_WIFI"
#define WIFI_STA_PASSWD     "MOT_DE_PASSE_WIFI"

// API HTTP locale sur l'ESP32
#define API_HTTP_PORT       80

// CAN bitrate en kbps (valeurs supportees: 125, 250, 500, 1000)
#define CAN_BITRATE_KBPS    250
// Pins TWAI CAN1 (doc ESP32-CAN-X2): RX=IO6, TX=IO7
#define CAN_TX_GPIO         GPIO_NUM_7
#define CAN_RX_GPIO         GPIO_NUM_6


#endif
