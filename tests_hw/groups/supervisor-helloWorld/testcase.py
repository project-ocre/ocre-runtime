#!/usr/bin/env python3
import time
import serial
import sys


def main():

    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
    conn.reset_input_buffer()
    time.sleep(5)
    print("Running hello-world container:")
    conn.write(b'ocre start hello-world\n')
    response = conn.read(2048).decode(errors='ignore')

    print("Container Output:")
    print(response)
    conn.close()

    if "powered by Ocre" in response:
        sys.exit(0)
    sys.exit(1)

    

if __name__ == "__main__":
    main()