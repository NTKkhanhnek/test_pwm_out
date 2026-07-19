#py -3.11 .\pwm_simu\pwm_simu.py

import tkinter as tk
from tkinter import ttk

try:
    import serial
except ImportError:
    serial = None


BAUDRATE = 115200
UART_PORT = "COM5"
UART_BYTESIZE = 8
UART_PARITY = "N"
UART_STOPBITS = 1
MOTOR_MIN = 1
MOTOR_MAX = 4
PULSE_MIN = 1100
PULSE_MAX = 1950


class PwmSimulator:
    def __init__(self, root):
        self.root = root
        self.root.title("PWM UART Simulator")
        self.root.geometry("620x320")
        self.root.resizable(False, False)

        self.uart = None
        self.motor_var = tk.IntVar(value=1)
        self.pulse_var = tk.IntVar(value=1100)
        self.port_var = tk.StringVar(value=UART_PORT)
        self.auto_send_var = tk.BooleanVar(value=True)
        self.status_var = tk.StringVar(value="Disconnected.")
        self.updating_ui = False

        self.build_ui()

    def build_ui(self):
        top = ttk.Frame(self.root, padding=16)
        top.pack(fill="x")

        ttk.Label(top, text="UART port").pack(side="left")
        ttk.Entry(top, textvariable=self.port_var, width=10).pack(side="left", padx=(8, 10))
        self.connect_button = ttk.Button(top, text="Connect", command=self.toggle_uart)
        self.connect_button.pack(side="left")
        ttk.Checkbutton(top, text="Auto", variable=self.auto_send_var).pack(side="left", padx=(14, 0))

        body = ttk.Frame(self.root, padding=(16, 18))
        body.pack(fill="both", expand=True)

        left = ttk.LabelFrame(body, text="motor", padding=16)
        left.pack(side="left", fill="both", expand=True, padx=(0, 10))

        right = ttk.LabelFrame(body, text="pulse width", padding=16)
        right.pack(side="left", fill="both", expand=True, padx=(10, 0))

        self.motor_slider = ttk.Scale(
            left,
            from_=MOTOR_MIN,
            to=MOTOR_MAX,
            orient="horizontal",
            command=self.on_motor_slider,
        )
        self.motor_slider.pack(fill="x", pady=(20, 8))

        motor_spin = ttk.Spinbox(
            left,
            from_=MOTOR_MIN,
            to=MOTOR_MAX,
            textvariable=self.motor_var,
            width=8,
            command=self.on_input_change,
        )
        motor_spin.pack()
        motor_spin.bind("<Return>", self.on_enter)
        motor_spin.bind("<FocusOut>", self.on_focus_out)

        self.pulse_slider = ttk.Scale(
            right,
            from_=PULSE_MIN,
            to=PULSE_MAX,
            orient="horizontal",
            command=self.on_pulse_slider,
        )
        self.pulse_slider.pack(fill="x", pady=(20, 8))

        pulse_spin = ttk.Spinbox(
            right,
            from_=PULSE_MIN,
            to=PULSE_MAX,
            textvariable=self.pulse_var,
            width=8,
            command=self.on_input_change,
        )
        pulse_spin.pack()
        pulse_spin.bind("<Return>", self.on_enter)
        pulse_spin.bind("<FocusOut>", self.on_focus_out)

        bottom = ttk.Frame(self.root, padding=(16, 0, 16, 16))
        bottom.pack(fill="x")

        ttk.Button(bottom, text="Send", command=self.send_values).pack(pady=(0, 10))
        ttk.Label(bottom, textvariable=self.status_var, anchor="center").pack(fill="x")

        self.sync_sliders()

    def get_int_var(self, var, fallback):
        try:
            return int(var.get())
        except (tk.TclError, ValueError):
            return fallback

    def clamp_values(self):
        motor = self.get_int_var(self.motor_var, MOTOR_MIN)
        pulse = self.get_int_var(self.pulse_var, PULSE_MIN)

        self.motor_var.set(min(max(motor, MOTOR_MIN), MOTOR_MAX))
        self.pulse_var.set(min(max(pulse, PULSE_MIN), PULSE_MAX))

    def sync_sliders(self):
        self.updating_ui = True
        self.motor_slider.set(self.get_int_var(self.motor_var, MOTOR_MIN))
        self.pulse_slider.set(self.get_int_var(self.pulse_var, PULSE_MIN))
        self.updating_ui = False

    def on_motor_slider(self, value):
        if self.updating_ui:
            return

        self.motor_var.set(round(float(value)))
        self.send_if_auto()

    def on_pulse_slider(self, value):
        if self.updating_ui:
            return

        self.pulse_var.set(round(float(value)))
        self.send_if_auto()

    def on_input_change(self):
        self.clamp_values()
        self.sync_sliders()
        self.send_if_auto()

    def on_enter(self, _event):
        self.clamp_values()
        self.sync_sliders()
        self.send_values()

    def on_focus_out(self, _event):
        self.clamp_values()
        self.sync_sliders()

    def toggle_uart(self):
        if self.uart is not None:
            self.uart.close()
            self.uart = None
            self.connect_button.config(text="Connect")
            self.status_var.set("Disconnected.")
            return

        if serial is None:
            self.status_var.set("Missing pyserial. Run: python -m pip install pyserial")
            return

        try:
            self.uart = serial.Serial(
                port=self.port_var.get(),
                baudrate=BAUDRATE,
                bytesize=UART_BYTESIZE,
                parity=UART_PARITY,
                stopbits=UART_STOPBITS,
                timeout=0.1,
            )
        except serial.SerialException as exc:
            self.uart = None
            self.status_var.set(f"Cannot open port: {exc}")
            return

        self.connect_button.config(text="Disconnect")
        self.status_var.set(
            f"Connected to {self.port_var.get()} @ {BAUDRATE} 8N1."
        )

    def send_if_auto(self):
        if self.auto_send_var.get():
            self.send_values()
        else:
            self.status_var.set(f"Ready: {self.motor_var.get()} {self.pulse_var.get()}")

    def send_values(self):
        self.clamp_values()
        self.sync_sliders()

        if self.uart is None:
            self.status_var.set("Not connected.")
            return

        packet = f"{self.motor_var.get()} {self.pulse_var.get()}\r\n"

        try:
            self.uart.write(packet.encode("ascii"))
        except serial.SerialException as exc:
            self.status_var.set(f"UART send failed: {exc}")
            return

        self.status_var.set(f"Sent: {packet.strip()}")


def main():
    root = tk.Tk()
    app = PwmSimulator(root)
    root.protocol("WM_DELETE_WINDOW", lambda: close_app(root, app))
    root.mainloop()


def close_app(root, app):
    if app.uart is not None:
        app.uart.close()

    root.destroy()


if __name__ == "__main__":
    main()
