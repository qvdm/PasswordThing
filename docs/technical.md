# Pasword Generation

The entropy harvester for the password generator is based on a scheme
documented at https://gist.github.com/endolith/2568571 .

When you plug the Password Thing into a USB port it starts to harvest
entropy by periodically sampling the lower buts of a fast-running timer. 
The bits are scrambled and combined to generate one bit of entropy for each
software cycle or SysTick. 

The default systick interval is 100ms, but when the amount of available
entropy is low, such as right after startup, the entropy harvester is run
every 1ms during idle time.  

Up to 256 bytes of entropy are stored in a queue to be used by the password
generator.  

The stored entropy is used to create password characters from the available
character set, which may be modified by the user through the configuration
interface.  

Some entropy is wasted on thrown-away bits to avoid modulo bias  so that the
randomness of the generated passwords are not dependent on the selected
character set.  

# Serial configuration interface

The GUI configuration interface uses an underlying serial protocol to communuicate with the PT.  
This interface is not intended for human use, but can be used for troubleshooting in a pinch. 

# Software

To access the serial configruation interface, you need a Serial Terminal Emulator, since the PT emulates a
serial port.  CoolTerm and MobaXterm / PuTTY are options that work fine, but you can use any
terminal emulator that supports USB serial ports.  

# Settings

The connection settings are: **115200 N 8 1** (115,200 baud, no parity, 8
data bits, 1 stop bit).

The terminal emulator should also be configured to assert DTR and RTS on
startup.  

# Entering Configuration mode

To enter serial configuration mode, press any button while plugging the PT into a USB
port.  The LED will be solid white until the terminal emulator connects
(asserts DTR/RTS) and then the LED will blink off briefly once a second to
indicate configuration mode. 

Alternatively, you can enter configuration mode at any time by pressing the Next
button on the PT for longer than 3 seconds.  The LED will change to white to 
indicate that the PT is in configuration mode.  

When you enter Configuration mode and connect a serial terminal, you should
see the current software version, folllowed by a prompt: **>** on the terminal, unless
a lock code was set, in which case the prompt will be **#**

# Unlocking
If a lock code was set, you will need an unlock code to enter Serial command mode.
Enter the lock code (exactly 4 digits between 1 and 3) followed by **<Return>**
If you cannot remember the lock code, enter **Z**.  
**This will clear the contents of the EEPROM, deleting all passwords, userids and settings, and disable the lock code.**

# Commands
Serial configuration commands have the format: 

> <slot|group><cmd>[arg]

Where **slot** is a password slot between 0 and 5, inclusive, **group** indicates
the command group, one of:  **G**enerator | **S**ettings | **T**imeout | se**Q**uence | **E**eprom | **M**aintenance

**cmd** selects a command within the group, as shown below, and **arg** is a numerical argument.  

## Slot commands

Slot commands have the format: 

> <0..5><cmd>[arg]

* #O<passwd> : The **S** command sets the password for the slot selected by **#** to the string specified by **passwd**
  - Example: **1Shunter2** - sets the password for slot 1 to "hunter2"
* #I<userid> : The **I** command sets the userid for the slot selected by **#** to the string specified by **userid**
  - Example: **2Ijoe** - sets the userid for slot 2 to "joe"
* #N<name> : The **N** command sets the name of the slot selected by **#** to the string specified by **name**
  - Example: **3NWindows** - sets the name of slot 3 to "Windows"
* #P : The **P** command prints the name, userid and password for the slot selected by **#*
  - Example: **1P** - prints the information stored in slot 1
* #G : The **G** command generates a new password for the slot selected by **#*, based on the rules specified by group **G** commands. 
  - Example: **1G** - generates a new password for slot 1
* #C : The **C** command clears the contents of the  slot selected by **#**
  - Example: **1C** - Clears all the information in slot 1
* #D<dest> : The **D** command duplicates the contents of the  slot selected by **#** into the slot specified by **dest**
  - Example: **1D2** - Copies all information from slot 1 to slot 2
  
## Generator commands

Generator commands have the format: 

> G<cmd>[arg]

* GM<A|N||L|S> : The **M** command sets the password generator mode to the mode specified in the argument - **A**lphabetic, **N**umeric, a**L**phanumeric or **S**pecial
  - Example: **GMS** - sets the password generation mode to "Special", i.e. to generate passwords containing special characters. 
* GL<length> : The **L** command sets the password generator length to the length specified in the argument 
  - Example: **GL12** - configures the password generator  to generate 12 character passwords
* GEL : The **E** command displays the current entropy level of the password generator
  - Example: **GEL** - displays the amount of entropy available for password generation.  

## Settings commands

Commands in the Settings group have the format: 

> S<cmd>[arg]

* SF[T] : The **F** command shows or toggles the Display Flip setting
  - Examples: **SFT** - toggle display flip setting, **SF** - show current display flip setting
* SR[T] : The **R** command shows or toggles the Password Revert setting
  - Examples: **SRT** - toggle password revert setting, **SR** - show current password revert setting

## Timeout commands

Commands in the Timeout group have the format: 

> T<cmd>[timeout]

* TD[timeout] : The **D** command shows or sets the display timeout
  - Examples: **TD30** - set display timeout to 30s, **TD** - show current display timeout setting
* TI[timeout] : The **I** command shows or sets the LED timeout
  - Examples: **TI30** - set LED timeout to 30s, **TI** - show current LED timeout setting
* TL[timeout] : The **L** command shows or sets the Lock timeout
  - Examples: **TL30** - set autolock timeout to 30 minutes, **TL** - show current lock timeout setting

## Sequence commands

Commands in the Sequence group have the format: 

> Q<cmd>[sequence]
 
* QB[sequence] : The **B** command shows or sets the button sequence
  - Examples: **QBSNG** - sets the button sequence to "Sel-Next-Gen", **QB** - shows the current button sequence
* QL[sequence] : The **L** command shows or sets the LED sequence
  - Examples: **QLRGBCMY** - sets the LED sequence to "R-G-B-C-M-Y", **QL** - shows the current LED sequence
  
## Eeprom commands

Commands in the Eeprom group have the format: 

> E<cmd>

* ED : The **D** command dumps the contents of the Eeprom in human readable form
* EB : The **B** command dumps the contents of the Eeprom in a format suitable to save as a backup
* ER<backup> : The **R** command, followed by dump created with the 'B' command restores the contents of the Eeprom to the backup
* EZ : The **Z** command clears the contents of the Eeprom
  
## Maintenance commands

Commands in the Maintenance group have the format: 

> M<cmd>
 
* MR : The **R** command resets the PT.  It will disconnect from the terminal session and restart in normal mode
* ML : The **L** command toggles the display of "ayb.ca/pwt" during startup
* MS[sequence] : The **S** command shows or sets sthe security code/sequence
  - Examples: **MS1231** - sets the security sequence to "1-2-3-1", **MS** - shows the current security sequence
  

# Configuration interface issues
The configuration user interface is intended to be a machine-to-machine interface and
is not fault tolerant at all.  Bad input can cause the PT to crash or go into an unstable state.   

In that case, just unplug and restart the configuration session.  If password
generation or entry stops working for a slot, use the Generate button to
generate a password for the slot in normal mode, and try again. 



