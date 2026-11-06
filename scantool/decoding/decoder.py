
from config.settings import (
    CAN_RESPONSE_ID,
    CAN_FRAME_LENGTH,
    OBD_RESPONSE_OFFSET,
    ISO_TP_SINGLE_FRAME,
    PID_COOLANT_TEMP,
    PID_ENGINE_RPM,
    PID_VEHICLE_SPEED,
    PID_THROTTLE_POSITION,
    PID_CONTROL_MODULE_VOLTAGE
)


class OBDDecoder:

    def __init__(self):

        self.pid_information = {
            PID_COOLANT_TEMP: {
                "name": "Coolant Temperature",
                "unit": "°C",
                "data_length": 1
            },

            PID_ENGINE_RPM: {
                "name": "Engine RPM",
                "unit": "tr/min",
                "data_length": 2
            },

            PID_VEHICLE_SPEED: {
                "name": "Vehicle Speed",
                "unit": "km/h",
                "data_length": 1
            },

            PID_THROTTLE_POSITION: {
                "name": "Throttle Position",
                "unit": "%",
                "data_length": 1
            },

            PID_CONTROL_MODULE_VOLTAGE: {
                "name": "Control Module Voltage",
                "unit": "V",
                "data_length": 2
            }
        }

    def validate_response(self, message):

        if message is None:
            raise ValueError("Aucune reponse recue.")

        if message.is_extended_id:
            raise ValueError("Identifiant CAN etendu non accepte.")

        if message.arbitration_id != CAN_RESPONSE_ID:
            raise ValueError("Identifiant CAN de reponse invalide.")

        if message.dlc != CAN_FRAME_LENGTH:
            raise ValueError("Longueur de trame CAN invalide.")

        data = list(message.data)

        if len(data) != CAN_FRAME_LENGTH:
            raise ValueError("Nombre d'octets incorrect.")

        pci_type = data[0] & 0xF0

        if pci_type != ISO_TP_SINGLE_FRAME:
            raise ValueError("Trame ISO-TP non supportee.")

        payload_length = data[0] & 0x0F

        if payload_length < 3:
            raise ValueError("Message OBD-II trop court.")

        if payload_length > CAN_FRAME_LENGTH - 1:
            raise ValueError("Longueur ISO-TP invalide.")

        service = data[1]

        if service == 0x7F:
            raise ValueError(
                f"Reponse OBD-II negative : service demande 0x{data[2]:02X}, "
                f"code d'erreur 0x{data[3]:02X}"
            )

        if service != OBD_RESPONSE_OFFSET + 0x01:
            raise ValueError("Service OBD-II inattendu.")

        pid = data[2]

        if pid not in self.pid_information:
            raise ValueError(f"PID non supporte : 0x{pid:02X}")

        required_length = 2 + self.pid_information[pid]["data_length"]

        if payload_length < required_length:
            raise ValueError("Donnees du PID incompletes.")

        return data

    def decode_value(self, pid, data):

        A = data[3]
        B = data[4]

        if pid == PID_COOLANT_TEMP:
            return A - 40

        elif pid == PID_ENGINE_RPM:
            return (256 * A + B) / 4

        elif pid == PID_VEHICLE_SPEED:
            return A

        elif pid == PID_THROTTLE_POSITION:
            return (A * 100) / 255

        elif pid == PID_CONTROL_MODULE_VOLTAGE:
            return (256 * A + B) / 1000

        else:
            raise ValueError(f"PID inconnu : 0x{pid:02X}")

    def decode_response(self, message):

        data = self.validate_response(message)

        pid = data[2]

        value = self.decode_value(pid, data)

        information = self.pid_information[pid]

        result = {
            "pid": pid,
            "pid_hex": f"0x{pid:02X}",
            "name": information["name"],
            "value": value,
            "unit": information["unit"]
        }

        return result

    def display_result(self, result):

        print("\n========== OBD-II RESULT ==========")

        print(f"PID   : {result['pid_hex']}")

        print(f"Name  : {result['name']}")

        print(
            f"Value : {result['value']:.2f} "
            f"{result['unit']}"
        )

        print("===================================")
