#!/usr/bin/env python3
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

import sys
sys.path.append("../..")

import testlib  # noqa: E402
import pexpect  # noqa: E402


def main():
    serial_conn, pex = testlib.setup('/dev/ttyACM0')

    pex.write(b'net iface\n')
    pex.expect(["ocre:~$", pexpect.TIMEOUT], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')
    if ("DHCP preferred" not in runtime_output):  # Listed under IPv4 address if one exists
        print("Board does not have an IP address, running DHCP client")
        pex.write(b'net dhcpv4 client start 1\n')
        while ("DHCP preferred" not in runtime_output):
            print(".", end='')
            pex.write(b'net iface\n')
            pex.expect(["ocre:~$", pexpect.TIMEOUT], 5)
            runtime_output = bytes(pex.before).decode(errors='ignore')
        print('\nBoard obtained IP address')

    print("Creating container on board off of webserver-complex.wasm")
    pex.write(
        b'ocre run -n webserver-complex -k networking -d webserver-complex.wasm\n')
    pex.expect([pexpect.TIMEOUT, pexpect.EOF], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')

    if ("Failed to create container" in runtime_output) or ("Server Status: ONLINE" not in runtime_output):
        print("Failed to create container")
        testlib.format_runtime_output(runtime_output, "Failed")

        testlib.full_exit(serial_conn, 1)

    testlib.format_runtime_output(runtime_output, "Setup")
    testlib.full_exit(serial_conn, 0)


if __name__ == "__main__":
    main()
