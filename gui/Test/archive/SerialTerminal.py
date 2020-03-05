#!/usr/bin/env python
# -*- coding: utf-8 -*-
try:
	from tkinter import *
except:
	from Tkinter import *
from tkFileDialog import asksaveasfilename,askopenfilename
from tkMessageBox import *
from serial import Serial
from serial.tools.list_ports import comports
import time, sys, json
from os.path import expanduser,isfile

def _(text):
	return text

class Application(Tk):
	def onCopy(self):
		self.text.event_generate("<<Copy>>")
	def onPaste(self):
		self.text.event_generate("<<Paste>>")
	def onCut(self):
		self.text.event_generate("<<Cut>>")
	def onClear(self):
		self.text.delete('1.0', END)
	def onClick(self, event=None):
		showinfo("Ohh yaaaa!!!","This program written by NeiroN!\nNow: "+time.strftime('%H:%M:%S'))
	def onOpen(self):
		filename = askopenfilename(filetypes = ((_("Text files"), "*.csv;*.txt"),(_("All files"), "*.*") ))
		if filename == "":
			return
		self.text.delete('1.0', END)
		fd = open(filename,"r")
		for line in fd:
			self.text.insert(INSERT, line)
		fd.close()
	def onSave(self):
		filename = asksaveasfilename(filetypes = ((_("Text files"), "*.csv;*.txt"),(_("All files"), "*.*") ))
		if filename == "":
			return
		fd = open(filename,"w")
		fd.write(self.text.get('1.0', END))
		fd.close()
	def openPort(self):
		if self.serial == None:
			self.serial = Serial(port=self.port.get(), baudrate=self.baud.get())
			#self.serial.open()
		else:
			self.serial.close()
			self.serial = Serial(port=self.port.get(), baudrate=self.baud.get())
			#self.serial.open()
		self.text.insert(INSERT, "===========Port %s opened>>>\n"%self.port.get(),"info")
		self.Reciver()
	def closePort(self):
		self.serial.close()
		self.port.set("")
		self.serial = None
		self.text.insert(INSERT, "===========Port %s closed>>>\n"%self.port.get(),"info")
	def setBaud(self):
		if self.serial != None:
			self.serial.close()
			self.serial = Serial(port=self.port.get(), baudrate=self.baud.get())
			self.serial.open()
	def OnEnterText(self, event):
		text = u''+self.text.get("end-1c linestart","end-1c")
		self.text.delete("end-1c linestart","end")
		self.Send(text)
	def OnEnterInput(self, event):
		text=u''+self.input.get()
		self.Send(text)
		self.input.delete(0, END)
	def LocalEcho(self, text):
		if self.echo.get():
			if self.dir.get(): self.text.insert(INSERT, "<<< ", "out")
			self.text.insert(INSERT, "%s\n"%text)
			if(self.autoscroll): self.text.see("end")
	def Send(self, text):
		data = ""
		self.LocalEcho(text)
		if self.sendhex.get():
			for c in text.split(" "):
				data += "%c"%int(c,16)
		else:
			data = text
		data += self.endline.get().replace(" ","")
		if self.serial != None:
			self.serial.write(bytes(data))
	def Reciver(self):
		if self.serial != None:
			rx = []
			while (self.serial.inWaiting()>0):
				rx.append(ord(self.serial.read(1)))
				time.sleep(0.001)
			if rx != []:
				if self.dir.get(): self.text.insert(INSERT, ">>> ", "in")
				for s in rx:
					if self.hexmode.get():
						self.text.insert(INSERT, "%02X "%s)
					else:
						self.text.insert(INSERT, "%c"%s)
				if(rx[-1] != 0x0d): self.text.insert(INSERT, "\n")
				if(self.autoscroll): self.text.see("end")
		self.after(1, self.Reciver)
	def onExit(self):
		self.saveConfig()
		#if askyesno(_("Exit"), _("Do you want to quit the application?")):
		self.destroy()
	def setDefaults(self):
		config = {}
		config['endline'] = " "
		config['port'] = ""
		config['baud'] = 9600
		config['echo'] = True
		config['hexmode'] = True
		config['dir'] = True
		config['autoscroll'] = True
		config['sendhex'] = False
		return config
	def saveConfig(self):
		config = {}
		config['endline'] = self.endline.get()
		config['port'] = self.port.get()
		config['baud'] = self.baud.get()
		config['echo'] = self.echo.get()
		config['hexmode'] = self.hexmode.get()
		config['dir'] = self.dir.get()
		config['autoscroll'] = self.autoscroll.get()
		config['sendhex'] = self.sendhex.get()
		cfg = open(expanduser("~/.serialterminal.json"),"w")
		cfg.write(json.dumps(config, indent = 4))
		cfg.close()
	def createWidgets(self):
		config = self.setDefaults()
		if isfile(expanduser("~/.serialterminal.json")):
			try:
				config = json.loads(open(expanduser("~/.serialterminal.json")).read())
			except:
				pass
		self.serial = None
		self.endline = StringVar()
		self.endline.set(config['endline'])
		self.inline = StringVar()
		self.port = StringVar()
		self.port.set(config['port'])
		self.baud = IntVar()
		self.baud.set(config['baud'])
		self.echo = IntVar()
		self.echo.set(config['echo'])
		self.hexmode = IntVar()
		self.hexmode.set(config['hexmode'])
		self.dir = IntVar()
		self.dir.set(config['dir'])
		self.autoscroll = IntVar()
		self.autoscroll.set(config['autoscroll'])
		self.sendhex = IntVar()
		self.sendhex.set(config['sendhex'])
		self.menubar = Menu(self)
		filemenu = Menu(self.menubar, tearoff=0)
		filemenu.add_command(label=_("Open"), command=self.onOpen)
		filemenu.add_command(label=_("Save"), command=self.onSave)
		filemenu.add_separator()
		filemenu.add_command(label=_("Exit"), command=self.quit)
		self.menubar.add_cascade(label=_("File"), menu=filemenu)
		editmenu = Menu(self.menubar, tearoff=0)
		editmenu.add_command(label=_("Cut"), command=self.onCut)
		editmenu.add_command(label=_("Copy"), command=self.onCopy)
		editmenu.add_command(label=_("Paste"), command=self.onPaste)
		editmenu.add_separator()
		editmenu.add_command(label=_("Clear"), command=self.onClear)
		self.menubar.add_cascade(label=_("Edit"), menu=editmenu)
		portmenu = Menu(self.menubar, tearoff=0)
		for n, (port, desc, hwid) in enumerate(sorted(comports()), 1):
			#sys.stderr.write('--- {:2}: {:20} {!r}\n'.format(n, port, desc))
			portmenu.add_radiobutton(label=port , value=port, variable=self.port, command=self.openPort)
		portmenu.add_separator()
		portmenu.add_checkbutton(label=_("Output in HEX"), onvalue=True, offvalue=False, variable=self.hexmode)
		portmenu.add_checkbutton(label=_("Input in HEX"), onvalue=True, offvalue=False, variable=self.sendhex)
		portmenu.add_checkbutton(label=_("Local echo"), onvalue=True, offvalue=False, variable=self.echo)
		portmenu.add_checkbutton(label=_("Direction tag"), onvalue=True, offvalue=False, variable=self.dir)
		portmenu.add_checkbutton(label=_("Autoscroll"), onvalue=True, offvalue=False, variable=self.autoscroll)
		baudmenu=Menu(portmenu, tearoff=0)
		for n in [1200,2400,4800,9600,14400,38400,57600,115200]:
			baudmenu.add_radiobutton(label=str(n), value=n, variable=self.baud, command=self.setBaud)
		portmenu.add_cascade(label=_("Baudrate"), menu=baudmenu)
		endmenu=Menu(portmenu, tearoff=0)
		endmenu.add_radiobutton(label=_("None"), value=" ", variable=self.endline)
		endmenu.add_radiobutton(label="0x0A - LF", value="\n", variable=self.endline)
		endmenu.add_radiobutton(label="0x0D - CR", value="\r", variable=self.endline)
		endmenu.add_radiobutton(label="CR+LF", value="\r\n", variable=self.endline)
		endmenu.add_radiobutton(label="TAB", value="\t", variable=self.endline)
		portmenu.add_cascade(label=_("End of line"), menu=endmenu)
		portmenu.add_separator()
		portmenu.add_command(label=_("Close Port"), command=self.closePort)
		self.menubar.add_cascade(label=_("Port"), menu=portmenu)
		helpmenu = Menu(self.menubar, tearoff=0)
		helpmenu.add_command(label=_("About"), command=self.onClick)
		self.menubar.add_cascade(label=_("Help"), menu=helpmenu)
		self.config(menu=self.menubar)
		
		self.text = Text(wrap=NONE)
		self.vscrollbar = Scrollbar(orient='vert', command=self.text.yview)
		self.text['yscrollcommand'] = self.vscrollbar.set
		self.hscrollbar = Scrollbar(orient='hor', command=self.text.xview)
		self.text['xscrollcommand'] = self.hscrollbar.set
		self.text.tag_config("a", foreground="blue", underline=1)
		self.text.tag_config("info", foreground="gray")
		self.text.tag_config("in", foreground="green", background="gray")
		self.text.tag_config("out", foreground="red", background="gray")
		#text.tag_bind("Enter>", show_hand_cursor)
		#text.tag_bind("Leave>", show_arrow_cursor)
		self.text.tag_bind("a","<Button-1>", self.onClick)
		self.text.config(cursor="arrow")
		#self.text.insert(INSERT, "click me!", "a")
		self.text.bind("<Return>", self.OnEnterText)
		self.input = Entry(self, textvariable=self.inline)
		# размещаем виджеты
		self.text.grid(row=0, column=0, sticky='nsew')
		self.vscrollbar.grid(row=0, column=1, sticky='ns')
		self.hscrollbar.grid(row=1, column=0, sticky='ew')
		self.input.grid(row=2, column=0, columnspan=2, pady=3, padx=3, sticky='ew')
		self.input.bind("<Return>", self.OnEnterInput)
		# конфигурируем упаковщик, чтобы текстовый виджет расширялся
		self.rowconfigure(0, weight=1)
		self.columnconfigure(0, weight=1)
		self.protocol("WM_DELETE_WINDOW", self.onExit)
	def __init__(self):
		Tk.__init__(self)
		self.title(_('Serial Terminal'))
		#self.geometry('300x400+200+200') # ширина=500, высота=400, x=300, y=200
		self.protocol('WM_DELETE_WINDOW', self.quit) # обработчик закрытия окна
		self.resizable(True, True) # размер окна может быть изменен только по горизонтали
		self.createWidgets()

root=Application()
root.mainloop()
sys.exit()