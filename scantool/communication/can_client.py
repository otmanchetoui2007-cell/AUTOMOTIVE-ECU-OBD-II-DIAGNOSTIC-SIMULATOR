import can
import time

from config.settings import (
    CAN_INTERFACE,
    CAN_CHANNEL,
    CAN_REQUEST_ID,
    CAN_RESPONSE_ID,
    CAN_FRAME_LENGTH,
    CAN_TIMEOUT
)


class CANClient:

    def __init__(self):
        """
        Initialise l'objet CANClient.
        La connexion CAN n'est pas encore ouverte.
        """
        self.bus = None
        self.connected = False

    # --------------------------------------
    # 1. CONNEXION AU BUS CAN
    # --------------------------------------

    def connect(self):
        """
        Ouvre une connexion avec l'interface CAN.
        """

        if self.connected:
            print("CAN deja connecte.")
            return

        try:
            self.bus = can.Bus(
                interface=CAN_INTERFACE,
                channel=CAN_CHANNEL
            )

            self.connected = True

            print(f"Connexion CAN reussie : {CAN_CHANNEL}")

        except (can.CanError, OSError) as error:
            self.bus = None
            self.connected = False

            raise RuntimeError(
                f"Impossible de se connecter a {CAN_CHANNEL}: {error}"
            ) from error

    # --------------------------------------
    # 2. ENVOI D'UNE TRAME CAN
    # --------------------------------------

    def send_frame(self, can_id, data):
        """
        Envoie une trame CAN standard.

        can_id : identifiant CAN
        data   : liste ou sequence d'octets
        """

        if not self.connected or self.bus is None:
            raise RuntimeError("CAN non connecte.")

        if not isinstance(can_id, int) or not 0 <= can_id <= 0x7FF:
            raise ValueError("Identifiant CAN standard invalide.")

        if len(data) > CAN_FRAME_LENGTH:
            raise ValueError("Une trame CAN classique accepte 8 octets maximum.")

        if not all(isinstance(byte, int) and 0 <= byte <= 255 for byte in data):
            raise ValueError("Les donnees doivent etre des octets entre 0 et 255.")

        message = can.Message(
            arbitration_id=can_id,
            data=data,
            is_extended_id=False
        )

        try:
            self.bus.send(message)

            print(f"Trame CAN envoyee : ID = 0x{can_id:X}")

        except can.CanError as error:
            raise RuntimeError(
                f"Echec de l'envoi CAN : {error}"
            ) from error

    # --------------------------------------
    # 3. RECEPTION D'UNE TRAME CAN
    # --------------------------------------

    def receive_frame(self, timeout=None, expected_id=None):
        """
        Attend une trame CAN.

        timeout     : temps maximal d'attente en secondes.
        expected_id : identifiant attendu (facultatif).

        Retourne le message CAN ou None en cas de timeout.
        """

        if not self.connected or self.bus is None:
            raise RuntimeError("CAN non connecte.")

        if timeout is None:
            timeout = CAN_TIMEOUT

        if timeout < 0:
            raise ValueError("Le timeout ne peut pas etre negatif.")

        if expected_id is not None:
            if not isinstance(expected_id, int) or not 0 <= expected_id <= 0x7FF:
                raise ValueError("Identifiant CAN attendu invalide.")

        deadline = time.monotonic() + timeout

        while True:

            remaining = deadline - time.monotonic()

            if remaining <= 0:
                return None

            try:
                message = self.bus.recv(timeout=remaining)

            except can.CanError as error:
                raise RuntimeError(
                    f"Erreur pendant la reception CAN : {error}"
                ) from error

            if message is None:
                return None

            # Ignorer les trames CAN etendues
            if message.is_extended_id:
                continue

            # Ignorer les identifiants non attendus
            if expected_id is not None:
                if message.arbitration_id != expected_id:
                    continue

            print(
                f"Trame CAN recue : ID = 0x{message.arbitration_id:X}"
            )

            return message

    # --------------------------------------
    # 4. DECONNEXION DU BUS CAN
    # --------------------------------------

    def disconnect(self):
        """
        Ferme proprement la connexion CAN.
        """

        if self.bus is not None:

            self.bus.shutdown()

            self.bus = None
            self.connected = False

            print("Connexion CAN fermee.")

    # --------------------------------------
    # 5. GESTION DU CONTEXTE
    # --------------------------------------

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.disconnect()
        return False
