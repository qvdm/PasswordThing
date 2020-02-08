# menu.py
import sys
import glob
try:
    import tkinter as tk
    from tkinter import messagebox
except:
    import Tkinter as tk
    import tkMessageBox as messagebox
import pygubu
import serial


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
        self.serial=None

        self.builder = builder = pygubu.Builder()

        builder.add_from_file('menu.ui')

        self.mainwindow = builder.get_object('mainwindow', self.master)
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        self.mainlabel = mainlabel = builder.get_object('lmain', self.master)
        self.builder.tkvariables['strlmain'].set("PwThing Version " + __VERSION__)

        self.portcombo = portcombo = builder.get_object('cport', self.master)
        portcombo['values'] = portlist
        portcombo.current(0)
    
        self.master.protocol("WM_DELETE_WINDOW", self.on_close_window)

        self.termbox = termbox = builder.get_object('tterm', self.master)

        builder.connect_callbacks(self)


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

    def on_connect(self):
        if self.port != None:
            if self.serial == None:
                self.serial = serial.Serial(port=self.port, baudrate=self.baud)
            else:
                self.serial.close()
                self.serial = serial.Serial(port=self.port, baudrate=self.baud)
            self.termbox.insert(tk.INSERT, "===========Port %s opened>>>\n"%self.port,"info")
            self.Serial_term()
        else:
            messagebox.showerror('Error', 'No port selected')
           

    def on_close_window(self, event=None):
        if self.dirty:
            msg = messagebox.askquestion ('Exit Configurator','There are unsaved changes.  Are you sure you want to exit',icon = 'warning')
            if msg != 'yes':
                return

        if self.serial != None:
            self.serial.close()
        
        # Call destroy on toplevel to finish program
        self.mainwindow.master.destroy()

    def Serial_term(self):
        if self.serial != None:
            rx = []
            while (self.serial.inWaiting()>0):
                rx.append(ord(self.serial.read(1)))
                time.sleep(0.001)
            if rx != []:
                for s in rx:
                    self.termbox.insert(tk.INSERT, "%c"%s)
                if(rx[-1] != 0x0d): self.termbox.insert(tk.INSERT, "\n")
                if(self.autoscroll): self.termbox.see("end")

if __name__ == '__main__':
    portlist = serial_ports()
    portlist.insert(0,'')
    root = tk.Tk()
    app = Application(root)
    app.run()
    
    