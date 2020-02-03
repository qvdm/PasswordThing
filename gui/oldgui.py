import tkinter as tk 
from tkinter import messagebox
import pygubu


class Application:
    def __init__(self, master):

        #1: Create a builder
        self.builder = builder = pygubu.Builder()

        #2: Load an ui file
        builder.add_from_file('menu.ui')

        #3: Create the widget using a master as parent
        self.mainwindow = builder.get_object('mainwindow', self.master)

        self.mainmenu = menu = builder.get_object('mainmenu', self.master)


if __name__ == '__main__':
    root = tk.Tk()
    app = Application(root)
    root.mainloop()