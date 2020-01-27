# menu.py
import sys
import glob
import tkinter as tk
from tkinter import messagebox
import pygubu
import serial


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


class MyApplication(pygubu.TkApplication):

    def _create_ui(self):
        #1: Create a builder
        self.builder = builder = pygubu.Builder()

        #2: Load an ui file
        builder.add_from_file('menu.ui')

        #3: Create the widget using self.master as parent
        self.mainwindow = builder.get_object('mainwindow', self.master)

        # Set main menu
        self.mainmenu = menu = builder.get_object('mainmenu', self.master)
        self.set_menu(menu)

        # Configure callbacks
        builder.connect_callbacks(self)


    def on_mfile_item_clicked(self, itemid):
        if itemid == 'mfile_open':
            messagebox.showinfo('File', 'You clicked Open menuitem')

        if itemid == 'mfile_quit':
            messagebox.showinfo('File', 'You clicked Quit menuitem. Byby')
            self.quit();

    def on_mtools_item_clicked(self, itemid):
        if itemid == 'mtools_dump':
            messagebox.showinfo('File', 'You clicked Dump menuitem')
        elif itemid == 'mtools_clear':
            messagebox.showinfo('File', 'You clicked Clear menuitem')

    def on_about_clicked(self):
        messagebox.showinfo('About', 'You clicked About menuitem')


if __name__ == '__main__':
    ports = serial_ports()
    root = tk.Tk()
    app = MyApplication(root)
    app.run()
    
    