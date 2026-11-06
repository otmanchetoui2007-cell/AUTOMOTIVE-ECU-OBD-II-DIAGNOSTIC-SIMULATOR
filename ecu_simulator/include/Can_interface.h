#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <stdint.h>

/* Linux provides this type. Keep the public header usable by tools that do
    not have the Linux CAN headers installed (for example, on Windows). */
#if defined(__linux__)
#  include <linux/can.h>
#else
#  define CAN_INTERFACE_NO_LINUX_CAN_H
#endif

#ifdef CAN_INTERFACE_NO_LINUX_CAN_H
#  ifndef CAN_MAX_DLEN
#    define CAN_MAX_DLEN 8
#  endif
struct can_frame {
    uint32_t can_id;
    uint8_t can_dlc;
    uint8_t __pad;
    uint8_t __res0;
    uint8_t __res1;
    uint8_t data[CAN_MAX_DLEN];
};
#endif

/* Structure qui représente notre connexion CAN */
/* int : retourne 0 si l'initialisation réussit et -1 en cas d'erreur.
CanInterface *can : pointeur vers la structure que la fonction va initialiser.
const char *ifname : nom de l'interface CAN, ici "vcan0". const signifie que la fonction ne doit pas modifier cette chaîne.
*/
typedef struct{
    int socket_fd;
} CanInterface;

/* Initialise la connexion à une interface CAN */
int can_init(CanInterface *can, const char *ifname);

/* Recevoir une trame CAN avec un timeout */
int can_receive(CanInterface *can,
                struct can_frame *frame,
                int timeout_ms);

/* Envoyer une trame CAN */
int can_send(CanInterface *can,
             const struct can_frame *frame);

/* Ferme proprement la socket CAN.
   Retourne 0 si elle est fermée ou déjà fermée,
   -1 si une erreur survient. */
int can_close(CanInterface *can);
#endif
