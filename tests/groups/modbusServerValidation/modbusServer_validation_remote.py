#!/usr/bin/env python3

import serial
import time
import sys
from pymodbus.client import ModbusTcpClient

def exitSafe(conn: serial.Serial, client: ModbusTcpClient, exitCode: int):
    conn.close()
    client.close()
    sys.exit(exitCode)

"""
This testcase is to be used following a modbus server being installed on a board.

The testcase forms a serial connection to the board, sends a break to start the Modbus server,
and reads / writes to registers on the modbus server through a connection on the testing agent  
"""

def main():
    print("starting Modbus server:")

    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    conn.send_break(duration=1)

    # Wait for modbus server to be up
    time.sleep(5)

    print("----* Reading client connection status *----")
    client_remote = ModbusTcpClient("ocre-b-u585i.lfedge.iol.unh.edu", port=1502)
    client_remote.connect()

    connection_results = [client_remote.connected, client_remote.is_socket_open()]

    print(connection_results)
    if (connection_results != [True, True]):
        exitSafe(conn, client_remote, 1)

    print("----* Testing LED Control Register *----")

    led_results = []

    led_results.append(client_remote.read_holding_registers(0x00).registers[0]) # 0
    client_remote.write_register(0x00, 0x01)
    time.sleep(5)
    led_results.append(client_remote.read_holding_registers(0x00).registers[0]) # 1
    client_remote.write_register(0x00, 0x02)
    time.sleep(5)
    led_results.append(client_remote.read_holding_registers(0x00).registers[0]) # 2
    client_remote.write_register(0x00, 0x00)
    time.sleep(5)
    led_results.append(client_remote.read_holding_registers(0x00).registers[0]) # 0

    print(led_results)
    if (led_results != [0,1,2,0]):
        exitSafe(conn, client_remote, 1)


    print("----* Test Button Press Count Register *----")
    button_result = (client_remote.read_holding_registers(0x01).registers[0]) # 0

    if button_result != 0:
        exitSafe(conn, client_remote, 1)
        

    print("----* Waiting for updates to Modbus server container before continuing *---")

    # print("----* Test reading 32 bit floats *----") 
    # print("Acceleration X")
    # print(client_remote.convert_from_registers([0x04, 0x05], client_remote.DATATYPE.FLOAT32, word_order="little"))
    # print(client_remote.read_holding_registers(0x04).registers)
    # print(client_remote.read_holding_registers(0x05).registers)
    # print(client_remote.read_input_registers(0x04).registers)
    # print(client_remote.read_input_registers(0x05).registers)

    
    # print("Gyroscope X")
    # print(client_remote.convert_from_registers([0x08, 0x09], client_remote.DATATYPE.FLOAT32, word_order="little"))
    # print(client_remote.read_holding_registers(0x08).registers)
    # print(client_remote.read_holding_registers(0x09).registers)
    # print(client_remote.read_input_registers(0x08).registers)
    # print(client_remote.read_input_registers(0x09).registers)

    
    # print("Humidity")
    # print(client_remote.convert_from_registers([0x14, 0x15], client_remote.DATATYPE.FLOAT32, word_order="little"))
    # print(client_remote.read_holding_registers(0x14).registers)
    # print(client_remote.read_holding_registers(0x15).registers)
    # print(client_remote.read_input_registers(0x14).registers)
    # print(client_remote.read_input_registers(0x15).registers)

    # print("Temperature")
    # print(client_remote.convert_from_registers([0x16, 0x17], client_remote.DATATYPE.FLOAT32, word_order="little"))
    # print(client_remote.read_holding_registers(0x16).registers)
    # print(client_remote.read_holding_registers(0x17).registers)
    # print(client_remote.read_input_registers(0x16).registers)
    # print(client_remote.read_input_registers(0x17).registers)

    print("----* Closing Connection *----")
    exitSafe(conn, client_remote, 0)

if __name__ == "__main__":
    main()


