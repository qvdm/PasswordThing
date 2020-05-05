# menu.py
import sys
import glob
import tkinter as tk
from tkinter import messagebox
import tk_tools
import pygubu
import serial
import time
import re
import binascii
import string
import secrets

# Versions:
#
# 1. VERSION = GUI SW Version
# 2. EEDVER = EEprom dump version, describes the dump format
# 3. EEVVER = EEprom variable schema version, describes eeprom variables
# 4. PWTVER = Device SW Version
#

__VERSION__ = '0.3'
__EEDVER__ =  4
__EEVVER__ =  2


## TBD: /root/wip/pygubu-develop/pygubu/ui2code.py ./menu.ui > ./menu.py && get rid of pygubu

#TBD: Fix CRC, complete write, fix runtime errors, scroll terminal area, get versions

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
        self.valid=False
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

        self.g_eedver=0
        self.g_eevver=0
        self.g_badver = False

        self.g_ucode=0
        self.g_slotname= []
        self.g_uid = []
        self.g_pwd = []
        self.g_dto = 0
        self.g_ito = 0
        self.g_lto = 0
        self.g_flip = False
        self.g_revert = False
        self.g_logo = False
        self.g_bseq = 0
        self.g_lseq = 0
        self.g_kfail = 0
        self.g_semas = '0000000000000000'
        self.g_sig = 'EFBEADDE'

        self.g_eedata = bytearray()

        self.builder = builder = pygubu.Builder()

        builder.add_from_file('menu.ui')

        self.mainwindow = builder.get_object('mainwindow', self.master)
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        self.mainlabel = mainlabel = builder.get_object('lmain', self.master)
        self.builder.tkvariables['strlmain'].set("PWT Configurator Version " + __VERSION__)
        
        self.verlabel = verlabel = builder.get_object('lvarver', self.master)

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
    
        self.pwframe = pwframe = builder.get_object('fslots', self.master)

        self.vcmd = (self.pwframe.register(self.setdirty), '%P')

        builder.connect_callbacks(self)
        self.master.after(100, self.process_serial)

    def clear_device_data(self) :
        self.g_ucode=0
        self.g_slotname= []
        self.g_uid = []
        self.g_pwd = []
        self.g_dto = 0
        self.g_ito = 0
        self.g_lto = 0
        self.g_flip = False
        self.g_revert = False
        self.g_logo = False
        self.g_bseq = 0
        self.g_lseq = 0
        self.g_kfail = 0


    def enable_button(self, button) :
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(state=tk.NORMAL) 

    def disable_button(self, button) :
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(state=tk.DISABLED) 

    def red_button(self, button) :
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='red')

    def green_button(self, button) :
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='green')

    def black_button(self, button) :
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='black')

    def set_combo_current(self, cbname, indx) :
        comboobj = self.builder.get_object(cbname, self.master)
        comboobj.current(indx)

    def on_port_selected(self, event):
        self.port = self.builder.tkvariables.__getitem__('tvport').get() 
        if self.port :
            self.enable_button('bconnect')
        else :
            self.disable_button('bconnect')

    def set_ser_connected(self) :
        self.disable_button('brescan')
        self.led.to_green(on=True)


    def set_ser_disconnected(self) :
        self.enable_button('brescan')
        self.disable_button('bunlock')
        self.disable_button('bread')
        self.disable_button('bvalidate')
        self.disable_button('bwrite')
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
            #print("01 "+ l)
            if len(l) == 3 :
                t, ev = l[:1], l[1:]
                self.builder.tkvariables['strlver'].set(ev)     
                self.g_eevver = int(ev)
                if self.g_eevver < __EEVVER__ :
                    messagebox.showwarning('Warning', 'Device EEprom schema is too old.  You will not be able to write to the device unless you upgrade its firmware first.')
                    self.g_badver = True

                if t == 'L' :
                    self.locked = True
                    self.red_button('bunlock')
                    self.enable_button('bunlock')
                elif t == 'E' :
                    self.locked = False
                    self.black_button('bunlock')
                    self.disable_button('bunlock')
                    self.enable_button('bread')

                if (self.g_eevver > 1) : # Release available
                    self.master.after(500, self.ask_release)
                else :
                    self.builder.tkvariables['sfwver'].set('N/A')     


            else :
                self.master.after(200, self.get_version)

    def ask_release(self) :
        self.send_serial('R')
        self.master.after(500, self.get_release)

    def get_release(self) :
        if self.serial_avail() :
            l = self.get_serial_line()
            if len(l) >= 3 :
                self.builder.tkvariables['sfwver'].set(l[2:])     
            else :
                self.master.after(200, self.get_release)


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
        self.disable_button('bvalidate')
        self.disable_button('bwrite')
        self.send_serial("EB")  
        self.master.after(500, self.get_dump)

    def get_dump(self) :
        if self.serial_avail() :
            l = self.get_serial_line()
            #print("02 "+ l)
            if l.startswith('V') :
                # Got a dump - parse it
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
        self.g_eedver=int(v[1:]);
        s = self.get_serial_line() # s has the entire dump
        print("PDS " + s + "\n")
        self.get_serial_line()

        self.builder.tkvariables['seedver'].set(str(self.g_eedver).zfill(2))  
        if int(self.g_eedver) != __EEDVER__ :
            messagebox.showerror('Error', 'Incompatible eeprom dump format.')
            self.g_badver = True
            return

        if len(s) > 6 :
            for i in range(8) :
                splitat = 140 # pwd field is 70 bytes
                l, r = s[:splitat], s[splitat:]
                s=r
                pw_sl.append(l)
                #print(l+"\n")
            splitat = 32 # variable field is 16 bytes 
            var_s, r = s[:splitat], s[splitat:]
            #print(var_s+"\n")
            s=r
            splitat = 8 # crc field is 4 bytes
            crc_s, r = s[:splitat], s[splitat:]
            #print(crc_s+"\n")
            s=r
            splitat = 16 # semaphore field is 8 bytes, signature is final 4 bytes
            sem_s, sig_s = s[:splitat], s[splitat:]
            #print(sem_s+"\n")
            #print(sig_s+"\n")

            self.g_semas = sem_s
            self.g_sig = sig_s

            self.builder.tkvariables['sdcrc'].set(crc_s)
            self.builder.tkvariables['ssema'].set(sem_s)
            self.builder.tkvariables['ssig'].set(sig_s)

            self.populate_pw(pw_sl)
            self.populate_var(var_s)
            self.set_crc(crc_s)


    def populate_pw(self, pwlist) :
        for i in range(6) :
            # print(i)
            s = pwlist[i]
            #print('x01 s ' + s + '\n')
            splitat = 2 # userid len and pw len are  2 bytes each
            l, r = s[:splitat], s[splitat:]
            s=r
            #print('x02 ul ' + l + '\n')
            uidlen=int(l, 16)
            l, r = s[:splitat], s[splitat:]
            s=r
            #print('x03 pl ' + l + '\n')
            pwdlen=int(l, 16)
            splitat = 16 # name is 8 bytes
            l, r = s[:splitat], s[splitat:]
            s=r
            l = l.split('00')[0]
            #print('x04 sn ' + l + '\n')
            slotname = binascii.unhexlify(l).decode('utf-8').rstrip()

            splitat = 60 # userid and pw are 30 bytes each
            l, r = s[:splitat], s[splitat:]
            if uidlen :
                #print('x05 ui ' + l + '\n')
                uid = binascii.unhexlify(l).decode('utf-8')[:uidlen]
            else :
                uid = ''
            if pwdlen :
                #print('x06 pw ' + r + '\n')
                pwd = binascii.unhexlify(r).decode('utf-8')[:pwdlen]
            else :
                pwd = ''
            setter="self.builder.tkvariables[\'sename"+str(i)+"\'].set(slotname)" # goes wrong for 0
            eval(setter)
            setter="self.builder.tkvariables[\'suid"+str(i)+"\'].set(uid)"
            eval(setter)
            setter="self.builder.tkvariables[\'spwd"+str(i)+"\'].set(pwd)"
            eval(setter)


    def populate_var(self, vars) :
        r=vars
        l, r = r[:2], r[2:]
        url=bool(int(l, 16)) # display logo flag
        self.builder.tkvariables['bcblogo'].set(url)   

        l, r = r[:2], r[2:]
        dflip=bool(int(l, 16)) # flip display flag
        #print(str(dflip)+'\n')
        self.builder.tkvariables['bcbflip'].set(dflip)   

        l, r = r[:2], r[2:]
        ssec=int(l, 16) # security code index
        if ssec :
            self.builder.tkvariables['strlentry'].set(self.lctable[ssec+1])
        else :
            self.builder.tkvariables['strlentry'].set('')

        l, r = r[:2], r[2:]
        dto=int(l, 16) # display timeout
        #print(str(dto)+'\n')
        self.builder.tkvariables['sedto'].set(dto)   

        l, r = r[:2], r[2:]
        lto=int(l, 16) # led timeout
        self.builder.tkvariables['seito'].set(lto)   

        l, r = r[:2], r[2:]
        bseq=int(l, 16) # button sequence index
        #print(str(bseq)+'\n')
        self.set_combo_current('cbseq', bseq)

        l, r = r[:2], r[2:]
        lseq=int(l, 16) # led sequence index
        self.set_combo_current('clseq', lseq)

        l, r = r[:2], r[2:]
        rpw=bool(int(l, 16)) # pw revert flag
        self.builder.tkvariables['bcbrevert'].set(rpw)   

        l, r = r[:2], r[2:]
        lfail=int(l, 16) # security code fail action index
        self.set_combo_current('ckfail', lfail)

        l, r = r[:2], r[2:]
        slto=int(l, 16) # security lock timeout
        self.builder.tkvariables['selto'].set(slto)   

    def set_crc(self, crc) :
        #TBD
        self.enable_button('bvalidate')
        return

    def set_invalid(self, error) :
        messagebox.showerror('Error', error)
        self.red_button('bvalidate')
        self.disable_button('b_write')



    def on_validate(self) :
    
        snm = re.compile(r"^(?:\w|\-|\ ){0,8}$")
        uim = re.compile(r"^(?:\w|\-|\ |\@|\+|\.){0,30}$")
        pwm = re.compile(r"^(?:[\ -\~]){0,30}$")
        dig = re.compile(r"^(?:\d){0,4}$")

        self.valid=False
        self.clear_device_data()

        ucode = self.builder.tkvariables['strlentry'].get()
        if (len(ucode) > 0) and  ((len(ucode) < 4) or (not int(ucode) in self.lctable) or (int(ucode) == 0) ) :
            self.set_invalid('Unlock code must be exactly 4 digits in [1..3] ')
            return
        if ucode == '' :
            ucodei = 0
        else :
            ucodei = int(ucode)
        self.g_ucode = self.lctable.index(ucodei)
        #print("UC: " + str(self.g_ucode) + "\n")

        for i in range(6) :
            getter="self.builder.tkvariables[\'sename"+str(i)+"\'].get()"
            slotname = eval(getter)
            if re.match(snm, slotname) == None :
                self.set_invalid('Username must be 0 to 8 alphanumeric characters: slot ' + str(i))
                return
            self.g_slotname.append(slotname)
            #print("SN: " + str(i) + " " + self.g_slotname[i] + "\n")

            getter="self.builder.tkvariables[\'suid"+str(i)+"\'].get()"
            uid = eval(getter)
            if re.match(uim, uid) == None :
                self.set_invalid('Userid must be 0 to 30 letters, numbers, @, -, + or .: slot ' + str(i))
                return
            self.g_uid.append(uid)

            getter="self.builder.tkvariables[\'spwd"+str(i)+"\'].get()"
            pwd = eval(getter)
            if re.match(pwm, pwd) == None :
                self.set_invalid('Password must be 0 to 30 printable ASCII characters: slot ' + str(i))
                return
            self.g_pwd.append(pwd)

        # Add 'phantom' entries (reserved space) to make the buffer complete later
        for i in range(2) :
            self.g_slotname.append('')
            self.g_uid.append('')
            self.g_pwd.append('')
        

        dto = self.builder.tkvariables['sedto'].get()
        if ((re.match(dig, dto) == None) or (int(dto) > 255) ) :
            self.set_invalid('Display timeout must be 0 or less than 256 ')
            return
        self.g_dto = int(dto)
        #print("DT: " + str(self.g_dto) + "\n")

        ito = self.builder.tkvariables['seito'].get()
        if ((re.match(dig, ito) == None) or (int(ito) > 255) ) :
            self.set_invalid('LED timeout must be 0 or less than 256 ')
            return
        self.g_ito = int(ito)

        lto = self.builder.tkvariables['selto'].get()
        if ((re.match(dig, lto) == None) or (int(lto) > 255) ) :
            self.set_invalid('Lock timeout must be 0 or less than 256 ')
            return
        self.g_lto = int(lto)

        self.g_flip = self.builder.tkvariables['bcbflip'].get()
        #print("FL: " + str(self.g_flip) + "\n")
        self.g_revert = self.builder.tkvariables['bcbrevert'].get()
        self.g_logo = self.builder.tkvariables['bcblogo'].get()

        cbseqobj = self.builder.get_object('cbseq', self.master)
        self.g_bseq = cbseqobj.current()
        #print("BS: " + str(self.g_bseq) + "\n")

        clseqobj = self.builder.get_object('clseq', self.master)
        self.g_lseq = clseqobj.current()

        ckfailobj = self.builder.get_object('ckfail', self.master)
        self.g_kfail = ckfailobj.current()

        self.g_eedata = bytearray()

        for i in range(8) : # include phantom entries
            self.g_eedata += bytearray(len(self.g_uid[i]).to_bytes(1, byteorder='little'))
            self.g_eedata += bytearray(len(self.g_pwd[i]).to_bytes(1, byteorder='little'))
            self.g_eedata += self.g_slotname[i].ljust(8,'\0').encode('utf-8')
            self.g_eedata += self.g_uid[i].ljust(30,'\0').encode('utf-8')
            self.g_eedata += self.g_pwd[i].ljust(30,'\0').encode('utf-8')

        self.g_eedata += bytearray(self.g_logo.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_flip.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_ucode.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_dto.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_ito.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_bseq.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_lseq.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_revert.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_kfail.to_bytes(1, byteorder='little'))
        self.g_eedata += bytearray(self.g_lto.to_bytes(1, byteorder='little'))
        # Pad out unused variables (6)
        zero = 0
        self.g_eedata += bytearray(zero.to_bytes(6, byteorder='little'))

        crc = self.calc_crc()
        crcb = crc.to_bytes(4, byteorder='little')
        self.builder.tkvariables['slcrc'].set(binascii.hexlify(crcb))
        self.g_eedata += crcb

        print(binascii.hexlify(self.g_eedata))

        if not self.g_badver :
            self.enable_button('bwrite')
        self.green_button('bvalidate')
        self.valid=True
       
    def on_write(self) :

        self.on_validate()
        if self.valid :
            #TBD
            print("EESV"+self.g_eesver+"\n")
            buf = "V%02X\n" % int(self.g_eesver)


        return

    def setdirty(self) :
        self.valid=False
        self.disable_button('bwrite')
        self.black_button('bvalidate')
        #print("SetDirty\n")
        return True

    def on_reset(self) :
        self.clear_device_data()
        # TBD handle reset button
        return

    def on_clear(self) :
        # TBD handle clear button
        return

    def on_close_window(self, event=None):
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

    def calc_crc(self) :
        
        crc_table =  [0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
                      0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                      0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c ]
        crc = 0xFFFFFFFF;

        for index in range(576) :
            eebyte = self.g_eedata[index]
            
            crc = crc_table[(crc ^ eebyte) & 0x0f] ^ (crc >> 4);
            crc = crc_table[(crc ^ (eebyte >> 4)) & 0x0f] ^ (crc >> 4);
            crc = ~crc;

        return crc & 0xFFFFFFFF;


    def generate_pw(self) :
        mode = self.builder.tkvariables['sgmode'].get()
        len = int(self.builder.tkvariables['selen'].get())

        if (len < 6) or (len > 28) :
            messagebox.showwarning('Warning', 'Selected password length out of range - adjusting')
            if (len < 6) :
                len = 6
                self.builder.tkvariables['selen'].set(str(6))
            elif (len > 28) :
                len = 28
                self.builder.tkvariables['selen'].set(str(28))
  
        if mode == 'Numeric' : 
            return ''.join(secrets.choice(string.digits) for i in range(len))
        elif mode == 'Alpha' :
            return ''.join(secrets.choice(string.ascii_letters) for i in range(len))
        elif mode == 'Alphanumeric' :
            return ''.join(secrets.choice(string.ascii_letters+string.digits) for i in range(len))
        elif mode == 'Symbols' :
            return ''.join(secrets.choice(string.ascii_letters+string.digits+string.punctuation) for i in range(len))
        return ''


    def on_mfile_item_clicked(self, itemid):
        if itemid == 'mfile_open':
            #messagebox.showinfo('File', 'You clicked Open menuitem')
            options = {}
            f = tkinter.filedialog.askopenfile(mode="r", **options)
        elif itemid == 'mfile_save':
            messagebox.showinfo('File', 'You clicked Save menuitem')
        elif itemid == 'mfile_quit':
            self.on_close_window();

    def on_mtools_item_clicked(self, itemid):
        if itemid == 'mtools_dump':
            messagebox.showinfo('File', 'You clicked Dump menuitem')
        elif itemid == 'mtools_clear':
            messagebox.showinfo('File', 'You clicked Clear menuitem')

    def on_about_clicked(self):
        messagebox.showinfo('About', 'You clicked About menuitem')

    # Copy/paste/clear/generate handlers
    #  need to do in a stupid way due to pygubu limitations - refactor when changing UI framework
    def on_copy0(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename0'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid0'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd0'].get())

    def on_paste0(self) :
        self.builder.tkvariables['sename0'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid0'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd0'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear0(self) :
        self.builder.tkvariables['sename0'].set('')
        self.builder.tkvariables['suid0'].set('')
        self.builder.tkvariables['spwd0'].set('')

    def on_generate0(self) :
        self.builder.tkvariables['spwd0'].set(self.generate_pw())

    def on_copy1(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename1'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid1'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd1'].get())

    def on_paste1(self) :
        self.builder.tkvariables['sename1'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid1'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd1'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear1(self) :
        self.builder.tkvariables['sename1'].set('')
        self.builder.tkvariables['suid1'].set('')
        self.builder.tkvariables['spwd1'].set('')

    def on_generate1(self) :
        self.builder.tkvariables['spwd1'].set(self.generate_pw())

    def on_copy2(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename2'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid2'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd2'].get())

    def on_paste2(self) :
        self.builder.tkvariables['sename2'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid2'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd2'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear2(self) :
        self.builder.tkvariables['sename2'].set('')
        self.builder.tkvariables['suid2'].set('')
        self.builder.tkvariables['spwd2'].set('')

    def on_generate2(self) :
        self.builder.tkvariables['spwd2'].set(self.generate_pw())

    def on_copy3(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename3'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid3'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd3'].get())

    def on_paste3(self) :
        self.builder.tkvariables['sename3'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid3'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd3'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear3(self) :
        self.builder.tkvariables['sename3'].set('')
        self.builder.tkvariables['suid3'].set('')
        self.builder.tkvariables['spwd3'].set('')

    def on_generate3(self) :
        self.builder.tkvariables['spwd3'].set(self.generate_pw())     

    def on_copy4(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename4'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid4'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd4'].get())

    def on_paste4(self) :
        self.builder.tkvariables['sename4'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid4'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd4'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear4(self) :
        self.builder.tkvariables['sename4'].set('')
        self.builder.tkvariables['suid4'].set('')
        self.builder.tkvariables['spwd4'].set('')

    def on_generate4(self) :
        self.builder.tkvariables['spwd4'].set(self.generate_pw())

    def on_copy5(self) :
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename5'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid5'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd5'].get())

    def on_paste5(self) :
        self.builder.tkvariables['sename5'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid5'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd5'].set(self.builder.tkvariables['spwdcb'].get())

    def on_clear5(self) :
        self.builder.tkvariables['sename5'].set('')
        self.builder.tkvariables['suid5'].set('')
        self.builder.tkvariables['spwd5'].set('')

    def on_generate5(self) :
        self.builder.tkvariables['spwd5'].set(self.generate_pw())


if __name__ == '__main__':
    portlist = serial_ports()
    portlist.insert(0,'')
    root = tk.Tk()
    app = Application(root)
    app.run()
    
    
