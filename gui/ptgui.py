# menu.py
import sys
import glob
import tkinter as tk
from tkinter import messagebox
import tk_tools
import pygubu
import serial
import threading
import time
import queue
import re

__VERSION__ = '0.1'

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


class Application(pygubu.TkApplication):

    def _create_ui(self):
        self.dirty=False
        self.port=None
        self.baud=115200
        self.ser=None

        self.ser_open = False;
        self.ser_rcvstack = []

        self.rxqueue = queue.Queue()
        self.txqueue = queue.Queue()
        self.cmdqueue = queue.Queue()

        self.builder = builder = pygubu.Builder()

        builder.add_from_file('menu.ui')

        self.mainwindow = builder.get_object('mainwindow', self.master)
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        self.mainlabel = mainlabel = builder.get_object('lmain', self.master)
        self.builder.tkvariables['strlmain'].set("PwThing Version " + __VERSION__)
        
        self.verlabel = verlabel = builder.get_object('lfwver', self.master)

        self.portcombo = portcombo = builder.get_object('cport', self.master)
        portcombo['values'] = portlist
        portcombo.current(0)
    
        self.master.protocol("WM_DELETE_WINDOW", self.on_close_window)

        self.termbox = termbox = builder.get_object('tterm', self.master)

        self.canvas = canvas = builder.get_object('cvled', self.master)
        self.led = led = tk_tools.Led(canvas, size=15)
        led.pack()
        led.to_red(on=True)

        builder.connect_callbacks(self)
        self.master.after(100, self.process_serial)

       
    def on_mfile_item_clicked(self, itemid):
        if itemid == 'mfile_open':
            messagebox.showinfo('File', 'You clicked Open menuitem')

        if itemid == 'mfile_quit':
            self.on_close_window();

    def on_mtools_item_clicked(self, itemid):
        if itemid == 'mtools_dump':
            messagebox.showinfo('File', 'You clicked Dump menuitem')
        elif itemid == 'mtools_clear':
            messagebox.showinfo('File', 'You clicked Clear menuitem')

    def on_about_clicked(self):
        messagebox.showinfo('About', 'You clicked About menuitem')

    def on_port_selected(self, event):
        self.port = self.builder.tkvariables.__getitem__('tvport').get() 

    def on_rescan_ports(self):
        portlist = serial_ports()
        portlist.insert(0,'')
        self.portcombo['values'] = portlist
        self.portcombo.current(0)

    def setversion(self, s) :
        self.builder.tkvariables['strlver'].set(s)

    def open_serial(self):
        self.ser.rts=True
        self.ser.dtr=True
        self.ser_open = True
        self.led.to_green(on=True)

    def close_serial(self):         
        self.ser.rts=False
        self.ser.dtr=False
        self.ser_open = False
        self.led.to_yellow(on=True)

    def process_serial(self):
        out=""
        if self.ser_open == True:
            while self.ser.inWaiting() > 0:
                c = self.ser.read(1).decode('utf8')
                out += c
                if (c == '\r') or (c == '\n') :
                    self.ser_rcvstack.append(out)
                    self.termbox.insert(tk.INSERT, "!")

            if out != "":
                self.led.to_green(on=True)
#            if len(self.ser_rcvstack) > 0 :
#                s = self.ser_rcvstack.pop()
#                t=s.rstrip()
#                if re.search("^V", t) :
#                    self.setversion(t)
#                self.termbox.insert(tk.INSERT, s)
                self.termbox.insert(tk.INSERT, out)
            self.master.after(20, self.process_serial)
        else:
            self.master.after(100, self.process_serial)
   
    def on_connect(self): 
        if self.port != None:
            if self.ser == None:
                self.ser = serial.Serial(port=self.port, baudrate=self.baud, 
                                         parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, 
                                         timeout=0, writeTimeout=0)
                if self.ser != None:
                    self.open_serial()
            else:
                self.close_serial()
                self.ser = serial.Serial(port=self.port, baudrate=self.baud, 
                                         parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, 
                                         timeout=0, writeTimeout=0)
                if self.ser != None:
                    self.open_serial()
            self.termbox.insert(tk.INSERT, "===========Port %s opened>>>\n"%self.port,"info")
        else:
            messagebox.showerror('Error', 'No port selected')


    def on_close_window(self, event=None):
        if self.dirty:
            msg = messagebox.askquestion ('Exit Configurator','There are unsaved changes.  Are you sure you want to exit',icon = 'warning')
            if msg != 'yes':
                return

        if self.ser != None:
            self.close_serial()
        
        # Call destroy on toplevel to finish program
        self.mainwindow.master.destroy()


if __name__ == '__main__':
    portlist = serial_ports()
    portlist.insert(0,'')
    root = tk.Tk()
    app = Application(root)
    app.run()
    
    