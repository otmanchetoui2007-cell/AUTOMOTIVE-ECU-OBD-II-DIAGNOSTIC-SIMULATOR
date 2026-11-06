
import tkinter as tk
import math
import time


# ==========================================================
# CIRCULAR GAUGE WITH SMOOTH ANIMATION
# ==========================================================

class CircularGauge(tk.Canvas):

    def __init__(
        self,
        parent,
        title,
        unit,
        maximum,
        color="#00D4FF",
        size=280,
        red_zone=None
    ):
        super().__init__(
            parent,
            width=size,
            height=size,
            bg="#192735",
            highlightthickness=0
        )

        self.title = title
        self.unit = unit
        self.maximum = maximum
        self.color = color
        self.size = size
        self.red_zone = red_zone

        self.value = 0.0
        self.target_value = 0.0
        self.animation_id = None

        self.draw_gauge()

    def point(self, angle, radius):

        cx = self.size / 2
        cy = self.size / 2

        rad = math.radians(angle)

        return (
            cx + radius * math.cos(rad),
            cy - radius * math.sin(rad)
        )

    def set_value(self, value):

        value = max(0.0, min(float(value), self.maximum))
        self.target_value = value

        if self.animation_id is not None:
            self.after_cancel(self.animation_id)
            self.animation_id = None

        self.animate()

    def animate(self):

        difference = self.target_value - self.value

        if abs(difference) < 0.5:
            self.value = self.target_value
            self.animation_id = None
            self.draw_gauge()
            return

        # Interpolation pour un mouvement fluide
        self.value += difference * 0.22

        self.draw_gauge()

        self.animation_id = self.after(20, self.animate)

    def draw_gauge(self):

        self.delete("all")

        cx = self.size / 2
        cy = self.size / 2

        radius = self.size * 0.37

        # Cercle extérieur
        self.create_oval(
            cx - radius - 18,
            cy - radius - 18,
            cx + radius + 18,
            cy + radius + 18,
            outline="#304458",
            width=2
        )

        # Arc de fond
        self.create_arc(
            cx - radius,
            cy - radius,
            cx + radius,
            cy + radius,
            start=135,
            extent=-270,
            style=tk.ARC,
            outline="#34495E",
            width=12
        )

        # Zone rouge du compte-tours
        if self.red_zone is not None:

            red_start = self.red_zone / self.maximum

            self.create_arc(
                cx - radius,
                cy - radius,
                cx + radius,
                cy + radius,
                start=135 - 270 * red_start,
                extent=-270 * (1 - red_start),
                style=tk.ARC,
                outline="#FF3B4F",
                width=12
            )

        # Arc de progression
        progress = self.value / self.maximum

        active_color = self.color

        if self.red_zone is not None and self.value >= self.red_zone:
            active_color = "#FF3B4F"

        self.create_arc(
            cx - radius,
            cy - radius,
            cx + radius,
            cy + radius,
            start=135,
            extent=-270 * progress,
            style=tk.ARC,
            outline=active_color,
            width=12
        )

        # Graduations
        for i in range(11):

            fraction = i / 10
            angle = 135 - 270 * fraction

            x1, y1 = self.point(angle, radius + 3)
            x2, y2 = self.point(angle, radius + 14)

            tick_color = "#A8B7C8"

            if (
                self.red_zone is not None
                and fraction * self.maximum >= self.red_zone
            ):
                tick_color = "#FF5252"

            self.create_line(
                x1, y1, x2, y2,
                fill=tick_color,
                width=2
            )

            tx, ty = self.point(angle, radius + 29)

            label = int(self.maximum * fraction)

            self.create_text(
                tx, ty,
                text=str(label),
                fill=tick_color,
                font=("Segoe UI", 8)
            )

        # Titre
        self.create_text(
            cx,
            cy - 43,
            text=self.title,
            fill="#A8B7C8",
            font=("Segoe UI", 10, "bold")
        )

        # Valeur numérique
        self.create_text(
            cx,
            cy + 2,
            text=f"{self.value:.0f}",
            fill=active_color,
            font=("Segoe UI", 29, "bold")
        )

        # Unité
        self.create_text(
            cx,
            cy + 35,
            text=self.unit,
            fill="#A8B7C8",
            font=("Segoe UI", 9)
        )

        # Aiguille
        angle = 135 - 270 * progress

        nx, ny = self.point(angle, radius - 18)

        self.create_line(
            cx, cy, nx, ny,
            fill="#F2F5F8",
            width=3
        )

        self.create_oval(
            cx - 7, cy - 7,
            cx + 7, cy + 7,
            fill=active_color,
            outline=""
        )


