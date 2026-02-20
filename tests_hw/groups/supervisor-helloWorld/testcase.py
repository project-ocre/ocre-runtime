#!/usr/bin/env python3

import sys
sys.path.append("../..")

import pexpect
import testlib

"""
This testcase is to be used following the flashing of the supervisor sample to a board with the hello-world container running.

The testcase forms a serial connection to the board, sends a break and checks that 
the string "powered by Ocre" appears in the output of that break command. 
"""

line = "powered by Ocre"

def main():
    full_output = ''
    serial_conn, pex = testlib.setup('/dev/ttyACM0')

    print("Running hello-world container:")
    pex.write(b'ocre start hello-world\n')
    expect_index = pex.expect([line, pexpect.TIMEOUT], 20)
    full_output += bytes(pex.before).decode(errors='ignore')

    if (expect_index == 1):
        print(f"Failed to find line: '{line}' in given timeout.")
        pex.expect([pexpect.EOF, pexpect.TIMEOUT], 10)
        full_output += bytes(pex.before).decode(errors='ignore')
        testlib.format_runtime_output(full_output, "Failed")

        testlib.full_exit(serial_conn, 1)

    pex.expect(["ocre:~$", pexpect.TIMEOUT], 10)
    full_output += bytes(pex.before).decode(errors='ignore')

    if (expect_index == 1):
        print("Container failed to exit in given timeout")
        pex.expect([pexpect.EOF, pexpect.TIMEOUT], 10)
        full_output += bytes(pex.before).decode(errors='ignore')
        testlib.format_runtime_output(full_output, "Failed")

        testlib.full_exit(serial_conn, 1)

    testlib.format_runtime_output(full_output, "Entire")
    testlib.full_exit(serial_conn, 0)
    
if __name__ == "__main__":
    main()