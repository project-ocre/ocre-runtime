#!/usr/bin/env python3

import serial
import time
import sys

"""
This testcase is to be used following the flashing of the default demo sample to a board.

The testcase forms a serial connection to the board, sends a break and checks that 
the string "powered by Ocre" appears in the output of that break command. 
"""

lines_to_check = [
    "powered by Ocre",
    "Generic blinking started.",
    "Subscriber initialized",
    "Publisher initialized",
    "Demo completed successfully",
]


def main():

    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
    conn.reset_input_buffer()
    conn.send_break()
    print("Container Output:")
    for line in lines_to_check:
        response = conn.read_until(line.encode(), 4096).decode(errors='ignore')
        print(response)

        if line not in response:
            conn.reset_output_buffer()
            conn.close()
            sys.exit(1)
    
    conn.reset_output_buffer()
    conn.close()
    sys.exit(0)

    
if __name__ == "__main__":
    main()