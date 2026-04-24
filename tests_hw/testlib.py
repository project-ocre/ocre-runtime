# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

import serial
import sys
import pexpect
import pexpect.fdpexpect
from typing import Tuple

shell_prompt = "ocre:~$"
bu585_base_url = "http://ocre-b-u585i.lfedge.iol.unh.edu:8000"


def setup(device_filepath: str) -> Tuple[serial.Serial, pexpect.fdpexpect.fdspawn]:
    conn = serial.Serial(device_filepath, 115200, timeout=10)
    conn.reset_input_buffer()
    conn.reset_output_buffer()
    pex = pexpect.fdpexpect.fdspawn(conn.fileno())

    return (conn, pex)


def full_exit(conn: serial.Serial, status: int):
    conn.reset_input_buffer()
    conn.reset_output_buffer()
    conn.close()
    sys.exit(status)


def format_runtime_output(runtime_output: str, section: str) -> None:
    print(f"""
========== {section} runtime output ==========
{runtime_output}
==========--------------------------==========
""")
    return
