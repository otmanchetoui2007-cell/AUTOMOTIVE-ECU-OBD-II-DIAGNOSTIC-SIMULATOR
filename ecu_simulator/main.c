
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <linux/can.h>

#include "Can_interface.h"
#include "Engine_model.h"
#include "obd2.h"

/* Configuration */
#define CAN_INTERFACE_NAME "vcan0"
#define CAN_RECEIVE_TIMEOUT 100

/* Contrôle de l'arrêt du programme */
static volatile sig_atomic_t running = 1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

/* Calcul du temps écoulé en secondes */
static float calculate_dt(struct timespec *previous,
                          struct timespec *current)
{
    float dt;

    dt = (float)(current->tv_sec - previous->tv_sec);

    dt += (float)(current->tv_nsec - previous->tv_nsec)
          / 1000000000.0f;

    return dt;
}

int main(void)
{
    /* =====================================
       1. DECLARATION DES STRUCTURES
       ===================================== */

    CanInterface can;
    Engine_model engine;

    struct can_frame rx_frame;
    struct can_frame tx_frame;

    OBD2_Request request;
    OBD2_Response response;

    int rx_status;
    int tx_status;

    struct timespec previous_time;
    struct timespec current_time;

    float dt;

    /* =====================================
       2. CONFIGURATION DU SIGNAL CTRL+C
       ===================================== */

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    /* =====================================
       3. INITIALISATION DU MODELE MOTEUR
       ===================================== */

    engine_init(&engine);

    if (!engine_validate(&engine))
    {
        fprintf(stderr, "Erreur : modele moteur invalide.\n");
        return 1;
    }

    printf("Modele moteur initialise.\n");

    /* =====================================
       4. INITIALISATION DE L'INTERFACE CAN
       ===================================== */

    if (can_init(&can, CAN_INTERFACE_NAME) < 0)
    {
        perror("Erreur initialisation CAN");
        return 1;
    }

    printf("Interface CAN %s initialisee.\n",
           CAN_INTERFACE_NAME);

    /* Initialisation de l'horloge */
    if (clock_gettime(CLOCK_MONOTONIC, &previous_time) < 0)
    {
        perror("Erreur clock_gettime");
        can_close(&can);
        return 1;
    }

    printf("\nECU moteur demarre.\n");
    printf("En attente des requetes OBD-II...\n\n");

    /* =====================================
       5. BOUCLE PRINCIPALE DE L'ECU
       ===================================== */

    while (running)
    {
        /* ---------------------------------
           5.1 Reception CAN
           --------------------------------- */

        memset(&rx_frame, 0, sizeof(rx_frame));

        rx_status = can_receive(
            &can,
            &rx_frame,
            CAN_RECEIVE_TIMEOUT
        );

        if (rx_status < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("Erreur reception CAN");
            break;
        }

        /* ---------------------------------
           5.2 Mise a jour du modele moteur
           --------------------------------- */

        if (clock_gettime(CLOCK_MONOTONIC, &current_time) < 0)
        {
            perror("Erreur horloge");
            break;
        }

        dt = calculate_dt(&previous_time, &current_time);
        previous_time = current_time;

        if (dt > 0.0f)
        {
            engine_update(&engine, dt);
        }

        if (!engine_validate(&engine))
        {
            fprintf(stderr, "Erreur : etat moteur invalide.\n");
            continue;
        }

        /* 0 signifie qu'aucune trame n'a ete recue (timeout).
           On continue quand meme la simulation moteur. */
        if (rx_status == 0)
        {
            continue;
        }

        /* ---------------------------------
           5.3 Verification de l'identifiant CAN
           --------------------------------- */

        /* Ignorer les trames etendues, RTR et erreurs */
        if ((rx_frame.can_id & CAN_EFF_FLAG) ||
            (rx_frame.can_id & CAN_RTR_FLAG) ||
            (rx_frame.can_id & CAN_ERR_FLAG))
        {
            continue;
        }

        /* Accepter uniquement les requetes OBD-II */
        if ((rx_frame.can_id & CAN_SFF_MASK)
             != OBD2_CAN_REQUEST_ID)
        {
            continue;
        }

        if (rx_frame.can_dlc > OBD2_CAN_MAX_DATA_LENGTH)
        {
            fprintf(stderr, "DLC CAN invalide.\n");
            continue;
        }

        printf("Requete CAN recue : ID = 0x%03X, DLC = %u\n",
               (unsigned int)rx_frame.can_id,
               (unsigned int)rx_frame.can_dlc);

        /* ---------------------------------
           5.4 Conversion CAN vers OBD2_Request
           --------------------------------- */

        memset(&request, 0, sizeof(request));

        request.can_id = rx_frame.can_id & CAN_SFF_MASK;
        request.dlc = rx_frame.can_dlc;

        memcpy(
            request.data,
            rx_frame.data,
            request.dlc
        );

        /* ---------------------------------
           5.5 Traitement OBD-II
           --------------------------------- */

        memset(&response, 0, sizeof(response));

        if (!obd2_process_request(
                &request,
                &engine,
                &response))
        {
            printf("Requete OBD-II non supportee ou invalide.\n\n");
            continue;
        }

        /* ---------------------------------
           5.6 Conversion OBD2_Response vers CAN
           --------------------------------- */

        if (response.dlc > CAN_MAX_DLEN)
        {
            fprintf(stderr, "DLC de reponse invalide.\n");
            continue;
        }

        memset(&tx_frame, 0, sizeof(tx_frame));

        tx_frame.can_id = response.can_id;
        tx_frame.can_dlc = response.dlc;

        memcpy(
            tx_frame.data,
            response.data,
            response.dlc
        );

        /* ---------------------------------
           5.7 Envoi de la reponse CAN
           --------------------------------- */

        tx_status = can_send(&can, &tx_frame);

        if (tx_status < 0)
        {
            perror("Erreur envoi CAN");
            continue;
        }

        printf("Reponse OBD-II envoyee : ID = 0x%03X\n",
               (unsigned int)tx_frame.can_id);

        printf("DATA : ");

        for (uint8_t i = 0; i < tx_frame.can_dlc; i++)
        {
            printf("%02X ", tx_frame.data[i]);
        }

        printf("\n\n");
    }

    /* =====================================
       6. FERMETURE DE L'INTERFACE CAN
       ===================================== */

    printf("\nArret de l'ECU...\n");

    if (can_close(&can) < 0)
    {
        perror("Erreur fermeture CAN");
        return 1;
    }

    printf("Interface CAN fermee.\n");
    printf("ECU moteur arrete proprement.\n");

    return 0;
}

