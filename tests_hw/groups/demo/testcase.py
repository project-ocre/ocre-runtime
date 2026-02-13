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
    "Demo completed successfully",
]


# Test stub, add more checks to ensure other containers ran successfully
def main():


    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
    conn.reset_input_buffer()
    time.sleep(20)
    # Refactor to use conn.read_until to remove need for separate timeout
    response = conn.read(2048).decode(errors='ignore')

    print("Container Output:")
    print(response)
    conn.close()

    for line in lines_to_check:
        if line not in response:
            sys.exit(1)
    
    sys.exit(0)

    
if __name__ == "__main__":
    main()