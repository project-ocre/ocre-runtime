#!/usr/bin/env python3
import time
import serial


def main():
    conn = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
    conn.reset_input_buffer()
    conn.send_break(duration=1)
    time.sleep(5)
    print("Cleaning up container hello-world")
    conn.write(b'ocre container ps\n')
    response = conn.read(2048).decode(errors='ignore')
    if ("hello-world" in response):
        conn.write(b'ocre rm hello-world\n')
        response += conn.read(2048).decode(errors='ignore')

    print("==== Runtime Output: ====")
    print(response)
    conn.close()




if __name__ == "__main__":
    main()