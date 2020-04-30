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
        self.ser_rcv=""
        self.ser_rcvstack = []
        self.ser_to = 0

        self.ser_parsestring = ""



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
        if self.ser_open == False :
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
        self.led.to_yellow(on=True)

    def close_serial(self):         
        self.ser.rts=False
        self.ser.dtr=False
        self.ser_open = False
        self.led.to_red(on=True)

    def process_serial(self):
        if self.ser_open == True:
            self.ser_to = self.ser_to + 1;
            while self.ser.inWaiting() > 0:
                self.ser_to = 0
                c = self.ser.read(1).decode('utf8')
                self.ser_rcv += c
                #self.dbug("C/RCV:" + c + ":" + self.ser_rcv)
                if "\n" in self.ser_rcv :
                    self.ser_rcv = self.cleanup_str(self.ser_rcv) 
                    if len(self.ser_rcv) > 0 :
                      self.ser_rcvstack.append(self.ser_rcv)
                      self.led.to_green(on=True)
                      #self.dbug("!CR:" + self.ser_rcv + "L:" + str(len(self.ser_rcvstack)))
                    #else :
                      #self.dbug("!CBL")
                    self.termbox.insert(tk.INSERT, self.ser_rcv + "\n")
                    self.ser_rcv=""

            if (self.ser_to > 20) and (len(self.ser_rcv) > 0) :
                self.ser_rcv = self.cleanup_str(self.ser_rcv) 
                if len(self.ser_rcv) > 0 :
                    self.ser_rcvstack.append(self.ser_rcv)
                    self.led.to_green(on=True)
                    #self.dbug("!TO:" + self.ser_rcv + "L:" + str(len(self.ser_rcvstack)))
                #else :
                    #self.dbug("!TBL")
                self.termbox.insert(tk.INSERT, self.ser_rcv)
                self.ser_rcv=""

            self.master.after(20, self.process_serial)
        else:
            self.master.after(100, self.process_serial)
   
    def clear_serial(self) :
        while len(self.ser_rcvstack) > 0 :
            self.ser_rcvstack.pop(0)

    def get_serial_line(self) :
        l=""
        #self.dbug("@"+str(len(self.ser_rcvstack)))
        if len(self.ser_rcvstack) > 0 :
            l = self.ser_rcvstack.pop(0)
        return l

    def serial_avail(self) :
        if len(self.ser_rcvstack) > 0 :
            return True
        return False

    def send_serial(self, str) :
        if self.ser_open == True :
            s=str + "\r"
            self.ser.write(s.encode("utf-8"))

    def on_connect(self): 
        if self.ser_open :
            self.close_serial()
            self.builder.tkvariables['strbconn'].set('Connect') 
        else :
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
                self.builder.tkvariables['strbconn'].set('Disconnect') 
                self.termbox.insert(tk.INSERT, "===========Port %s opened>>>\n"%self.port,"info")
                self.ser_parsestring = ""
                self.send_serial('')
                self.master.after(500, self.ask_version)
            else:
                messagebox.showerror('Error', 'No port selected')


    def ask_version(self) :
        self.send_serial('V')
        self.master.after(500, self.get_version)

    def get_version(self) :
        if self.serial_avail() :
            l = self.get_serial_line()
            #print("01 "+ l)
            if l.startswith('E') :
                self.builder.tkvariables['strlver'].set(l)     

            self.master.after(200, self.get_version)

    def on_sendcr(self):
        if self.ser_open == True :
            self.ser.write("\r".encode("utf-8"))
       

    def on_read(self) :
        self.send_serial("EB")  
        self.master.after(500, self.get_dump)

    def get_dump(self) :
        if self.serial_avail() :
            l = self.get_serial_line()
            #print("02 "+ l)
            if l.startswith('V') :
                self.master.after(200, self.parse_dump, l);
            else :
                self.master.after(200, self.get_dump)

    def parse_dump(self, v) :
        pw_sl = []
        var_s=''
        crc_s=''
        sem_s=''
        sig_s=''

        print("PDV " + v + "\n");
        s = self.get_serial_line()
        print("PDS " + s + "\n")
        self.get_serial_line()
        if len(s) > 6 :
            for i in range(8) :
                splitat = 140
                l, r = s[:splitat], s[splitat:]
                s=r
                pw_sl.append(l)
                print(l+"\n")
            splitat = 32
            var_s, r = s[:splitat], s[splitat:]
            print(var_s+"\n")
            s=r
            splitat = 8
            crc_s, r = s[:splitat], s[splitat:]
            print(crc_s+"\n")
            s=r
            splitat = 16
            sem_s, sig_s = s[:splitat], s[splitat:]
            print(sem_s+"\n")
            print(sig_s+"\n")

        






    def on_close_window(self, event=None):
        if self.dirty:
            msg = messagebox.askquestion ('Exit Configurator','There are unsaved changes.  Are you sure you want to exit',icon = 'warning')
            if msg != 'yes':
                return

        if self.ser != None:
            self.close_serial()
        
        # Call destroy on toplevel to finish program
        self.mainwindow.master.destroy()

    def cleanup_str(self, str) :
        p = re.compile('\r|\n')
        str = p.sub("", str) 
        p = re.compile('^\s+')
        str = p.sub("", str) 
        p = re.compile('\s+$')
        str = p.sub("", str) 
        return str
    
    def dbug(self, str) :
        s=str.replace("\n", "[LF]")
        s=s.replace("\r", "[CR]")
        s=s.replace(" ", "[SPC]")
        print("$"+s+"\n")

if __name__ == '__main__':
    portlist = serial_ports()
    portlist.insert(0,'')
    root = tk.Tk()
    app = Application(root)
    app.run()
    
    