
#include "../include/obd2.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* =====================================================
   1. VALIDATION DE LA REQUETE OBD-II
   ===================================================== */

bool obd2_validate_request(const OBD2_Request *request)
{
    if (request == NULL)
    {
        return false;
    }

    /* Vérifier l'identifiant CAN */
    if (request->can_id != OBD2_CAN_REQUEST_ID)
    {
        return false;
    }

    /* Vérifier le DLC */
    if (request->dlc < 3 ||
        request->dlc > OBD2_CAN_MAX_DATA_LENGTH)
    {
        return false;
    }

    /* Vérifier le type ISO-TP */
    uint8_t pci = request->data[0];
    uint8_t frame_type = (pci & 0xF0) >> 4;

    if (frame_type != OBD2_ISOTP_SINGLE_FRAME)
    {
        return false;
    }

    /* Extraire la longueur du payload */
    uint8_t payload_length = pci & 0x0F;

    if (payload_length != 2)
    {
        return false;
    }

    /* Vérifier que les données sont présentes */
    if (payload_length > request->dlc - 1)
    {
        return false;
    }

    return true;
}


/* =====================================================
   2. DECODAGE DE LA REQUETE
   ===================================================== */

bool obd2_decode_request(
    const OBD2_Request *request,
    OBD2_DecodedRequest *decoded)
{
    if (request == NULL || decoded == NULL)
    {
        return false;
    }

    if (!obd2_validate_request(request))
    {
        return false;
    }

    /* Octet 0 : PCI
       Octet 1 : Mode
       Octet 2 : PID */

    decoded->mode = request->data[1];
    decoded->pid = request->data[2];

    return true;
}


/* =====================================================
   3. VERIFICATION DU MODE OBD-II
   ===================================================== */

bool obd2_is_mode_supported(uint8_t mode)
{
    switch (mode)
    {
        case OBD2_MODE_CURRENT_DATA:
            return true;

        default:
            return false;
    }
}


/* =====================================================
   4. VERIFICATION DU PID
   ===================================================== */

bool obd2_is_pid_supported(uint8_t mode, uint8_t pid)
{
    if (mode != OBD2_MODE_CURRENT_DATA)
    {
        return false;
    }

    switch (pid)
    {
        case OBD2_PID_COOLANT_TEMP:
        case OBD2_PID_ENGINE_RPM:
        case OBD2_PID_VEHICLE_SPEED:
        case OBD2_PID_THROTTLE:
        case OBD2_PID_CONTROL_VOLTAGE:
            return true;

        default:
            return false;
    }
}


/* =====================================================
   5. ENCODAGE DES DONNEES DU PID
   ===================================================== */

bool obd2_encode_pid_data(
    const Engine_model *engine,
    uint8_t pid,
    uint8_t *data,
    uint8_t *data_length)
{
    uint32_t raw_value;

    if (engine == NULL ||
        data == NULL ||
        data_length == NULL)
    {
        return false;
    }

    *data_length = 0;

    switch (pid)
    {
        /* ---------------------------------
           PID 05 : Temperature moteur
           Formule : A = Temperature + 40
           --------------------------------- */

        case OBD2_PID_COOLANT_TEMP:

            if (!isfinite(engine->coolant_temp))
                return false;

            if (engine->coolant_temp < -40.0f ||
                engine->coolant_temp > 215.0f)
                return false;

            raw_value = (uint32_t)
                lroundf(engine->coolant_temp + 40.0f);

            data[0] = (uint8_t)raw_value;

            *data_length = 1;

            break;


        /* ---------------------------------
           PID 0C : Regime moteur
           Formule : RPM = (256*A + B) / 4
           --------------------------------- */

        case OBD2_PID_ENGINE_RPM:

            if (!isfinite(engine->rpm))
                return false;

            if (engine->rpm < 0.0f ||
                engine->rpm > 16383.75f)
                return false;

            raw_value = (uint32_t)
                lroundf(engine->rpm * 4.0f);

            data[0] = (uint8_t)(raw_value >> 8);
            data[1] = (uint8_t)(raw_value & 0xFF);

            *data_length = 2;

            break;


        /* ---------------------------------
           PID 0D : Vitesse vehicule
           Formule : Speed = A km/h
           --------------------------------- */

        case OBD2_PID_VEHICLE_SPEED:

            if (!isfinite(engine->vehicle_speed))
                return false;

            if (engine->vehicle_speed < 0.0f ||
                engine->vehicle_speed > 255.0f)
                return false;

            raw_value = (uint32_t)
                lroundf(engine->vehicle_speed);

            data[0] = (uint8_t)raw_value;

            *data_length = 1;

            break;


        /* ---------------------------------
           PID 11 : Position accelerateur
           Formule : A = Position * 255 / 100
           --------------------------------- */

        case OBD2_PID_THROTTLE:

            if (!isfinite(engine->throttle))
                return false;

            if (engine->throttle < 0.0f ||
                engine->throttle > 100.0f)
                return false;

            raw_value = (uint32_t)
                lroundf(engine->throttle * 255.0f / 100.0f);

            data[0] = (uint8_t)raw_value;

            *data_length = 1;

            break;


        /* ---------------------------------
           PID 42 : Tension du module
           Formule : Voltage = (256*A + B)/1000
           --------------------------------- */

        case OBD2_PID_CONTROL_VOLTAGE:

            if (!isfinite(engine->battery_voltage))
                return false;

            if (engine->battery_voltage < 0.0f ||
                engine->battery_voltage > 65.535f)
                return false;

            raw_value = (uint32_t)
                lroundf(engine->battery_voltage * 1000.0f);

            data[0] = (uint8_t)(raw_value >> 8);
            data[1] = (uint8_t)(raw_value & 0xFF);

            *data_length = 2;

            break;


        default:
            return false;
    }

    return true;
}


