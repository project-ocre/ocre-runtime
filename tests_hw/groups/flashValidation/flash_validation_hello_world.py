#!/usr/bin/env python3

import serial
import time
import sys

"""
This testcase is to be used following the flashing of the Ocre runtime to a board.

The testcase forms a serial connection to the board, sends a break and checks that 
the string Hello World! appears in the output of that break command. 
"""

def main():
    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)

    print('**** WAITING FOR SYSTEM TO FULLY INITIALIZE ****\n')

    # Wait for the system to boot and initialize completely
    # We need to wait for network initialization and OCRE runtime startup
    time.sleep(5)

    # Clear any existing data in the buffer
    conn.reset_input_buffer()

    # Send break to get current output
    conn.send_break(duration=1)
    time.sleep(10)

    print('**** READING RESPONSE FROM BREAK ****\n')

    # Read response with longer timeout to capture all output
    response = conn.read(2048).decode(errors='ignore')
    print(response)

    print('**** CLOSING CONNECTION ****\n')

    conn.close()

    if "Hello World from Ocre!" in response:
        sys.exit(0)
    sys.exit(1)


if __name__ == "__main__":
    main()