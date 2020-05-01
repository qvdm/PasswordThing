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
import binascii

__VERSION__ = '0.2'
__EEVER__ =  '01'

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

        self.ser_open = False
        self.ser_connected = False
        self.ser_rcv=""
        self.ser_rcvstack = []
        self.ser_to = 0
        self.locked = False

        self.ser_parsestring = ""

        self.lctable = [0,
            1111,1112,1113,1121,1122,1123,1131,1132,1133,1211,1212,1213,1221,1222,1223,1231,1232,1233,1311,1312,1313,1321,1322,1323,1331,1332,1333,
            2111,2112,2113,2121,2122,2123,2131,2132,2133,2211,2212,2213,2221,2222,2223,2231,2232,2233,2311,2312,2313,2321,2322,2323,2331,2332,2333,
            3111,3112,3113,3121,3122,3123,3131,3132,3133,3211,3212,3213,3221,3222,3223,3231,3232,3233,3311,3312,3313,3321,3322,3323,3331,3332,3333 ]

        self.rxqueue = queue.Queue()
        self.txqueue = queue.Queue()
        self.cmdqueue = queue.Queue()

        self.builder = builder = pygubu.Builder()

        builder.add_from_file('menu.ui')

        self.mainwindow = builder.get_object('mainwindow', self.master)
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        self.mainlabel = mainlabel = builder.get_object('lmain', self.master)
        self.builder.tkvariables['strlmain'].set("PWT Configurator Version " + __VERSION__)
        
        self.verlabel = verlabel = builder.get_object('lfwver', self.master)

        self.portcombo = portcombo = builder.get_object('cport', self.master)
        self.portcombo['values'] = portlist
        self.portcombo.current(0)

        self.gencombo = gencombo = builder.get_object('cmode', self.master)
        self.gencombo.current(2)
        self.builder.tkvariables['selen'].set('20') 

    
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
        connect_button = self.builder.get_object('bconnect', self.master)

        self.port = self.builder.tkvariables.__getitem__('tvport').get() 
        if self.port :
            self.enable_button(connect_button)
        else :
            self.disable_button(connect_button)

    def enable_button(self, button) :
        button.config(state=tk.NORMAL) 

    def disable_button(self, button) :
        button.config(state=tk.DISABLED) 

    def set_ser_connected(self) :
        rbutton = self.builder.get_object('brescan', self.master)
        rbutton.config(state=tk.DISABLED)
        self.led.to_green(on=True)


    def set_ser_disconnected(self) :
        rbutton = self.builder.get_object('brescan', self.master)
        rbutton.config(state=tk.NORMAL)
        ulbutton = self.builder.get_object('bunlock', self.master)
        ulbutton.config(fg='black', state=tk.DISABLED)
        rdbutton = self.builder.get_object('bread', self.master)
        rdbutton.config(state=tk.DISABLED)

        self.builder.tkvariables['strbconn'].set('Connect') 
        self.led.to_red(on=True)

    def set_ser_open(self) :
        self.builder.tkvariables['strbconn'].set('Disconnect') 
        self.led.to_yellow(on=True)


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
        self.set_ser_open()

    def close_serial(self):         
        self.ser.rts=False
        self.ser.dtr=False
        self.ser_open = False
        self.set_ser_disconnected()

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
                      self.set_ser_connected()
                      #self.dbug("!CR:" + self.ser_rcv + "L:" + str(len(self.ser_rcvstack)))
                    #else :
                      #self.dbug("!CBL")
                    self.termbox.insert(tk.INSERT, self.ser_rcv + "\n")
                    self.ser_rcv=""

            if (self.ser_to > 20) and (len(self.ser_rcv) > 0) :
                self.ser_rcv = self.cleanup_str(self.ser_rcv) 
                if len(self.ser_rcv) > 0 :
                    self.ser_rcvstack.append(self.ser_rcv)
                    self.set_ser_connected()
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
            print("01 "+ l)
            if len(l) == 3 :
                t, ev = l[:1], l[1:]
                self.builder.tkvariables['strlver'].set(ev)     
                if (ev != __EEVER__ ) : 
                    messagebox.showerror('Error', 'Incompatible EE schema ' + ev + ' vs ' + __EEVER__)
                    return

                ulbutton = self.builder.get_object('bunlock', self.master)
                if t == 'L' :
                    self.locked = True
                    ulbutton.config(fg='red', state=tk.NORMAL)
                elif t == 'E' :
                    self.locked = False
                    ulbutton.config(fg='black', state=tk.DISABLED)
                    rdbutton = self.builder.get_object('bread', self.master)
                    rdbutton.config(state=tk.NORMAL)
            else :
                self.master.after(200, self.get_version)

    def lc_valid(self, code, search=re.compile(r'^[1-3]{4}').search) :
        return bool(search(code))

    def on_unlock(self):
        if self.locked :
            entry = self.builder.tkvariables['strlentry'].get()
            if (len(entry) < 4) or not self.lc_valid(entry) :
                messagebox.showerror('Error', 'Code must be 4 digits in [1..3]')
                return

            self.send_serial(entry)
            self.master.after(200, self.ask_version)
       

    def on_read(self) :
        self.send_serial("EB")  
        self.master.after(500, self.get_dump)

    def get_dump(self) :
        if self.serial_avail() :
            l = self.get_serial_line()
            print("02 "+ l)
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

            self.populate_pw(pw_sl)
            self.populate_var(var_s)
            self.set_crc(crc_s)

    def populate_pw(self, pwlist) :
        for i in range(6) :
            s = pwlist[i]
            #print('x01 s ' + s + '\n')
            splitat = 2
            l, r = s[:splitat], s[splitat:]
            s=r
            #print('x02 ul ' + l + '\n')
            uidlen=int(l, 16)
            l, r = s[:splitat], s[splitat:]
            s=r
            #print('x03 pl ' + l + '\n')
            pwdlen=int(l, 16)
            splitat = 16
            l, r = s[:splitat], s[splitat:]
            s=r
            l = l.split('00')[0]
            #print('x04 sn ' + l + '\n')
            slotname = binascii.unhexlify(l).decode('utf-8')

            splitat = 60
            l, r = s[:splitat], s[splitat:]
            if uidlen :
                #print('x05 ui ' + l + '\n')
                uid = binascii.unhexlify(l).decode('utf-8')
            else :
                uid = ''
            if pwdlen :
                #print('x06 pw ' + r + '\n')
                pwd = binascii.unhexlify(r).decode('utf-8')
            else :
                pwd = ''
            setter="self.builder.tkvariables[\'sename"+str(i)+"\'].set(slotname)"
            eval(setter)
            setter="self.builder.tkvariables[\'suid"+str(i)+"\'].set(uid)"
            eval(setter)
            setter="self.builder.tkvariables[\'spwd"+str(i)+"\'].set(pwd)"
            eval(setter)


    def populate_var(self, vars) :
        r=vars
        l, r = r[:2], r[2:]
        url=bool(int(l, 16)) 
        self.builder.tkvariables['bcblogo'].set(url)   

        l, r = r[:2], r[2:]
        dflip=bool(int(l, 16))            
        print(str(dflip)+'\n')
        self.builder.tkvariables['bcbflip'].set(dflip)   

        l, r = r[:2], r[2:]
        ssec=int(l, 16)
        if ssec :
            self.builder.tkvariables['strlentry'].set(self.lctable[ssec+1])
        else :
            self.builder.tkvariables['strlentry'].set('')

        l, r = r[:2], r[2:]
        dto=int(l, 16)
        print(str(dto)+'\n')
        self.builder.tkvariables['sedto'].set(dto*10)   

        l, r = r[:2], r[2:]
        lto=int(l, 16)
        self.builder.tkvariables['seito'].set(lto*10)   

        l, r = r[:2], r[2:]
        bseq=int(l, 16)
        print(str(bseq)+'\n')
        bscombo = self.builder.get_object('cbseq', self.master)
        bscombo.current(bseq)
        
        l, r = r[:2], r[2:]
        lseq=int(l, 16)
        clcombo = self.builder.get_object('clseq', self.master)
        clcombo.current(lseq)

        l, r = r[:2], r[2:]
        rpw=bool(int(l, 16))    
        self.builder.tkvariables['bcbrevert'].set(rpw)   

        l, r = r[:2], r[2:]
        ltry=int(l, 16)
        self.builder.tkvariables['sektry'].set(ltry)   #!! remove and merge with fail

        l, r = r[:2], r[2:]
        lfail=int(l, 16)
        ufcombo = self.builder.get_object('ckfail', self.master)
        ufcombo.current(lfail)

    def set_crc(self, crc) :
        return


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


#def pwgen_alnum(length=20):
#    chars = string.ascii_uppercase + string.digits + string.ascii_lowercase
#    password = ''
#    for i in range(length):
#       password += chars[ord(os.urandom(1)) % len(chars)]
#    return password

if __name__ == '__main__':
    portlist = serial_ports()
    portlist.insert(0,'')
    root = tk.Tk()
    app = Application(root)
    app.run()
    
    