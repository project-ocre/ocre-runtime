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

    print("Cleaning up container webserver-complex")
    serial_conn.send_break()
    expect_index = pex.expect(["ocre:~$", pexpect.TIMEOUT], 30)
    runtime_output = bytes(pex.before).decode(errors='ignore')
    if (expect_index == 1):
        print("Zephyr runtime failed to come back up after a break")
        testlib.full_exit(serial_conn, 1)

    testlib.format_runtime_output(runtime_output, "Cleanup")
    testlib.full_exit(serial_conn, 0)


if __name__ == "__main__":
    main()
