# Circuit

The circuit for the PT model U is shown below.  The microprocessor board 
is a generic 32U4 "Leonardo" compatible board with a micro-usb connector. 
The board is available from Aliexpress and other vendors.  

![PasswordThing Model U](img/PWT-U.png)

*PT-U circuit diagram*


The model A is the same, excpet that it uses a smaller board (sometimes
called 'beetle' by online vendors.  The connector is USB-A of course. 

Note  that you would typically not wire in an OLED display for a model A
(although it is supported).  The buttons are connected to D9, D8 and D7 
as opposed to D9, D10, D11 in the  model U.  Finally, the model A board has a 
build-in LED on D6, so no need to connect an external one.  


# Compliling
The platformio platform is recommended for compiling the code.  Ensure that
support for teh Arduino Leonardo board is installed.  

There is a separate build specification for each of the two board types. 
Ensure that you build the correct one for your board, otherwise the 
buttons and led will not work correctly. 


