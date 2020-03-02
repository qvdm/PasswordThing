import serial
import threading
import time
import queue
import tkinter as tk

class SerialThread(threading.Thread):
    def __init__(self, rxqueue, txqueue):
        threading.Thread.__init__(self)
        self.rxqueue = rxqueue
        self.txqueue = txqueue
    def run(self):
        s = serial.Serial('COM8',115200)
        s.write(str.encode('\n'))
        time.sleep(0.2)
        while True:
            if s.inWaiting():
                text = s.readline(s.inWaiting())
                self.rxqueue.put(text)
            if self.txqueue.qsize():
                text = self.txqueue.get()
                s.write(str.encode(text))

class App(tk.Tk):
    def __init__(self):
        tk.Tk.__init__(self)
        self.geometry("1360x750")
        self.lc=0
        frameLabel = tk.Frame(self, padx=40, pady =40)
        self.text = tk.Text(frameLabel, wrap='word', font='TimesNewRoman 37',
                            bg=self.cget('bg'), relief='flat')
        frameLabel.pack()
        self.text.pack()
        self.rxqueue = queue.Queue()
        self.txqueue = queue.Queue()
        thread = SerialThread(self.rxqueue, self.txqueue)
        thread.start()
        self.process_serial()
    


    def process_serial(self):
        value=True
        while self.rxqueue.qsize():
            try:
                new=self.rxqueue.get()
                if value:
                    self.text.delete(1.0, 'end')
                value=False
                self.text.insert('end',new)
            except self.rxqueue.Empty:
                pass

        if (self.lc == 0) :
            pass
        elif (self.lc == 1) :
           self.txqueue.put('3123\n')
        elif (self.lc == 2) :
            pass
        elif (self.lc == 3) :
            self.txqueue.put('0P\n')
        self.lc = self.lc+1

        self.after(100, self.process_serial)

app = App()
tk.mainloop()