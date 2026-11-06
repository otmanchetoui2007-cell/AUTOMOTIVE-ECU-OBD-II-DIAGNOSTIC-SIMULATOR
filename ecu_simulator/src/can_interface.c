
#include "can_interface.h"

#if defined(__linux__)
#include <sys/poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if defined(__has_include)
#  if __has_include(<sys/socket.h>)
#    include <sys/socket.h>
#  endif
#else
#  include <sys/socket.h>
#endif
#if defined(__has_include)
#  if __has_include(<sys/ioctl.h>)
#    include <sys/ioctl.h>
#  endif
#else
#  include <sys/ioctl.h>
#endif
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif

#if defined(__linux__)
/* Initialise une connexion SocketCAN */
int can_init(CanInterface *can, const char *ifname)
{
    struct ifreq ifr;
    struct sockaddr_can addr;

    /* 1. Vérifier les arguments */
    if (can == NULL || ifname == NULL || ifname[0] == '\0')
    {
        fprintf(stderr, "Erreur : arguments invalides\n");
        return -1;
    }

    /* 2. Initialiser l'état de la structure */
    can->socket_fd = -1;

    /* 3. Vérifier la longueur du nom de l'interface */
    if (strlen(ifname) >= IFNAMSIZ)
    {
        fprintf(stderr, "Erreur : nom d'interface trop long\n");
        return -1;
    }

    /* 4. Créer une socket CAN brute */
    can->socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (can->socket_fd < 0)
    {
        perror("Erreur socket");
        return -1;
    }

    /* 5. Préparer la recherche de l'interface réseau */
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

    /* 6. Obtenir l'index de l'interface */
    if (ioctl(can->socket_fd, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("Erreur ioctl - interface introuvable");
        close(can->socket_fd);
        can->socket_fd = -1;
        return -1;
    }

    /* 7. Préparer l'adresse SocketCAN */
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    /* 8. Associer la socket à l'interface CAN */
    if (bind(can->socket_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        perror("Erreur bind");
        close(can->socket_fd);
        can->socket_fd = -1;
        return -1;
    }

    /* 9. Initialisation réussie */
    printf("SocketCAN connectee a %s\n", ifname);

    return 0;
}

int can_receive(CanInterface *can,
                struct can_frame *frame,
                int timeout_ms)
{
    struct pollfd pfd;
    ssize_t nbytes;
    int ret;

    /* 1. Verifier les arguments */
    if (can == NULL || frame == NULL ||
        can->socket_fd < 0 || timeout_ms < -1)
    {
        fprintf(stderr, "Erreur : arguments invalides\n");
        return -1;
    }

    /* 2. Configurer l'attente sur la socket */
    pfd.fd = can->socket_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /* 3. Attendre une trame ou un timeout */
    do
    {
        ret = poll(&pfd, 1, timeout_ms);
    }
    while (ret < 0 && errno == EINTR);

    if (ret == 0)
    {
        return 0;  /* Timeout */
    }

    if (ret < 0)
    {
        perror("Erreur poll");
        return -1;
    }

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        fprintf(stderr, "Erreur sur la socket CAN\n");
        return -1;
    }

    /* 4. Lire la trame depuis la socket */
    nbytes = read(can->socket_fd, frame, sizeof(*frame));

    if (nbytes < 0)
    {
        if (errno == EINTR || errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            return 0;
        }

        perror("Erreur read CAN");
        return -1;
    }

    /* 5. Verifier la taille de la trame */
    if ((size_t)nbytes != sizeof(*frame))
    {
        fprintf(stderr, "Erreur : taille de trame CAN invalide\n");
        return -1;
    }

    /* 6. Verifier la longueur des donnees */
    if (frame->can_dlc > CAN_MAX_DLEN)
    {
        fprintf(stderr, "Erreur : DLC CAN invalide\n");
        return -1;
    }

    /* 7. Une trame a ete recue */
    return 1;
}


int can_send(CanInterface *can,
             const struct can_frame *frame)
{
    ssize_t nbytes;

    /* 1. Verifier les arguments */
    if (can == NULL || frame == NULL ||
        can->socket_fd < 0)
    {
        fprintf(stderr, "Erreur : arguments invalides\n");
        return -1;
    }

    /* 2. Verifier la longueur des donnees */
    if (frame->can_dlc > CAN_MAX_DLEN)
    {
        fprintf(stderr, "Erreur : DLC CAN invalide\n");
        return -1;
    }

    /* 3. Verifier l'identifiant CAN */
    if (frame->can_id & CAN_ERR_FLAG)
    {
        fprintf(stderr,
                "Erreur : trame d'erreur CAN non autorisee ici\n");
        return -1;
    }

    if (frame->can_id & CAN_EFF_FLAG)
    {
        /* Identifiant etendu sur 29 bits */
        if ((frame->can_id & CAN_EFF_MASK) > CAN_EFF_MASK)
        {
            fprintf(stderr, "Erreur : identifiant CAN invalide\n");
            return -1;
        }
    }
    else
    {
        /* Identifiant standard sur 11 bits */
        if ((frame->can_id & CAN_SFF_MASK) > CAN_SFF_MASK)
        {
            fprintf(stderr, "Erreur : identifiant CAN invalide\n");
            return -1;
        }
    }

    /* 4. Envoyer la trame */
    do
    {
        nbytes = write(can->socket_fd,
                       frame,
                       sizeof(*frame));
    }
    while (nbytes < 0 && errno == EINTR);

    /* 5. Verifier le resultat */
    if (nbytes < 0)
    {
        perror("Erreur write CAN");
        return -1;
    }

    if ((size_t)nbytes != sizeof(*frame))
    {
        fprintf(stderr,
                "Erreur : taille de trame envoyee incorrecte\n");
        return -1;
    }

    /* 6. L'envoi a ete accepte par SocketCAN */
    return 0;
}


int can_close(CanInterface *can)
{
    int fd;

    /* 1. Vérifier que le pointeur est valide */
    if (can == NULL)
    {
        fprintf(stderr, "Erreur : pointeur CanInterface invalide\n");
        return -1;
    }

    /* 2. Vérifier si la socket est déjà fermée */
    if (can->socket_fd < 0)
    {
        return 0;
    }

    /* 3. Sauvegarder le descripteur */
    fd = can->socket_fd;

    /* 4. Marquer la socket comme fermée dans la structure */
    can->socket_fd = -1;

    /* 5. Fermer la socket au niveau du système */
    if (close(fd) < 0)
    {
        perror("Erreur lors de la fermeture de la socket CAN");
        return -1;
    }

    /* 6. Confirmer la fermeture */
    printf("Socket CAN fermee proprement\n");

    return 0;
}
#else

int can_init(CanInterface *can, const char *ifname)
{
    (void)can;
    (void)ifname;
    return -1;
}

int can_receive(CanInterface *can,
                struct can_frame *frame,
                int timeout_ms)
{
    (void)can;
    (void)frame;
    (void)timeout_ms;
    return -1;
}

int can_send(CanInterface *can,
             const struct can_frame *frame)
{
    (void)can;
    (void)frame;
    return -1;
}

int can_close(CanInterface *can)
{
    (void)can;
    return -1;
}
#endif
