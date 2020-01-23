# Introduction

The Password Thing (PT) provides a **"configuration"** interface to allow you to use
extended functionality that would be difficult to implement using just the
three (or two) buttons on the device.  Specifically it allows you to adjust 
user interface settings and to enter your own usernames and passwords

# Software

To access Configuration mode, you need a the companion configuration software.  
Binaries for Windows, MacOs and Linux are available.  


# Settings

## Help
Use the Help function or hover your cursor over a field for help

## Set Slot Name, Userid and Pasword
The GUI provides an area for setting the Name, UserId and Password for each slot.  
Length and format limits are enforced by the UI.  Fields left blank are not set 
to any value.  Select Update to store the entered values in the PT's Eeprom memory. 

## Password generator Mode and Length
The options for the password Generator mode are applicable only to the current 
configuration session and are :Alphabetic, Numeric, Alphanumeric and Special (allow special
characters) for the Mode. 

## Generate password
Select **'G'** at the end of the Slot area to generate a new password for a slot, 
using the settings defined in the previous secton. 

## Clear slot
Select **'C'** to clear the contents of a slot

 
## Display timeout
Adjust the time before the display blanks.  Enter a value of 0 to
disable blanking.

## Colour LED timeout
Adjust the time before the LED blanks

## Password Revert
If you have one password that you use all the time (like for your password
manager), it is often annoying to have to cycle back through the whole 
slot sequence after selecting another password.  If the Password revert
flag is set, the PT will return to Slot 0 when the display turns off due
to a display timeout. (The display timeout must be set to a nonzero value
for the Revert feature to work).  

## Flip display
The display of the PT model U may appear upside down depending on the
orientation in which you use it. 

Select Flip to flip the display around.  

## Reconfigure buttons
Depending on the orientation in which you use your PT model U or the side of
the computer where you plug in your model A, the default button layout may
be awkward to use.  You can select an alternative key assignment. 

## Reconfigure LED colours
The sequence of colours indicating the various slots may be adjusted.  


## Set security sequence
To provide protection against casual unathorized use of the PT, it may be
configured to require the user to press a button sequence before it will
start operating.  

A security sequence is entered as a set of exactly four digits, with **'1'**, **'2'** 
and **'3'** being the only digits allowed.   **'1'** refers to the **Select** button, 
**'2'** to the **Next** button and **'3'** to the **Generate** button.  (Note that 
these are the original button definitions.  If you remap the button function through 
the **'T'** function described above, it will **not** affect the security button 
sequence. 

**Example:**  A sequence of **1123** means pressing the **Select** button twice, then 
the **Next** button and then the **Generate** button.

## Exit / Reboot 

Select **Exit** to end the confguration session and restart the PWT in normal mode


## Show EEPROM Variables, Dump EEPROM,  Zero EEPROM and Show Entropy
These are diagnostic and repair functions.  Do not use unless directed  to
do so by a customer service representative. 
 