"""3D rocket attitude viewer. Reads "roll,pitch,yaw" lines from the ESP32.

Usage:
    python visualizer.py COM3
    python visualizer.py /dev/ttyUSB0 --log flight.csv
"""
import argparse
import time

import matplotlib.pyplot as plt
import numpy as np
import serial


def rot(r, p, y):
    r, p, y = np.radians([r, p, y])
    rx = np.array([[1, 0, 0], [0, np.cos(r), -np.sin(r)], [0, np.sin(r), np.cos(r)]])
    ry = np.array([[np.cos(p), 0, np.sin(p)], [0, 1, 0], [-np.sin(p), 0, np.cos(p)]])
    rz = np.array([[np.cos(y), -np.sin(y), 0], [np.sin(y), np.cos(y), 0], [0, 0, 1]])
    return rz @ ry @ rx


# Rocket body: long axis = X
BODY = np.array([[-1, 0, 0], [1, 0, 0]]).T
FINS = [np.array(f).T for f in (
    [[-1, 0, 0], [-1, 0.4, 0]], [[-1, 0, 0], [-1, -0.4, 0]],
    [[-1, 0, 0], [-1, 0, 0.4]], [[-1, 0, 0], [-1, 0, -0.4]])]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--log", help="save data to this CSV file")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)
    log = open(args.log, "w") if args.log else None
    if log:
        log.write("t,roll,pitch,yaw\n")
    t0 = time.time()

    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")
    try:
        while plt.fignum_exists(fig.number):
            line = ser.readline().decode(errors="ignore").strip()
            if not line or line.startswith("#"):
                continue
            try:
                r, p, y = map(float, line.split(","))
            except ValueError:
                continue
            if log:
                log.write(f"{time.time() - t0:.3f},{r},{p},{y}\n")
            R = rot(r, p, y)
            ax.cla()
            ax.set_xlim(-1.5, 1.5); ax.set_ylim(-1.5, 1.5); ax.set_zlim(-1.5, 1.5)
            ax.plot(*(R @ BODY), "b", lw=6)
            for f in FINS:
                ax.plot(*(R @ f), "r", lw=3)
            ax.set_title(f"Roll {r:.1f}  Pitch {p:.1f}  Yaw {y:.1f}")
            plt.pause(0.001)
    finally:
        if log:
            log.close()
        ser.close()


if __name__ == "__main__":
    main()
