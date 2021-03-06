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
comport=''
ledcolor='red'

# serial data callback 
def OnReceiveSerialData(message):
    str_message = message.decode("utf-8")
    textbox.insert('1.0', str_message)

root = tk.Tk() # create a Tk root window
root.title( "TERMINAL - Serial Data Terminal v1.01" )
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

#COM Port label
label_comport = Label(root,width=12,height=2,text="Serial Port:")
label_comport.place(x=10,y=26)
label_comport.config(font="bold")

tkvar = StringVar(root)
tkvar.set(' ')
comport_combobox =  tk.OptionMenu(root, tkvar, *portlist, command=set_port)
comport_combobox.place(x=100,y=26)

canvas1 = Canvas(root, height=25, width=25)
led = canvas1.create_oval(5,5,20,20, fill=ledcolor)
canvas1.pack()


# Register the callback above with the serial port object
serialPort.RegisterReceiveCallback(OnReceiveSerialData)

def sdterm_main():
    if serialPort.IsOpen() :
        ledcolor = 'green'
    else:
        ledcolor = 'red'
    canvas1.itemconfig(led, fill=ledcolor)
    root.after(200, sdterm_main)  # run the main loop once each 200 ms

#
#  commands associated with button presses
#
def OpenCommand():
    if button_openclose.cget("text") == 'Open Port':
        baudrate = baudrate_edit.get()
        serialPort.Open(comport,baudrate)
        button_openclose.config(text='ClosePort')
        textbox.insert('1.0', "Port Opened\r\n")
    elif button_openclose.cget("text") == 'Close Port':
        serialPort.Close()
        button_openclose.config(text='Open Port')
        textbox.insert('1.0',"Port "+comport+" Closed\r\n")


def ClearDataCommand():
    textbox.delete('1.0',END)

def SendDataCommand():
    message = senddata_edit.get()
    if serialPort.IsOpen():
        message += '\r\n'
        serialPort.Send(message)
        textbox.insert('1.0',message)
    else:
        textbox.insert('1.0', "Not sent - COM port is closed\r\n")

def RescanCommand():
    portlist = serialPort.PortList()
    portlist.insert(0, " ")

    tkvar.set('')
    comport_combobox['menu'].delete(0, 'end')

    for port in portlist:
        comport_combobox['menu'].add_command(label=port, command=tk._setit(tkvar, port))


def DisplayAbout():
    tk.messagebox.showinfo(
    "About",
    "Blah\r\n\r\n" )

# COM Port open/close button
button_openclose = Button(root,text="Open Port",width=20,command=OpenCommand)
button_openclose.config(font="bold")
button_openclose.place(x=210,y=30)

# COM Port rescan button
button_rescan = Button(root,text="Rescan Ports",width=20,command=RescanCommand)
button_rescan.config(font="bold")
button_rescan.place(x=410,y=30)

#Clear Rx Data button
button_cleardata = Button(root,text="Clear Rx Data",width=20,command=ClearDataCommand)
button_cleardata.config(font="bold")
button_cleardata.place(x=210,y=72)

#Send Message button
button_senddata = Button(root,text="Send Message",width=20,command=SendDataCommand)
button_senddata.config(font="bold")
button_senddata.place(x=420,y=72)

#About button
button_about = Button(root,text="About",width=16,command=DisplayAbout)
button_about.config(font="bold")
button_about.place(x=620,y=30)

#
# data entry labels and entry boxes
#

#Send Data entry box
senddata_edit = Entry(root,width=34)
senddata_edit.place(x=620,y=78)
senddata_edit.config(font="bold")
senddata_edit.insert(END,"Message")

#Baud Rate label
label_baud = Label(root,width=10,height=2,text="Baud Rate:")
label_baud.place(x=10,y=70)
label_baud.config(font="bold")

#Baud Rate entry box
baudrate_edit = Entry(root,width=10)
baudrate_edit.place(x=100,y=80)
baudrate_edit.config(font="bold")
baudrate_edit.insert(END,"115200")

#
# The main loop
#
root.after(200, sdterm_main)
root.mainloop()
#


