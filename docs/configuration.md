# Introduction

The Password Thing (PT) provides a **"configuration"** interface to allow you to use
extended functionality that would be difficult to implement using just the
three (or two) buttons on the device.  

The actual code running on the PT is documented in the **Hardware and
Software** section of this manual. 

# Software

To access Configuration mode, you need a Serial Terminal Emulator, since the PT emulates a
serial port.  CoolTerm is one option that works fine, but you can use any
terminal emulator that supports USB serial ports.  

# Settings

The connection settings are: **115200 N 8 1** (115,200 baud, no parity, 8
data bits, 1 stop bit).

The terminal emulator should also be configured to assert DTR and RTS on
startup.  

# Entering Configurationmode

To enter configuration mode, press any button while plugging the PT into a USB
port.  The LED will be solid white until the terminal emulator connects
(asserts DTR/RTS) and then the LED will blink off briefly once a second to
indicate configuration mode. 

# Commands

## Help
You can press **'?'** or **'h'** at any Slot prompt to get a list of available commands.  

## Slot number
The serial terminal should now show a prompt with the current Slot number
(0).  You can press **'S'** followed by a slot number in the range 0-5 to change
the current slot.  *Shortcut: just press the numeric key corresponding to
the desired slot number*

Other commands to change the slot are **'N'** for Next slot and **'R'** for
pRevious slot.  

## Print password
Pressing **'P'** prints the password (and userid, if there is one) stored in
the current slot. 

## Enter Userid
Press **'U'** to enter a new Userid for the slot. The maximum lenght of a
userid is 30 characters. 

## Enter Password
press **'O'** to enter a new password for the current slot.  The password
may contain numbers, letters, spaces  and special characters.  Maximum length 
is 30 characters. 

## Set password generator Mode
Press **'M'** to set the mode for subsequent generated passwords.  The
options are Alphabetic, Numeric, Alphanumeric and Special (allow special
characters)

The mode only applies to passwords generated via the configuration interface,
and a mode change only lasts for the current serial terminal sesssion.  

## Set password generator Length
Press **'L'** to set the length of subsequent generated passwords.  

The length only applies to passwords generated via the configuration interface,
and a length change only lasts for the current serial terminal sesssion.  

## Generate password
Press **'G'** to generate a new password for the currently selected slot. 
The Mode and Length selected above will determine the nature of the
generated password.  

## Clear slot
Press **'C'** to remove the userid and password stored in the current slot

## Duplicate  slot
Press **'A'** to copy the userid and password stored in the current slot to
another slot.  This may be useful when changing your password on a site. 
 
## Toggle running indicator
By default the PT has a red flashing led to indicate that the software is
running. If this indicator is too distracting, you can press **'B'** to
disable it.  

The status of the indicator is stored in nonvolatile memory and survives 
unplugging or restarting of the PT.  

## Privacy timeout
The PT model U displays passwords on the OLED display when they are
generated to entered.  For security / privacy reasons, the display blanks
after 10 seconds by default.  

Press **'I'** to adjust the pre-blank timeout.  Enter a value of 0 to
disable blanking completely. 

The timeout is stored in nonvolatile memory and survives unplugging or 
restarting of the PT.  

## Colour LED timeout
Some users find the bright colour LED on the PT distracting. There is 
an option to turn off the LED after a period of time.  

Press **'J'** to adjust the timeout.  Enter a value of 0 to disable.

The timeout is stored in nonvolatile memory and survives unplugging or 
restarting of the PT.  

## Flip display
The display of the PT model U may appear upside down depending on the
orientation in which you use it. 

Press **'F'** to flip the display around.  

The flip value is stored in nonvolatile memory and survives unplugging or 
restarting of the PT.  

## Reconfigure buttons
Depending on the orientation in which you use your PT model U or the side of
the computer where you plug in your model A, the default button layout may
be awkward.  You can press **'T'** to move the key assignments around.  

The button assignment is stored in nonvolatile memory and survives unplugging or 
restarting of the PT.  

## Reconfigure LED colours
The sequence of colours indicating the various slots may be adjusted.  

Press **'K'** to change to colour assignment.  

The colour assignment is stored in nonvolatile memory and survives unplugging or 
restarting of the PT.  

## Set security sequence
To provide protection against casual unathorized use of the PT, it may be
configured to require the user to press a button sequence before it will
start operating.  

Press **'W'** to set the security sequence.  

A security sequence is entered as a set of exactly four digits, with **'1'**, **'2'** 
and **'3'** being the only digits allowed.   **'1'** refers to the **Select** button, 
**'2'** to the **Next** button and **'3'** to the **Generate** button.  (Note that 
these are the original button definitions.  If you remap the button function through 
the **'T'** function described above, it will **not** affect the security button 
sequence at all.  

**Example:**  A sequence of **1123** means pressing the **Select** button twice, then 
the **Next** button and then the **Generate** button.



## Show EEPROM Variables, Dump EEPROM,  Zero EEPROM and Show Entropy
These are diagnostic and repair functions.  Do not use unless directed  to
do so by a customer service representative. 
 