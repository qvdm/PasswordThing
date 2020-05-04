import serial
import sys
import glob
import time


if sys.platform.startswith('win'):
    ports = ['COM%s' % (i + 1) for i in range(256)]
elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
    ports = glob.glob('/dev/tty[A-Za-z]*')
elif sys.platform.startswith('darwin'):
    ports = glob.glob('/dev/tty.*')
else:
    raise EnvironmentError('Unsupported platform')

comport = ''
for port in ports:
    try:
        s = serial.Serial(port, 115200, timeout=0.5)
        print(port)
        time.sleep(0.1)
        print(" open ")
        time.sleep(0.5)
        resp = s.readline()
        print(resp, "\n")
        comport = port
        if (resp != ''):
            comport = port
            break
    except (OSError, serial.SerialException):
        pass

def ser_write(str):
    time.sleep(0.1)
    s.write(str.encode())

def ser_read():
    time.sleep(0.1)
    return s.readline()

if (comport != ''):
    resp = ser_read()
    print (resp, "\n")
    resp = ser_read()
    print (resp, "\n")
    ser_write("V\r")
    resp = ser_read()
    print (resp, "\n")
    resp = ser_read()
    print (resp, "\n")
    resp = ser_read()
    print (resp, "\n")
