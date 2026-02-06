#!/usr/bin/env python3
import serial
import time
import sys

def main():

    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
    time.sleep(5)
    conn.reset_input_buffer()

    print("Creating container on board off of hello-world.wasm")
    conn.write(b'ocre create -n hello-world -k ocre:api hello-world.wasm\n')
    response = conn.read(2048).decode(errors='ignore')
    print(response)

    conn.close()

    if "Failed to create container" in response:
        sys.exit(1)
    
    
if __name__ == "__main__":
    main()