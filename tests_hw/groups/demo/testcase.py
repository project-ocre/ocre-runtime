#!/usr/bin/env python3

import pexpect
import testlib

"""
This testcase is to be used following the flashing of the default demo sample to a board.

The testcase forms a serial connection to the board, sends a break and checks that 
the string "powered by Ocre" appears in the output of that break command. 
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
    full_output = ''
    serial_conn, pex = testlib.setup('/dev/ttyACM0')
    serial_conn.send_break()

    print("Checking Runtime Output:")
    for line in lines_to_check:
        print(f"Checking for line: '{line}'")
        expect_index = pex.expect([line, pexpect.TIMEOUT], 30)
        full_output += bytes(pex.before).decode(errors='ignore')

        if (expect_index == 1):
            print(f"Failed to find line: '{line}' in given timeout.")

            pex.expect([pexpect.EOF, pexpect.TIMEOUT], 10)
            full_output += bytes(pex.before).decode(errors='ignore')
            testlib.format_runtime_output(full_output, "Failed")

            testlib.full_exit(serial_conn, 1)

    pex.expect([pexpect.EOF, pexpect.TIMEOUT], 10)
    full_output += bytes(pex.before).decode(errors='ignore')

    print("Able to successful verify runtime")
    testlib.format_runtime_output(full_output, "Entire")
    testlib.full_exit(serial_conn, 0)

    
if __name__ == "__main__":
    main()