# ==========================================================
# RPM HISTORY GRAPH
# ==========================================================

class RPMHistory(tk.Canvas):

    def __init__(self, parent, width=440, height=220):

        super().__init__(
            parent,
            width=width,
            height=height,
            bg="#192735",
            highlightthickness=0
        )

        self.width = width
        self.height = height

        self.values = []
        self.max_points = 60
        self.max_rpm = 8000

        self.draw_graph()

    def add_value(self, rpm):

        self.values.append(float(rpm))

        if len(self.values) > self.max_points:
            self.values.pop(0)

        self.draw_graph()

    def draw_graph(self):

        self.delete("all")

        left = 45
        right = self.width - 15
        top = 35
        bottom = self.height - 30

        # Titre
        self.create_text(
            15, 15,
            text="RPM HISTORY",
            anchor="w",
            fill="#00D4FF",
            font=("Segoe UI", 10, "bold")
        )

        # Grille horizontale
        for i in range(5):

            y = top + (bottom - top) * i / 4

            rpm_label = int(self.max_rpm * (4 - i) / 4)

            self.create_line(
                left, y, right, y,
                fill="#304458",
                dash=(3, 4)
            )

            self.create_text(
                left - 7, y,
                text=str(rpm_label),
                anchor="e",
                fill="#9AA9B8",
                font=("Segoe UI", 7)
            )

        # Zone de tracé
        if len(self.values) < 2:
            self.create_text(
                self.width / 2,
                self.height / 2,
                text="Waiting for RPM data...",
                fill="#9AA9B8",
                font=("Segoe UI", 9)
            )
            return

        points = []

        for i, rpm in enumerate(self.values):

            x = left + (
                i / (self.max_points - 1)
            ) * (right - left)

            rpm = max(0, min(rpm, self.max_rpm))

            y = bottom - (
                rpm / self.max_rpm
            ) * (bottom - top)

            points.extend([x, y])

        # Courbe RPM
        self.create_line(
            points,
            fill="#00D4FF",
            width=2,
            smooth=True
        )

        # Dernier point
        x = points[-2]
        y = points[-1]

        self.create_oval(
            x - 4, y - 4,
            x + 4, y + 4,
            fill="#00D4FF",
            outline=""
        )


# ==========================================================
# AUTOMOTIVE DASHBOARD
# ==========================================================

