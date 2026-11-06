# ==========================================
# SCANTOOL - OBD-II SERVICES
# ==========================================

from config.settings import (
    CAN_REQUEST_ID,
    CAN_FRAME_LENGTH,
    OBD_SERVICE_CURRENT_DATA,
    PID_COOLANT_TEMP,
    PID_ENGINE_RPM,
    PID_VEHICLE_SPEED,
    PID_THROTTLE_POSITION,
    PID_CONTROL_MODULE_VOLTAGE,
    ISO_TP_SINGLE_FRAME
)


class OBDServices:

    # --------------------------------------
    # 1. INITIALISATION
    # --------------------------------------

    def __init__(self):

        self.supported_pids = {
            PID_COOLANT_TEMP: "Coolant Temperature",
            PID_ENGINE_RPM: "Engine RPM",
            PID_VEHICLE_SPEED: "Vehicle Speed",
            PID_THROTTLE_POSITION: "Throttle Position",
            PID_CONTROL_MODULE_VOLTAGE: "Control Module Voltage"
        }

    # --------------------------------------
    # 2. VERIFIER LE PID
    # --------------------------------------

    def is_pid_supported(self, pid):

        return pid in self.supported_pids

    # --------------------------------------
    # 3. RECUPERER LE NOM DU PID
    # --------------------------------------

    def get_pid_name(self, pid):

        return self.supported_pids.get(pid, "Unknown PID")

    # --------------------------------------
    # 4. CONSTRUIRE UNE REQUETE OBD-II
    # --------------------------------------

    def build_request(self, pid, service=OBD_SERVICE_CURRENT_DATA):

        # Verifier le service
        if service != OBD_SERVICE_CURRENT_DATA:
            raise ValueError(
                f"Service OBD-II non implemente : 0x{service:02X}"
            )

        # Verifier le PID
        if not self.is_pid_supported(pid):
            raise ValueError(
                f"PID non supporte : 0x{pid:02X}"
            )

        # Construire les donnees de la requete
        data = [
            ISO_TP_SINGLE_FRAME | 0x02,
            service,
            pid
        ]

        # Ajouter les octets de remplissage
        while len(data) < CAN_FRAME_LENGTH:
            data.append(0x00)

        return data

    # --------------------------------------
    # 5. CONSTRUIRE UNE REQUETE COMPLETE
    # --------------------------------------

    def create_request(self, pid, service=OBD_SERVICE_CURRENT_DATA):

        data = self.build_request(pid, service)

        return {
            "can_id": CAN_REQUEST_ID,
            "data": data,
            "pid": pid,
            "service": service
        }

    # --------------------------------------
    # 6. AFFICHER LA REQUETE
    # --------------------------------------

    def display_request(self, request):

        print("\n========== OBD-II REQUEST ==========")

        print(f"CAN ID  : 0x{request['can_id']:03X}")

        print(f"Service : 0x{request['service']:02X}")

        print(f"PID     : 0x{request['pid']:02X}")

        print(
            "DATA    :",
            " ".join(f"{byte:02X}" for byte in request["data"])
        )

        print("====================================")
