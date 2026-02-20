#!/usr/bin/env python3

import sys
sys.path.append("../..")

import pexpect
import testlib


def main():
    serial_conn, pex = testlib.setup('/dev/ttyACM0')

    print("Cleaning up container hello-world")
    pex.write(b'ocre container ps\n')
    pex.expect(["ocre:~$", pexpect.TIMEOUT], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')
    if ("hello-world" in runtime_output):
        pex.write(b'ocre rm hello-world\n')
        runtime_output += bytes(pex.before).decode(errors='ignore')

    testlib.format_runtime_output(runtime_output, "Cleanup")
    testlib.full_exit(serial_conn, 0)

if __name__ == "__main__":
    main()