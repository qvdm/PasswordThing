#############################################################################################
#
# PTCONFIG.PY - PasswordThing configurator
#
# PT needs to be in serial mode (hold middle button while plugging into USB port,
#   LED should be white)
#
# Building EXE:  >C:\Python38\Scripts\pyinstaller.exe --onefile --windowed --icon=pt.ico ptconfig.py
#
# Future enhancements: ##  pygubu-develop/pygubu/ui2code.py ./ptconfig.ui > ./uo.py &&
#   get rid of pygubu
#
# TODO:  Set main window title to something other than TK
#
# Bugs: fix runtime errors,
#       read enable not always properly called
#       Pylint
#       Change messages to scrolling box, add more
#       Document operation
#       Comment functions
#
# Notes:
# 


import binascii
import glob
import re
import secrets
import string
import sys
import tkinter as tk
from tkinter import filedialog
from tkinter import messagebox

import pygubu
import serial
import tk_tools

# Versions:
#
# 1. VERSION = GUI SW Version
# 2. EEDVER = EEprom dump version, describes the dump format
# 3. EEVVER = EEprom variable schema version, describes eeprom variables
# 4. PWTVER = Device SW Version
#

__VERSION__ = '0.3'
__EEDVER__ = 4
__EEVVER__ = 2


###
# GUI App Class
class Application(pygubu.TkApplication):

    ###
    # GUI Entry point
    def _create_ui(self):
        self.g_port = None
        self.g_serial = None
        self.g_ser_rcv = ""
        self.g_ser_rcvstack = []
        self.g_ser_to = 0
        self.g_locked = False
        self.g_ser_active = False;

        self.g_valid = False

        self.g_ser_parsestring = ""

        self.g_lctable = [0,
                          1111, 1112, 1113, 1121, 1122, 1123, 1131, 1132, 1133, 1211, 1212, 1213, 1221,
                          1222, 1223, 1231, 1232, 1233, 1311, 1312, 1313, 1321, 1322, 1323, 1331, 1332,
                          1333, 2111, 2112, 2113, 2121, 2122, 2123, 2131, 2132, 2133, 2211, 2212, 2213,
                          2221, 2222, 2223, 2231, 2232, 2233, 2311, 2312, 2313, 2321, 2322, 2323, 2331,
                          2332, 2333, 3111, 3112, 3113, 3121, 3122, 3123, 3131, 3132, 3133, 3211, 3212,
                          3213, 3221, 3222, 3223, 3231, 3232, 3233, 3311, 3312, 3313, 3321, 3322, 3323,
                          3331, 3332, 3333]

        self.g_eedver = 0
        self.g_eevver = 0
        self.g_badver = False

        self.g_rdb_ver = ''
        self.g_rdb_body = ''

        self.g_ucode = 0
        self.g_slotname = []
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
        self.g_wrbuf = ''

        self.builder = builder = pygubu.Builder()

        builder.add_from_file('ptconfig.ui')

        self.mainwindow = builder.get_object('mainwindow', self.master)
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        self.mainlabel = builder.get_object('lmain', self.master)
        self.builder.tkvariables['strlmain'].set("PWT Configurator Version " + __VERSION__)

        self.verlabel = builder.get_object('lvarver', self.master)

        self.portcombo = builder.get_object('cport', self.master)
        self.portcombo['values'] = u_portlist
        self.portcombo.current(0)

        self.gencombo = builder.get_object('cmode', self.master)
        self.gencombo.current(2)
        self.builder.tkvariables['selen'].set('20')

        self.master.protocol("WM_DELETE_WINDOW", self.on_close_window)

        self.termbox = builder.get_object('tterm', self.master)
        self.eebox = builder.get_object('tee', self.master)
        self.msgbox = builder.get_object('tmessage', self.master)

        self.canvas = canvas = builder.get_object('cvled', self.master)
        self.led = led = tk_tools.Led(canvas, size=15)
        led.pack()
        led.to_red(on=True)

        self.pwframe = builder.get_object('fslots', self.master)
        
        builder.connect_callbacks(self)
        self.master.after(100, self.process_serial)

    def clear_device_data(self):
        self.g_ucode = 0
        self.g_slotname = []
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

    def enable_button(self, button):
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(state=tk.NORMAL)

    def disable_button(self, button):
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(state=tk.DISABLED)

    def red_button(self, button):
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='red')

    def green_button(self, button):
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='green')

    def black_button(self, button):
        buttonobj = self.builder.get_object(button, self.master)
        buttonobj.config(fg='black')

    def set_combo_current(self, cbname, indx):
        comboobj = self.builder.get_object(cbname, self.master)
        comboobj.current(indx)

    def disp_message(self, message):
        self.msgbox.insert(tk.END, message + "\n")
        self.msgbox.see(tk.END)


    def on_port_selected(self, event):
        if self.g_serial:
            self.close_serial()
        self.g_port = self.builder.tkvariables.__getitem__('tvport').get()
        if self.g_port:
            self.enable_button('bconnect')
        else:
            self.disable_button('bconnect')

    def set_ser_connected(self):
        self.disable_button('brescan')
        self.builder.tkvariables['strbconn'].set('Disconnect')
        self.led.to_yellow(on=True)
        self.disp_message("Serial connected, not active")


    def set_ser_active(self):
        self.disable_button('brescan')
        self.led.to_green(on=True)
        if (not self.g_ser_active) :
            self.disp_message("Serial Active")
        self.g_ser_active = True

    def set_ser_disconnected(self):
        self.enable_button('brescan')
        self.disable_button('bunlock')
        self.disable_button('bread')
        self.disable_button('bvalidate')
        self.disable_button('bwrite')
        self.builder.tkvariables['strbconn'].set('Connect')
        self.led.to_red(on=True)
        self.clear_device_data()
        self.disp_message("Serial Disconnected")
        self.g_ser_active = False;

    def on_rescan_ports(self):
        if not self.g_serial:
            self.u_portlist = scan_serial_ports()
            self.u_portlist.insert(0, '')
            self.portcombo['values'] = self.u_portlist
            self.portcombo.current(0)
            self.g_ser_active = False;
            

    def setversion(self, s):
        self.builder.tkvariables['strlver'].set(s)

    def open_serial(self):
        if self.g_serial:
            self.close_serial()
        if self.g_port is not None:
            self.g_serial = serial.Serial(port=self.g_port, baudrate=115200,
                                          parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                                          bytesize=serial.EIGHTBITS,
                                          timeout=0, writeTimeout=0)
            if self.g_serial:
                self.g_serial.rts = True
                self.g_serial.dtr = True
                self.set_ser_connected()
        else:
            messagebox.showerror('Error', 'No port selected')

    def close_serial(self):
        if self.g_serial:
            self.g_serial.rts = False
            self.g_serial.dtr = False
            self.g_serial.close()
        self.g_serial = None
        self.set_ser_disconnected()

    def process_serial(self):
        if self.g_serial:
            self.g_ser_to = self.g_ser_to + 1
            while self.g_serial.inWaiting() > 0:
                self.g_ser_to = 0
                c = self.g_serial.read(1).decode('utf8')
                self.g_ser_rcv += c
                # self.dbug("C/RCV:" + c + ":" + self.g_ser_rcv)
                if "\n" in self.g_ser_rcv:
                    self.g_ser_rcv = self.cleanup_str(self.g_ser_rcv)
                    if len(self.g_ser_rcv) > 0:
                        self.g_ser_rcvstack.append(self.g_ser_rcv)
                        self.set_ser_active()
                        if self.g_ser_rcv.startswith('#') and not self.g_locked:
                            self.set_locked()
                        elif self.g_ser_rcv.startswith('>') and self.g_locked:
                            self.set_unlocked()
                        # self.dbug("!CR:" + self.g_ser_rcv + "L:" + str(len(self.g_ser_rcvstack)))
                    # else:
                    # self.dbug("!CBL")
                    self.termbox.insert(tk.END, self.g_ser_rcv + "\n")
                    self.termbox.see(tk.END)
                    self.g_ser_rcv = ""

            if (self.g_ser_to > 20) and (len(self.g_ser_rcv) > 0):
                self.g_ser_rcv = self.cleanup_str(self.g_ser_rcv)
                if len(self.g_ser_rcv) > 0:
                    self.g_ser_rcvstack.append(self.g_ser_rcv)
                    self.set_ser_active()
                    # self.dbug("!TO:" + self.g_ser_rcv + "L:" + str(len(self.g_ser_rcvstack)))
                # else:
                # self.dbug("!TBL")
                self.termbox.insert(tk.END, self.g_ser_rcv)
                self.termbox.see(tk.END)
                self.g_ser_rcv = ""

            self.master.after(20, self.process_serial)
        else:
            self.master.after(100, self.process_serial)

    def clear_serial(self):
        if self.g_serial:
            while len(self.g_ser_rcvstack) > 0:
                self.g_ser_rcvstack.pop(0)

    def get_serial_line(self):
        if self.g_serial:
            l = ""
            if len(self.g_ser_rcvstack) > 0:
                l = self.g_ser_rcvstack.pop(0)
            return l
        else:
            return ''

    def serial_avail(self):
        if self.g_serial:
            if len(self.g_ser_rcvstack) > 0:
                return True
        return False

    def send_serial(self, sstr):
        if self.g_serial:
            s = sstr + "\r"
            self.g_serial.write(s.encode("utf-8"))

    def on_connect(self):  # Could be disconnect or connect, depending on previous serial port state
        if self.g_serial:
            self.close_serial()
        else:
            self.open_serial()
            if self.g_serial:
                self.termbox.insert(tk.INSERT, "===========Port %s opened>>>\n" % self.g_port, "info")
                self.g_ser_parsestring = ""
                self.send_serial('')
                self.master.after(500, self.ask_version)
            else:
                messagebox.showerror('Error', 'Unable to open port ' + self.g_port)

    def set_locked(self):
        self.g_locked = True
        self.red_button('bunlock')
        self.enable_button('bunlock')
        self.enable_button('bzero')
        self.disp_message("Device locked - enter lock code")


    def set_unlocked(self):
        self.g_locked = False
        self.black_button('bunlock')
        self.disable_button('bunlock')
        self.disable_button('bzero')
        self.enable_button('bread')
        self.disp_message("Device unlocked")

    def ask_version(self):
        self.send_serial('V')
        self.master.after(500, self.get_version)

    def get_version(self):
        if self.serial_avail():
            l = self.get_serial_line()
            if len(l) == 3:
                t, ev = l[:1], l[1:]
                self.builder.tkvariables['strlver'].set(ev)
                self.g_eevver = int(ev)
                if self.g_eevver < __EEVVER__:
                    messagebox.showwarning('Warning',
                                           'Device EEprom schema is too old.  You will not be able to write to the device unless you upgrade its firmware first.')
                    self.g_badver = True

                if t == 'L':
                    self.set_locked()
                elif t == 'E':
                    self.set_unlocked()

                self.enable_button('breset')
                if self.g_eevver > 1:  # Release available
                    self.master.after(500, self.ask_release)
                else:
                    self.builder.tkvariables['sfwver'].set('N/A')
            else:
                self.master.after(200, self.get_version)

    def ask_release(self):
        self.send_serial('R')
        self.master.after(500, self.get_release)

    def get_release(self):
        if self.serial_avail():
            l = self.get_serial_line()
            if len(l) >= 3:
                self.builder.tkvariables['sfwver'].set(l[2:])
            else:
                self.master.after(200, self.get_release)

    @staticmethod
    def lc_valid(code, search=re.compile(r'^[1-3]{4}').search):
        return bool(search(code))

    def on_unlock(self):
        if self.g_locked:
            entry = self.builder.tkvariables['strlentry'].get()
            if (len(entry) < 4) or not self.lc_valid(entry):
                messagebox.showerror('Error', 'Code must be 4 digits in [1..3]')
                return

            self.send_serial(entry)
            self.master.after(400, self.ask_version)

    def on_read(self):
        self.disable_button('bvalidate')
        self.black_button('bvalidate')
        self.disable_button('bwrite')
        self.send_serial("EB")
        self.master.after(500, self.get_dump_v)

    def get_dump_v(self):
        if self.serial_avail():
            v = self.get_serial_line()
            if v.startswith('V'):
                self.g_rdb_ver = v
                # Got a dump version - get the body
                self.master.after(200, self.get_dump_b)
            else:
                self.master.after(200, self.get_dump_v)

    def get_dump_b(self):
        b = self.get_serial_line()  # s has the entire dump
        self.get_serial_line()
        self.g_rdb_body = b
        self.parse_dump()

    def parse_dump(self):
        pw_sl = []

        self.eebox.delete("1.0", tk.END)
        self.eebox.insert(tk.END, self.g_rdb_ver + '\n')
        self.eebox.insert(tk.END, self.g_rdb_body)

        self.g_eedver = int(self.g_rdb_ver[1:])

        self.builder.tkvariables['seedver'].set(str(self.g_eedver).zfill(2))
        if int(self.g_eedver) != __EEDVER__:
            messagebox.showerror('Error', 'Incompatible eeprom dump format.')
            self.g_badver = True
            return

        s = self.g_rdb_body
        if len(s) > 6:
            for i in range(8):
                splitat = 140  # pwd field is 70 bytes
                l, r = s[:splitat], s[splitat:]
                s = r
                pw_sl.append(l)
            splitat = 32  # variable field is 16 bytes
            var_s, r = s[:splitat], s[splitat:]
            s = r
            splitat = 8  # crc field is 4 bytes
            crc_s, r = s[:splitat], s[splitat:]
            s = r
            splitat = 16  # semaphore field is 8 bytes, signature is final 4 bytes
            sem_s, sig_s = s[:splitat], s[splitat:]

            self.g_semas = sem_s
            self.g_sig = sig_s

            self.builder.tkvariables['sdcrc'].set(crc_s)
            self.builder.tkvariables['ssema'].set(sem_s)
            ss = "".join(reversed([sig_s[i:i + 2] for i in range(0, len(sig_s), 2)]))
            self.builder.tkvariables['ssig'].set(ss.upper())

            self.populate_pw(pw_sl)
            self.populate_var(var_s)

            self.enable_button('bvalidate')
            self.disp_message("Read OK")


    def populate_pw(self, pwlist):
        for i in range(6):
            s = pwlist[i]
            splitat = 2  # userid len and pw len are  2 bytes each
            l, r = s[:splitat], s[splitat:]
            s = r

            uidlen = int(l, 16)
            l, r = s[:splitat], s[splitat:]
            s = r

            pwdlen = int(l, 16)
            splitat = 16  # name is 8 bytes
            l, r = s[:splitat], s[splitat:]
            s = r
            #l = l.split('00')[0] 

            slotname = binascii.unhexlify(l).decode('utf-8').rstrip('\x00')

            splitat = 60  # userid and pw are 30 bytes each
            l, r = s[:splitat], s[splitat:]
            if uidlen:
                uid = binascii.unhexlify(l).decode('utf-8')[:uidlen].rstrip('\x00')
            else:
                uid = ''
            if pwdlen:
                pwd = binascii.unhexlify(r).decode('utf-8')[:pwdlen].rstrip('\x00')
            else:
                pwd = ''

            if uid.endswith('\t'):
                setter = "self.builder.tkvariables[\'btab" + str(i) + "\'].set(True)"
                uid = uid[:-1]
            else:
                setter = "self.builder.tkvariables[\'btab" + str(i) + "\'].set(False)"
            eval(setter)

            setter = "self.builder.tkvariables[\'sename" + str(i) + "\'].set(slotname)"  # throws exception for 0
            eval(setter)
            setter = "self.builder.tkvariables[\'suid" + str(i) + "\'].set(uid)"
            eval(setter)
            setter = "self.builder.tkvariables[\'spwd" + str(i) + "\'].set(pwd)"
            eval(setter)

    def populate_var(self, pvars):
        r = pvars
        l, r = r[:2], r[2:]
        url = not bool(int(l, 16))  # display logo flag
        self.builder.tkvariables['bcblogo'].set(url)

        l, r = r[:2], r[2:]
        dflip = bool(int(l, 16))  # flip display flag
        self.builder.tkvariables['bcbflip'].set(dflip)

        l, r = r[:2], r[2:]
        ssec = int(l, 16)  # security code index
        if ssec:
            self.builder.tkvariables['strlentry'].set(self.g_lctable[ssec + 1])
        else:
            self.builder.tkvariables['strlentry'].set('')

        l, r = r[:2], r[2:]
        dto = int(l, 16)  # display timeout
        self.builder.tkvariables['sedto'].set(dto)

        l, r = r[:2], r[2:]
        lto = int(l, 16)  # led timeout
        self.builder.tkvariables['seito'].set(lto)

        l, r = r[:2], r[2:]
        bseq = int(l, 16)  # button sequence index
        self.set_combo_current('cbseq', bseq)

        l, r = r[:2], r[2:]
        lseq = int(l, 16)  # led sequence index
        self.set_combo_current('clseq', lseq)

        l, r = r[:2], r[2:]
        rpw = bool(int(l, 16))  # pw revert flag
        self.builder.tkvariables['bcbrevert'].set(rpw)

        l, r = r[:2], r[2:]
        lfail = int(l, 16)  # security code fail action index
        self.set_combo_current('ckfail', lfail)

        l, r = r[:2], r[2:]
        slto = int(l, 16)  # security lock timeout
        self.builder.tkvariables['selto'].set(slto)

    def set_invalid(self, error):
        messagebox.showerror('Error', error)
        self.red_button('bvalidate')
        self.disable_button('bwrite')
        self.disp_message("Invalid data");

    def on_validate(self):

        # Compile regexes for validation
        snm = re.compile(r"^(?:\w|-| ){0,8}$")  # Slot name
        uim = re.compile(r"^(?:\w|-| |@|\+|\.){0,29}$")  # Userid
        pwm = re.compile(r"^(?:[ -~]){0,30}$")  # Password
        dig = re.compile(r"^(?:\d){0,4}$")  # digits
        sem = re.compile(r"^(?:[A-F]|\d){16}$")

        self.g_valid = False
        self.clear_device_data()

        ucode = self.builder.tkvariables['strlentry'].get()
        if (len(ucode) > 0) and ((len(ucode) < 4) or (not int(ucode) in self.g_lctable) or (int(ucode) == 0)):
            self.set_invalid('Unlock code must be exactly 4 digits in [1..3] ')
            return
        if ucode == '':
            self.g_ucode = 0
        else:
            ucodei = int(ucode)
            self.g_ucode = self.g_lctable.index(ucodei) - 1  # !!! check zero case

        for i in range(6):
            getter = "self.builder.tkvariables[\'sename" + str(i) + "\'].get()"
            slotname = eval(getter).rstrip()
            if re.match(snm, slotname) is None:
                self.set_invalid('Username must be 0 to 8 alphanumeric characters: slot ' + str(i))
                return
            self.g_slotname.append(slotname)

            getter = "self.builder.tkvariables[\'btab" + str(i) + "\'].get()"
            tab = eval(getter)

            getter = "self.builder.tkvariables[\'suid" + str(i) + "\'].get()"
            uid = eval(getter).rstrip()
            if re.match(uim, uid) is None:
                self.set_invalid('Userid must be 0 to 29 letters, numbers, @, -, + or .: slot ' + str(i))
                return
            if tab:
                uid += '\t'
            self.g_uid.append(uid)

            getter = "self.builder.tkvariables[\'spwd" + str(i) + "\'].get()"
            pwd = eval(getter).rstrip()
            if re.match(pwm, pwd) is None:
                self.set_invalid('Password must be 0 to 30 printable ASCII characters: slot ' + str(i))
                return
            self.g_pwd.append(pwd)

        # Add 'phantom' entries (reserved space) to make the buffer complete later
        for i in range(2):
            self.g_slotname.append('')
            self.g_uid.append('')
            self.g_pwd.append('')

        dto = self.builder.tkvariables['sedto'].get()
        if (re.match(dig, dto) is None) or (int(dto) > 255):
            self.set_invalid('Display timeout must be 0 or less than 256 ')
            return
        self.g_dto = int(dto)

        ito = self.builder.tkvariables['seito'].get()
        if (re.match(dig, ito) is None) or (int(ito) > 255):
            self.set_invalid('LED timeout must be 0 or less than 256 ')
            return
        self.g_ito = int(ito)

        lto = self.builder.tkvariables['selto'].get()
        if (re.match(dig, lto) is None) or (int(lto) > 255):
            self.set_invalid('Lock timeout must be 0 or less than 256 ')
            return
        self.g_lto = int(lto)

        self.g_flip = self.builder.tkvariables['bcbflip'].get()
        self.g_revert = self.builder.tkvariables['bcbrevert'].get()
        self.g_logo = not self.builder.tkvariables['bcblogo'].get()

        cbseqobj = self.builder.get_object('cbseq', self.master)
        self.g_bseq = cbseqobj.current()

        clseqobj = self.builder.get_object('clseq', self.master)
        self.g_lseq = clseqobj.current()

        ckfailobj = self.builder.get_object('ckfail', self.master)
        self.g_kfail = ckfailobj.current()

        self.g_eedata = bytearray()

        for i in range(8):  # include phantom entries
            self.g_eedata += bytearray(len(self.g_uid[i]).to_bytes(1, byteorder='little'))
            self.g_eedata += bytearray(len(self.g_pwd[i]).to_bytes(1, byteorder='little'))
            self.g_eedata += self.g_slotname[i].ljust(8, '\0').encode('utf-8')
            self.g_eedata += self.g_uid[i].ljust(30, '\0').encode('utf-8')
            self.g_eedata += self.g_pwd[i].ljust(30, '\0').encode('utf-8')

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

        crc = binascii.crc32(self.g_eedata)
        crcb = crc.to_bytes(4, byteorder='little')
        s = binascii.hexlify(crcb).decode("utf-8").upper()
        self.builder.tkvariables['slcrc'].set(s)
        self.g_eedata += crcb

        semas = self.builder.tkvariables['ssema'].get()
        if re.match(sem, semas) is None:
            self.set_invalid('Semaphore list must be 8 2-digit uppercase hex bytes')
        self.g_semas = semas

        self.eebox.delete("1.0", tk.END)
        if not self.g_badver:
            self.enable_button('bwrite')
            self.g_wrbuf = "V%02X\n" % int(self.g_eedver)
            self.g_wrbuf += binascii.hexlify(self.g_eedata).decode("utf-8").upper() + self.g_semas + self.g_sig
            self.eebox.insert(tk.END, self.g_wrbuf)

        self.green_button('bvalidate')
        self.g_valid = True
        self.disp_message("Data valid");


    def on_write(self):
        self.on_validate()
        if self.g_valid:
            self.disable_button('bwrite')
            self.black_button('bvalidate')
            self.master.after(200, self.request_restore)
            self.disp_message("Write - restoring and resetting");


    def request_restore(self):
        self.send_serial("ER")
        self.master.after(200, self.clear_serial)
        self.master.after(400, self.do_restore)

    def do_restore(self):
        self.send_serial(self.g_wrbuf)
        self.master.after(400, self.clear_serial)
        self.master.after(600, self.on_read)
        self.master.after(800, self.on_reset)

    def on_reset(self):
        self.send_serial("MR")
        self.master.after(200, self.close_serial)

    def on_zero(self):
        self.send_serial("Z")
        self.master.after(200, self.close_serial)

    def on_clear(self):
        # TBD handle clear button
        return

    def on_close_window(self, event=None):
        self.close_serial()

        # Call destroy on toplevel to finish program
        self.mainwindow.master.destroy()

    @staticmethod
    def cleanup_str(s):
        p = re.compile(r'[\r\n]')
        s = p.sub("", s)
        p = re.compile(r'^\s+')
        s = p.sub("", s)
        p = re.compile(r'\s+$')
        s = p.sub("", s)
        return s

    @staticmethod
    def dbug(s):
        s = s.replace("\n", "[LF]")
        s = s.replace("\r", "[CR]")
        s = s.replace(" ", "[SPC]")
        print("$" + s + "\n")

    def generate_pw(self):
        mode = self.builder.tkvariables['sgmode'].get()
        pwlen = int(self.builder.tkvariables['selen'].get())

        if (pwlen < 6) or (pwlen > 28):
            messagebox.showwarning('Warning', 'Selected password length out of range - adjusting')
            if pwlen < 6:
                pwlen = 6
                self.builder.tkvariables['selen'].set(str(6))
            elif pwlen > 28:
                pwlen = 28
                self.builder.tkvariables['selen'].set(str(28))

        if mode == 'Numeric':
            return ''.join(secrets.choice(string.digits) for i in range(pwlen))
        elif mode == 'Alpha':
            return ''.join(secrets.choice(string.ascii_letters) for i in range(pwlen))
        elif mode == 'Alphanumeric':
            return ''.join(secrets.choice(string.ascii_letters + string.digits) for i in range(pwlen))
        elif mode == 'Symbols':
            return ''.join(
                secrets.choice(string.ascii_letters + string.digits + string.punctuation) for i in range(pwlen))
        return ''

    def on_mfile_item_clicked(self, itemid):
        if itemid == 'mfile_open':
            filename = filedialog.askopenfilename(initialdir="%USERPROFILE%/Documents", title="Select file",
                                                  filetype=(("text files", "*.txt"), ("all files", "*.*")))
            if filename:
                f = open(filename, "r")
                self.g_rdb_ver = f.readline().rstrip()
                self.g_rdb_body = f.readline().rstrip()
                f.close()
                self.black_button('bvalidate')
                self.disable_button('bwrite')
                self.parse_dump()

        elif itemid == 'mfile_save':
            self.on_validate()
            if self.g_valid:
                filename = filedialog.asksaveasfilename(initialdir="%USERPROFILE%/Documents", title="Select file",
                                                        filetypes=(("text files", "*.txt"), ("all files", "*.*")))
                if filename:
                    if not '.' in filename:
                        filename += '.txt'
                    f = open(filename, "w")
                    l = ["V%02X" % int(self.g_eedver) + '\n',
                         binascii.hexlify(self.g_eedata).decode("utf-8").upper() + self.g_semas + self.g_sig + '\n']
                    f.writelines(l)
                    f.close()

        elif itemid == 'mfile_quit':
            self.on_close_window()

    @staticmethod
    def on_about_clicked():
        messagebox.showinfo('About', 'PWT Configurator.  See https://ayb.ca/pwt')

    # Copy/paste/clear/generate handlers
    #  need to do in a stupid way due to pygubu limitations - refactor when changing UI framework
    def on_copy0(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename0'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid0'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd0'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab0'].get())

    def on_paste0(self):
        self.builder.tkvariables['sename0'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid0'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd0'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab0'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear0(self):
        self.builder.tkvariables['sename0'].set('')
        self.builder.tkvariables['suid0'].set('')
        self.builder.tkvariables['spwd0'].set('')
        self.builder.tkvariables['btab0'].set('')

    def on_generate0(self):
        self.builder.tkvariables['spwd0'].set(self.generate_pw())

    def on_copy1(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename1'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid1'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd1'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab1'].get())

    def on_paste1(self):
        self.builder.tkvariables['sename1'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid1'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd1'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab1'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear1(self):
        self.builder.tkvariables['sename1'].set('')
        self.builder.tkvariables['suid1'].set('')
        self.builder.tkvariables['spwd1'].set('')
        self.builder.tkvariables['btab1'].set('')

    def on_generate1(self):
        self.builder.tkvariables['spwd1'].set(self.generate_pw())

    def on_copy2(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename2'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid2'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd2'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab2'].get())

    def on_paste2(self):
        self.builder.tkvariables['sename2'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid2'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd2'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab2'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear2(self):
        self.builder.tkvariables['sename2'].set('')
        self.builder.tkvariables['suid2'].set('')
        self.builder.tkvariables['spwd2'].set('')
        self.builder.tkvariables['btab2'].set('')

    def on_generate2(self):
        self.builder.tkvariables['spwd2'].set(self.generate_pw())

    def on_copy3(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename3'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid3'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd3'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab3'].get())

    def on_paste3(self):
        self.builder.tkvariables['sename3'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid3'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd3'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab3'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear3(self):
        self.builder.tkvariables['sename3'].set('')
        self.builder.tkvariables['suid3'].set('')
        self.builder.tkvariables['spwd3'].set('')
        self.builder.tkvariables['btab3'].set('')

    def on_generate3(self):
        self.builder.tkvariables['spwd3'].set(self.generate_pw())

    def on_copy4(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename4'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid4'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd4'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab4'].get())

    def on_paste4(self):
        self.builder.tkvariables['sename4'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid4'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd4'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab4'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear4(self):
        self.builder.tkvariables['sename4'].set('')
        self.builder.tkvariables['suid4'].set('')
        self.builder.tkvariables['spwd4'].set('')
        self.builder.tkvariables['btab4'].set('')

    def on_generate4(self):
        self.builder.tkvariables['spwd4'].set(self.generate_pw())

    def on_copy5(self):
        self.builder.tkvariables['senamecb'].set(self.builder.tkvariables['sename5'].get())
        self.builder.tkvariables['suidcb'].set(self.builder.tkvariables['suid5'].get())
        self.builder.tkvariables['spwdcb'].set(self.builder.tkvariables['spwd5'].get())
        self.builder.tkvariables['btabcb'].set(self.builder.tkvariables['btab5'].get())

    def on_paste5(self):
        self.builder.tkvariables['sename5'].set(self.builder.tkvariables['senamecb'].get())
        self.builder.tkvariables['suid5'].set(self.builder.tkvariables['suidcb'].get())
        self.builder.tkvariables['spwd5'].set(self.builder.tkvariables['spwdcb'].get())
        self.builder.tkvariables['btab5'].set(self.builder.tkvariables['btabcb'].get())

    def on_clear5(self):
        self.builder.tkvariables['sename5'].set('')
        self.builder.tkvariables['suid5'].set('')
        self.builder.tkvariables['spwd5'].set('')
        self.builder.tkvariables['btab5'].set('')

    def on_generate5(self):
        self.builder.tkvariables['spwd5'].set(self.generate_pw())


###
# Scan for serial ports with devices connected
def scan_serial_ports():
    if sys.platform.startswith('win'):
        print("Windows detected")
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        print("Linux detected")
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        print("MacOs detected")
        ports = glob.glob('/dev/tty.*')
    else:
        print("No OS detected")
        raise EnvironmentError('Unsupported platform')

    print(ports)
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    print(result)
    return result


###
# Main
if __name__ == '__main__':
    # Get a list of serial ports
    u_portlist = scan_serial_ports()
    u_portlist.insert(0, '')

    # Create and start the GUI environment
    root = tk.Tk()
    ## root.iconbitmap(default='pt.ico')
    app = Application(root)
    app.run()
