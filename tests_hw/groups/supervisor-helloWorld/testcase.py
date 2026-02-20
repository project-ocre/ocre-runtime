#!/usr/bin/env python3

import sys
sys.path.append("../..")

import pexpect
import testlib

"""
This testcase is to be used following the flashing of the supervisor sample to a board with the hello-world container put up.

The testcase forms a serial connection to the board, runs the hello-world container  
and checks the string "powered by Ocre" appears in the output of the container. 
"""

line = "powered by Ocre"

def main():
    serial_conn, pex = testlib.setup('/dev/ttyACM0')

    print("Running hello-world container:")
    pex.write(b'ocre start hello-world\n')
 
    expect_index = pex.expect(["ocre:~$", pexpect.TIMEOUT], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')

    if (expect_index == 1):
        print("Container failed to exit in given timeout")
        testlib.format_runtime_output(runtime_output, "Failed")
        testlib.full_exit(serial_conn, 1)

    if (line not in runtime_output):
        print(f"Failed to find line: '{line}' in given timeout.")
        testlib.format_runtime_output(runtime_output, "Failed")
        testlib.full_exit(serial_conn, 1)

    testlib.format_runtime_output(runtime_output, "Entire")
    testlib.full_exit(serial_conn, 0)
    
if __name__ == "__main__":
    main()