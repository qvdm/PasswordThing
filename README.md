# PasswordThing
Atmega 32U4 based Password generator and typer

Organized for building with ![Platformio](https://platformio.org/)

	To compile with the Arduino IDE, copy contents of the  **src** directory to
	another directory and rename main.cpp to the name of that directory, with a 
	.ino extension.  Follow Arduino instructions to install the SSD1306Ascii
	library in your Arduino environment.  **Note:** I have not tested this under
	the Arduino environment. 

Get started by cloning the repository, including the submodule for managing
the OLED:

	git clone git@github.com:qvdm/PasswordThing.git
	cd PasswordThing
	git submodule init
	git submodule update


The configurator GUI requires [pygubu](https://github.com/alejandroautalan/pygubu)
and Python >= 3.6 with tkinter, tk_tools and pySerial installed.  To build
in Platformio, ensure that the correct Python runtime is selected, load
ptgui.py in the editor and press the Run button.  It will fail the first
time, after which you can type 'cd gui' in the newly opened Terminal and run
the program again.

See the docs directory for a manual, which is published at 
https://qvdm.github.io/PasswordThing/
and can be generated with **mkdocs gh-deploy**

Circuit assembly and software compiliation  instructions can be found  in 
the **Hardware amd Software**  section of the manual. 

