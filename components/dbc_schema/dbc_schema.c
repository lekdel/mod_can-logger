#include "dbc_schema.h"

const char *g_dbc_name = "belo";

const dbc_signal_desc_t g_dbc_signals[] = {
    {"etat_charge", "", 0x301, 0, 8, 1, 0, 1.0f, 0.0f},
    {"soc", "", 0x301, 8, 8, 1, 0, 1.0f, 0.0f},
    {"temperature_drive", "C", 0x302, 56, 8, 1, 1, 1.0f, 0.0f},
    {"id_citerne", "", 0x302, 15, 16, 0, 0, 1.0f, 0.0f},
    {"temperature_moteur", "C", 0x302, 48, 8, 1, 1, 1.0f, 0.0f},
    {"temperature_liquide", "", 0x302, 40, 8, 1, 1, 1.0f, 0.0f},
    {"temperature_batterie", "", 0x302, 32, 8, 1, 1, 1.0f, 0.0f},
    {"type_liquide", "", 0x302, 0, 8, 1, 0, 1.0f, 0.0f},
    {"etat_systeme", "", 0x300, 56, 8, 1, 0, 1.0f, 0.0f},
    {"consigne_vitesse_moteur", "RPM", 0x300, 8, 8, 1, 0, 50.0f, 0.0f},
    {"courant_batterie", "A", 0x300, 48, 8, 1, 0, 1.0f, 0.0f},
    {"tension_batterie", "V", 0x300, 40, 8, 1, 0, 1.0f, 0.0f},
    {"pression", "PSI", 0x300, 24, 8, 1, 0, 1.0f, 0.0f},
    {"vitesse_moteur", "RPM", 0x300, 16, 8, 1, 0, 50.0f, 0.0f},
    {"temps_ecoule_dechargement", "min", 0x300, 0, 8, 1, 0, 1.0f, 0.0f},
};

const size_t g_dbc_signal_count = sizeof(g_dbc_signals) / sizeof(g_dbc_signals[0]);
