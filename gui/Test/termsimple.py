#
import tkinter as tk
import tkinter.scrolledtext as tkscrolledtext
from tkinter import *
import serial_rx_tx
import _thread
import time
from tkinter import messagebox

# globals
serialPort = serial_rx_tx.SerialPort()
portlist = serialPort.PortList()

print("Portlist=", ", ".join(portlist))
portlist.reverse()

for port in portlist:
    time.sleep(0.5)
    print(port, "\n")
    try:
        serialPort.Open(port, 115200)
        serialPort.Send("V")
        print("RCV: ", serialPort.Receiveline(), "\n")
        serialPort.Close()
    except:
        print("Error opening port ", port, "\n")

# serial data callback 
def OnReceiveSerialData(message):
    str_message = message.decode("utf-8")
    textbox.insert('1.0', str_message)

root = tk.Tk() # create a Tk root window
root.title( "Termsimple" )
# set up the window size and position
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()
window_width = screen_width/2
window_height = screen_width/3
window_position_x = screen_width/2 - window_width/2
window_position_y = screen_height/2 - window_height/2
root.geometry('%dx%d+%d+%d' % (window_width, window_height, window_position_x, window_position_y))

# scrolled text box used to display the serial data
frame = tk.Frame(root, bg='cyan')
frame.pack(side="bottom", fill='both', expand='no')
textbox = tkscrolledtext.ScrolledText(master=frame, wrap='word', width=180, height=28) #width=characters, height=lines
textbox.pack(side='bottom', fill='y', expand=True, padx=0, pady=0)
textbox.config(font="bold")

# Register the callback above with the serial port object
serialPort.RegisterReceiveCallback(OnReceiveSerialData)

def sdterm_main():
    root.after(200, sdterm_main)  # run the main loop once each 200 ms

#
# The main loop
#
root.after(200, sdterm_main)
root.mainloop()
#


