
import tkinter as tk
import can

from interface.dashboard import Dashboard
from decoding.decoder import OBDDecoder


app = Dashboard()
decoder = OBDDecoder()

app.set_connection(True)


def simulate_ecu_response():

    responses = [
        [0x04, 0x41, 0x0C, 0x1A, 0xF8, 0, 0, 0],
        [0x03, 0x41, 0x0D, 0x3C, 0, 0, 0, 0],
        [0x03, 0x41, 0x05, 0x82, 0, 0, 0, 0],
        [0x03, 0x41, 0x11, 0x80, 0, 0, 0, 0],
        [0x04, 0x41, 0x42, 0x2E, 0xE0, 0, 0, 0]
    ]

    for data in responses:

        message = can.Message(
            arbitration_id=0x7E8,
            data=data,
            is_extended_id=False
        )

        result = decoder.decode_response(message)

        app.update_data(result)

    app.after(2000, simulate_ecu_response)


app.after(1000, simulate_ecu_response)

app.mainloop()