class Dashboard(tk.Tk):

    def __init__(self):

        super().__init__()

        self.title("AUTO DIAGNOSTIC | DIGITAL COCKPIT")
        self.geometry("1250x850")
        self.minsize(1050, 760)

        self.configure(bg="#101820")

        self.BG = "#101820"
        self.PANEL = "#192735"
        self.CARD = "#223344"
        self.CYAN = "#00D4FF"
        self.GREEN = "#00D68F"
        self.ORANGE = "#FFAA33"
        self.RED = "#FF5252"
        self.GRAY = "#9AA9B8"

        self.metric_labels = {}

        self.create_header()
        self.create_dashboard()
        self.create_footer()

    # ------------------------------------------------------
    # HEADER
    # ------------------------------------------------------

    def create_header(self):

        header = tk.Frame(
            self,
            bg=self.PANEL,
            height=65
        )

        header.pack(fill="x")
        header.pack_propagate(False)

        tk.Label(
            header,
            text="AUTO DIAGNOSTIC",
            font=("Segoe UI", 19, "bold"),
            bg=self.PANEL,
            fg=self.CYAN
        ).pack(side="left", padx=25)

        tk.Label(
            header,
            text="REAL-TIME VEHICLE MONITORING",
            font=("Segoe UI", 9),
            bg=self.PANEL,
            fg=self.GRAY
        ).pack(side="left")

        self.status_label = tk.Label(
            header,
            text="● ECU DISCONNECTED",
            font=("Segoe UI", 10, "bold"),
            bg=self.PANEL,
            fg=self.RED
        )

        self.status_label.pack(side="right", padx=25)

    # ------------------------------------------------------
    # DASHBOARD CONTENT
    # ------------------------------------------------------

    def create_dashboard(self):

        main = tk.Frame(self, bg=self.BG)

        main.pack(
            fill="both",
            expand=True,
            padx=15,
            pady=10
        )

        # Main gauges
        top = tk.Frame(main, bg=self.BG)
        top.pack()

        self.rpm_gauge = CircularGauge(
            top,
            "ENGINE RPM",
            "tr/min",
            8000,
            "#00D4FF",
            300,
            red_zone=6500
        )

        self.rpm_gauge.pack(side="left", padx=12)

        self.speed_gauge = CircularGauge(
            top,
            "VEHICLE SPEED",
            "km/h",
            240,
            "#00D68F",
            300
        )

        self.speed_gauge.pack(side="left", padx=12)

        # Secondary indicators
        bottom = tk.Frame(main, bg=self.BG)
        bottom.pack(fill="x", pady=5)

        # Throttle circular gauge
        self.throttle_gauge = CircularGauge(
            bottom,
            "THROTTLE",
            "%",
            100,
            "#FFAA33",
            220
        )

        self.throttle_gauge.pack(
            side="left",
            padx=8,
            pady=5
        )

        # Coolant and voltage cards
        sensor_frame = tk.Frame(
            bottom,
            bg=self.BG
        )

        sensor_frame.pack(
            side="left",
            padx=8,
            fill="y"
        )

        self.create_sensor_card(
            sensor_frame,
            0x05,
            "COOLANT TEMP",
            "°C",
            self.CYAN
        )

        self.create_sensor_card(
            sensor_frame,
            0x42,
            "ECU VOLTAGE",
            "V",
            self.GREEN
        )

        # RPM graph
        self.rpm_history = RPMHistory(
            bottom,
            width=440,
            height=220
        )

        self.rpm_history.pack(
            side="left",
            padx=8,
            pady=5
        )

    # ------------------------------------------------------
    # SENSOR CARD
    # ------------------------------------------------------

    def create_sensor_card(
        self,
        parent,
        pid,
        title,
        unit,
        color
    ):

        card = tk.Frame(
            parent,
            bg=self.CARD,
            width=155,
            height=95
        )

        card.pack(pady=7)
        card.pack_propagate(False)

        tk.Label(
            card,
            text=title,
            bg=self.CARD,
            fg=self.GRAY,
            font=("Segoe UI", 9, "bold")
        ).pack(pady=(10, 2))

        value_label = tk.Label(
            card,
            text="--",
            bg=self.CARD,
            fg=color,
            font=("Segoe UI", 22, "bold")
        )

        value_label.pack()

        tk.Label(
            card,
            text=unit,
            bg=self.CARD,
            fg=self.GRAY,
            font=("Segoe UI", 8)
        ).pack()

        self.metric_labels[pid] = value_label

    # ------------------------------------------------------
    # FOOTER
    # ------------------------------------------------------

    def create_footer(self):

        footer = tk.Frame(
            self,
            bg=self.PANEL,
            height=35
        )

        footer.pack(side="bottom", fill="x")

        tk.Label(
            footer,
            text="OBD-II  |  CAN BUS  |  SOCKETCAN  |  VIRTUAL ECU",
            bg=self.PANEL,
            fg=self.GRAY,
            font=("Segoe UI", 9)
        ).pack(pady=8)

    # ------------------------------------------------------
    # CONNECTION STATUS
    # ------------------------------------------------------

    def set_connection(self, connected):

        if connected:

            self.status_label.config(
                text="● ECU CONNECTED",
                fg=self.GREEN
            )

        else:

            self.status_label.config(
                text="● ECU DISCONNECTED",
                fg=self.RED
            )

    # ------------------------------------------------------
    # UPDATE A PID VALUE
    # ------------------------------------------------------

    def update_metric(self, pid, value):

        if pid == 0x0C:

            self.rpm_gauge.set_value(value)
            self.rpm_history.add_value(value)

        elif pid == 0x0D:

            self.speed_gauge.set_value(value)

        elif pid == 0x11:

            # Throttle en temps réel
            self.throttle_gauge.set_value(value)

        elif pid in self.metric_labels:

            if pid == 0x42:
                formatted = f"{value:.2f}"
            else:
                formatted = f"{value:.0f}"

            self.metric_labels[pid].config(
                text=formatted
            )

    # ------------------------------------------------------
    # UPDATE FROM OBD DECODER
    # ------------------------------------------------------

    def update_data(self, result):

        pid = result["pid"]
        value = result["value"]

        self.update_metric(pid, value)


if __name__ == "__main__":

    app = Dashboard()
    app.mainloop()
