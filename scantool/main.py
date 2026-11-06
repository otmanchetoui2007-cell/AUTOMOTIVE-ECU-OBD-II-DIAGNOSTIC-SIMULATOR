
import threading
import queue
import time

from communication.can_client import CANClient
from obd.obd_services import OBDServices
from decoding.decoder import OBDDecoder
from interface.dashboard import Dashboard

from config.settings import (
    PID_ENGINE_RPM,
    PID_VEHICLE_SPEED,
    PID_COOLANT_TEMP,
    PID_THROTTLE_POSITION,
    PID_CONTROL_MODULE_VOLTAGE
)


# ---------------------------------------
# Configuration
# ---------------------------------------

PIDS_TO_READ = [
    PID_ENGINE_RPM,
    PID_VEHICLE_SPEED,
    PID_COOLANT_TEMP,
    PID_THROTTLE_POSITION,
    PID_CONTROL_MODULE_VOLTAGE
]

POLL_INTERVAL = 0.2


# ---------------------------------------
# Communication CAN en arriere-plan
# ---------------------------------------

def can_worker(data_queue, stop_event):

    client = CANClient()
    obd = OBDServices()
    decoder = OBDDecoder()

    try:

        # Connexion au bus CAN
        client.connect()

        data_queue.put(("connection", True))

        print("ScanTool connecte a vcan0.")

        # Lecture cyclique des PIDs
        while not stop_event.is_set():

            for pid in PIDS_TO_READ:

                if stop_event.is_set():
                    break

                try:

                    # Construire la requete OBD-II
                    request = obd.create_request(pid)

                    # Envoyer la requete a l'ECU
                    client.send_frame(
                        request["can_id"],
                        request["data"]
                    )

                    # Attendre la reponse ECU
                    response = client.receive_frame(
                        timeout=1.0,
                        expected_id=0x7E8
                    )

                    if response is None:

                        data_queue.put(
                            ("error", f"Timeout pour PID 0x{pid:02X}")
                        )

                        continue

                    # Decoder la reponse
                    result = decoder.decode_response(response)

                    # Transmettre le resultat au dashboard
                    data_queue.put(("data", result))

                except ValueError as error:

                    data_queue.put(
                        ("error", str(error))
                    )

                time.sleep(POLL_INTERVAL)

    except (RuntimeError, OSError) as error:

        data_queue.put(("error", str(error)))

    finally:

        client.disconnect()

        data_queue.put(("connection", False))

        print("Communication CAN arretee.")


# ---------------------------------------
# Application principale
# ---------------------------------------

def main():

    data_queue = queue.Queue()

    stop_event = threading.Event()

    app = Dashboard()

    app.set_connection(False)

    # Lancer le thread CAN
    worker = threading.Thread(
        target=can_worker,
        args=(data_queue, stop_event),
        daemon=True
    )

    worker.start()

    # -----------------------------------
    # Reception des donnees dans Tkinter
    # -----------------------------------

    def process_queue():

        try:

            while True:

                event_type, content = data_queue.get_nowait()

                if event_type == "connection":

                    app.set_connection(content)

                elif event_type == "data":

                    app.update_data(content)

                elif event_type == "error":

                    print("Diagnostic :", content)

        except queue.Empty:
            pass

        if app.winfo_exists():

            app.after(100, process_queue)

    # -----------------------------------
    # Fermeture propre de l'application
    # -----------------------------------

    def close_application():

        stop_event.set()

        app.destroy()

    app.protocol(
        "WM_DELETE_WINDOW",
        close_application
    )

    # Actualisation du dashboard
    app.after(100, process_queue)

    # Demarrer Tkinter
    app.mainloop()


if __name__ == "__main__":

    main()

