#!/usr/bin/env python3

import sys
sys.path.append("../..")

import testlib
import pexpect

def main():
    serial_conn, pex = testlib.setup('/dev/ttyACM0')

    print("Creating container on board off of hello-world.wasm")
    pex.write(b'ocre create -n hello-world -k ocre:api hello-world.wasm\n')
    expect_index = pex.expect(["ocre:~$", pexpect.TIMEOUT], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')
    
    if (expect_index == 1):
        print("Container failed to create container in given timeout")
        testlib.format_runtime_output(runtime_output, "Failed")
        testlib.full_exit(serial_conn, 1)

    if "Failed to create container" in runtime_output:
        print("Failed to create container")
        testlib.format_runtime_output(runtime_output, "Failed")
        testlib.full_exit(serial_conn, 1)
    
    testlib.full_exit(serial_conn, 0)

    
if __name__ == "__main__":
    main()