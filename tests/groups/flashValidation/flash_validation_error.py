#!/usr/bin/env python3

import serial
import time
import sys

"""
This testcase is to be used following the flashing of the Ocre runtime to a board.

The testcase forms a serial connection to the board, sends a break and checks that 
there are no instance of the error prefix "E: " in the output returned from the break. 
"""

def main():
    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    conn.send_break(duration=1)

    time.sleep(1)

    print('**** READING RESPONSE FROM BREAK ****\n')

    response = conn.read(1024).decode(errors='ignore')
    print(response)

    print('**** CLOSING CONNECTION ****\n')

    conn.close()

    if "E:" not in response:
        sys.exit(0)
    sys.exit(1)


if __name__ == "__main__":
    main()
