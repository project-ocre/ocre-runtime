#!/usr/bin/env python3

import sys
sys.path.append("../..")

import pexpect
import testlib

"""
This testcase is to be used following the flashing of the default demo sample to a board.

The testcase forms a serial connection to the board, sends a break and checks that 
the strings from each section of a successful run appears in the output of that break command. 
"""

lines_to_check = [
    "powered by Ocre",
    "Generic blinking started.",
    "Container exited with status 0",
    "Subscriber initialized",
    "Publisher initialized",
    "Subscriber exited with status 0",
    "Publisher exited with status 0",
    "Demo completed successfully",
]

def main():
    serial_conn, pex = testlib.setup('/dev/ttyACM0')
    serial_conn.send_break()
    pex.expect([pexpect.EOF, pexpect.TIMEOUT], 45)
    runtime_output = bytes(pex.before).decode(errors='ignore')

    print("Checking Runtime Output:")
    for line in lines_to_check:
        print(f"Checking for line: '{line}'")
        if (line not in runtime_output):
            print(f"Failed to find line: '{line}' in given timeout.")

            testlib.format_runtime_output(runtime_output, "Failed")
            testlib.full_exit(serial_conn, 1)

    print("Able to successful verify runtime")
    testlib.format_runtime_output(runtime_output, "Entire")
    testlib.full_exit(serial_conn, 0)

    
if __name__ == "__main__":
    main()