/* =====================================================
   6. CONSTRUCTION DE LA REPONSE OBD-II
   ===================================================== */

bool obd2_build_response(
    const OBD2_DecodedRequest *decoded,
    const Engine_model *engine,
    OBD2_Response *response)
{
    uint8_t pid_data[4] = {0};
    uint8_t pid_data_length = 0;

    if (decoded == NULL ||
        engine == NULL ||
        response == NULL)
    {
        return false;
    }

    if (!obd2_is_mode_supported(decoded->mode) ||
        !obd2_is_pid_supported(decoded->mode, decoded->pid))
    {
        return false;
    }

    if (!obd2_encode_pid_data(
            engine,
            decoded->pid,
            pid_data,
            &pid_data_length))
    {
        return false;
    }

    /* Initialiser la reponse */
    memset(response, 0, sizeof(*response));

    /* Identifiant CAN de reponse */
    response->can_id = OBD2_CAN_RESPONSE_ID;

    /*
       Structure ISO-TP Single Frame :

       Octet 0 : longueur du payload
       Octet 1 : Mode positif (Mode + 0x40)
       Octet 2 : PID
       Octets suivants : donnees du PID
    */

    uint8_t payload_length = 2 + pid_data_length;

    response->data[0] =
        OBD2_ISOTP_SINGLE_FRAME | payload_length;

    response->data[1] =
        decoded->mode + OBD2_POSITIVE_RESPONSE;

    response->data[2] = decoded->pid;

    memcpy(
        &response->data[3],
        pid_data,
        pid_data_length
    );

    /* CAN classique : trame de 8 octets */
    response->dlc = OBD2_CAN_MAX_DATA_LENGTH;

    return true;
}


/* =====================================================
   7. TRAITEMENT COMPLET DE LA REQUETE
   ===================================================== */

bool obd2_process_request(
    const OBD2_Request *request,
    const Engine_model *engine,
    OBD2_Response *response)
{
    OBD2_DecodedRequest decoded;

    if (request == NULL ||
        engine == NULL ||
        response == NULL)
    {
        return false;
    }

    /* Etape 1 : validation */
    if (!obd2_validate_request(request))
    {
        return false;
    }

    /* Etape 2 : decodage */
    if (!obd2_decode_request(request, &decoded))
    {
        return false;
    }

    /* Etape 3 : verification du Mode */
    if (!obd2_is_mode_supported(decoded.mode))
    {
        return false;
    }

    /* Etape 4 : verification du PID */
    if (!obd2_is_pid_supported(decoded.mode, decoded.pid))
    {
        return false;
    }

    /* Etape 5 : construction de la reponse */
    if (!obd2_build_response(&decoded, engine, response))
    {
        return false;
    }

    return true;
}

