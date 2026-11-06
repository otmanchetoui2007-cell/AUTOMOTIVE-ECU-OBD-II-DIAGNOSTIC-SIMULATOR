
#ifndef OBD2_H
#define OBD2_H
#include <stdbool.h>
#include <stdint.h>
#include "engine_model.h"

#include <stdint.h>
/* ==========================================
   1. IDENTIFIANTS CAN OBD-II
   ========================================== */
/* Identifiant de requête fonctionnelle */
#define OBD2_CAN_REQUEST_ID       0x7DF
/* Identifiant de réponse de notre ECU moteur */
#define OBD2_CAN_RESPONSE_ID      0x7E8
/* ========================================== */
/* 2. MODES OBD-II */
/* ========================================== */
/* Mode 01 : données actuelles */
#define OBD2_MODE_CURRENT_DATA    0x01
/* Mode 02 : données enregistrées au moment
   d'un défaut */
#define OBD2_MODE_FREEZE_FRAME    0x02
/* Mode 03 : lecture des codes défauts */
#define OBD2_MODE_READ_DTC        0x03
/* Mode 04 : effacement des défauts */
#define OBD2_MODE_CLEAR_DTC       0x04
/* Mode 07 : défauts détectés pendant le
   cycle de conduite actuel */
#define OBD2_MODE_PENDING_DTC     0x07
/* Mode 09 : informations du véhicule */
#define OBD2_MODE_VEHICLE_INFO    0x09
/* Mode 0A : défauts permanents */
#define OBD2_MODE_PERMANENT_DTC   0x0A
/* ==========================================
   3. PID DU MODE 01
   ========================================== */
/* PID 05 : température du liquide de refroidissement */
#define OBD2_PID_COOLANT_TEMP     0x05
/* PID 0C : régime moteur */
#define OBD2_PID_ENGINE_RPM       0x0C
/* PID 0D : vitesse du véhicule */
#define OBD2_PID_VEHICLE_SPEED    0x0D
/* PID 11 : position de l'accélérateur */
#define OBD2_PID_THROTTLE         0x11
/* PID 42 : tension du module de commande */
#define OBD2_PID_CONTROL_VOLTAGE  0x42
/* ==========================================
   4. PARAMETRES CAN ET ISO-TP
   ========================================== */
/* Taille maximale des données CAN classique */
#define OBD2_CAN_MAX_DATA_LENGTH  8
/* Longueur minimale d'une trame CAN classique */
#define OBD2_CAN_MIN_DATA_LENGTH  0
/* Type de trame ISO-TP : Single Frame */
#define OBD2_ISOTP_SINGLE_FRAME   0x00
/* Réponse positive : mode demandé + 0x40 */
#define OBD2_POSITIVE_RESPONSE    0x40
/* Réponse négative ISO 14229 (UDS) */
#define OBD2_NEGATIVE_RESPONSE    0x7F
/*
 * Structure 1 : requête OBD-II brute
 */
typedef struct{
    uint32_t can_id;       // Identifiant de la trame CAN
    uint8_t dlc;           // Nombre d'octets dans la trame
    uint8_t data[OBD2_CAN_MAX_DATA_LENGTH]; // Données reçues
} OBD2_Request;
/*
 * Structure 2 : requête OBD-II décodée
 */
typedef struct{
    uint8_t mode;          // Mode OBD-II demandé
    uint8_t pid;           // Paramètre demandé
} OBD2_DecodedRequest;
/*
 * Structure 3 : réponse OBD-II
 */
typedef struct{
    uint32_t can_id;       // Identifiant CAN de réponse
    uint8_t dlc;           // Nombre d'octets à transmettre
    uint8_t data[OBD2_CAN_MAX_DATA_LENGTH]; // Données de réponse
} OBD2_Response;

/* 1. Vérifier la validité de la requête */
bool obd2_validate_request(const OBD2_Request *request);

/* 2. Extraire le mode et le PID */
bool obd2_decode_request(
    const OBD2_Request *request,
    OBD2_DecodedRequest *decoded
);

/* 3. Vérifier si le mode est supporté */
bool obd2_is_mode_supported(uint8_t mode);

/* 4. Vérifier si le PID est supporté */
bool obd2_is_pid_supported(uint8_t mode, uint8_t pid);

/* 5. Récupérer et encoder les données du PID */
bool obd2_encode_pid_data(
    const Engine_model *engine,
    uint8_t pid,
    uint8_t *data,
    uint8_t *data_length
);

/* 6. Construire la réponse OBD-II */
bool obd2_build_response(
    const OBD2_DecodedRequest *decoded,
    const Engine_model *engine,
    OBD2_Response *response
);

/* 7. Coordonner le traitement complet */
bool obd2_process_request(
    const OBD2_Request *request,
    const Engine_model *engine,
    OBD2_Response *response
);

#endif
