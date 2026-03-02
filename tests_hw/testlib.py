import serial
import sys
import pexpect
import pexpect.fdpexpect
from typing import Tuple

shell_prompt = "ocre:~$" 

def setup(device_filepath: str) -> Tuple[serial.Serial, pexpect.fdpexpect.fdspawn]:
    conn = serial.Serial(device_filepath, 115200, timeout=10)
    conn.reset_input_buffer()
    conn.reset_output_buffer()
    conn.flush()
    pex = pexpect.fdpexpect.fdspawn(conn.fileno())
    return (conn, pex)


def full_exit(conn: serial.Serial, status: int):
    conn.reset_input_buffer()
    conn.reset_output_buffer()
    conn.flush()
    conn.close()
    sys.exit(status)


def format_runtime_output(runtime_output: str, section: str) -> None:
    print(f"""
========== {section} runtime output ==========
{runtime_output}
==========--------------------------==========
""")
    return