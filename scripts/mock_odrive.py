import asyncio
from typing import Optional

import serial_asyncio
from pprint import pprint
from math import pi
import socket
from time import sleep
import subprocess


class UartServer(asyncio.Protocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.transport = None
        self.buffer = []


    def connection_made(self, transport: serial_asyncio.SerialTransport):
        self.transport = transport
        print('UartServer serial port opened')
        self.transport.serial.rts = False


    def connection_lost(self, exc: Optional[Exception]):
        print('port closed')
        self.transport.loop.stop()

    def data_received(self, data: bytes):
        self.buffer.append(data.decode())
        contents = "".join(self.buffer)
        if '\n' in contents:
            # print(contents)
            if contents.startswith("f 0"):
                print("fbk0")
                self.transport.write("111111.111 0.0\n".encode())
            elif contents.startswith("f 1"):
                print("fbk1")
                self.transport.write("222222.222 0.0\n".encode())
            else:
                print(contents)
            self.buffer = []


if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    coroutine1 = serial_asyncio.create_serial_connection(loop, UartServer, '/dev/ttyJ1', 921600)
    loop.run_until_complete(coroutine1)
    loop.run_forever()