# What is this thing?
PasswordThing (PT) is a physical device for storing passwords.  When you plug it
into a USB port on your computer, it pretends to be a keyboard, and when you
press the right button, it 'types' a password (optionally preceded by a
username).  

PT's main purpose is to allow you to set a long and complicated startup password on
your computer, without having to remember it.  The secondary purpose is to
allow you to set a long and complicated password on your password manager,
without having to remember it.  

The downside is that you are now carrying a physical device that lets anyone
with access to it (and knows its purpose) to log into your computer and/or 
password manager.  

Classic tradeof between security and convenience. 

# Quick start

You should really read the rest of the manual at your earliest convenience, but to get started:

- Plug your Password Thing (PT) into a USB port
- The first time, it may take a few seconds for your computer to install drivers and recognize the device
- The Colour LED should be blinking red and the Display should say **"Slot 0 I"**
- PT is now furiously generating entropy.  Give it about 10 seconds to do its thing
- Press the (G)enerate button for more than a second.  The display should now change to **"Slot 0 P"** 
- Now you have a random password in Slot 0, which will be entered via a virtual keyboard every time you press the (S)elect button
- Log into your computer and change the login password.  When prompted for the new password, press the (S)elect button on the PT
- Next time you log on to the computer, just press the (S)elect button to enter the generated password

# Introduction

This user guide explains how to use your new Password Thing 
(abbreviated as PT in the rest of this document).  

## WARNING

The Password Thing is a **convenience** device, **NOT** a security device. 
It increases your security by removing the temptation to choose easily-remembered
(and therefore easily guessable) master passwords.  It decreases your security
by storing your master password unencrypted in a device that can be lost or 
borrowed/stolen if you are not careful.  We recommend that you carry your PT 
on a keychain together with your other important keys.  

## Motivation and Purpose

We all use password managers to store our passwords for the hundreds or
thousands of websites and accounts we visit that require passwords. 

However, there are two passwords that we still need to memorize (or write on
a sticky note), namely our computer password and the password for the
password manager. 

The Password Thing allows you to generate strong passwords for your computer
and password manager, and to store them in a physical device, so that you do
not have to remember them.  

## Security

The Password Thing is intended to disrupt your workflow as little as
possible.  Also the nature of the Thing means that it cannot secure stored
passwords with a secure password, since that would defeat the whole purpose of
using the device in the first place. 

Therefore you should treat the PT as you would a physical key that protects
valuable assests, like a safe filled with valuables.  

The PT **can** be configured to require a 4-button sequence to be tapped before
it will operate.  This provides a basic minimal layer of security against casual 
use by unathorized people, but it will not deter a determined attacker, and can 
be trivially bypassed through brute force or by using the serial port
configuration / debug mechanism. 

### Threat model
Think of the PT as an old-fashioned key.  You may own, lease or rent a car
and/or a house which is secured by nothing more than a key.  If someoone
really wants to take your car, they can steal your key, or beat you with a
rubber hose until you give it to them.  Your only protection is the law. 

If you lose your key, there is a level of security afforded by the fact that 
it is not immediately obvious what car or house the key belongs to.  If
someone takes the key off your desk, they may know exactly where your car is
parked or where you live.   

The PT is the same, just for your computer.  If someone really wants to get
into your computer, they could always beat you with a rubber hose until you
give them the password.  

If you lose a PT, chances are that nobody will know what it is or who it
belongs to.  But if you walk away from your computer and leave the PT stuck
in a USB port, you make a thief's life much easier.  


# Theory of operation
The PT has six 'slots' for storing passwords and optionally userids.  Each
slot can store a userid and a password up to 30 characters in length.  

The PT can auto-generate a password for each slot, but userids have to be
entered through the configuration interface.  

To use the PT, connect it to a USB port, select the appropriate slot using
the Next button (see below for button assignments), move your cursor to the
password field (if needed) and press the Select button.  The PT will emulate
a USB keyboard and 'type' the password stored in the slot.  
button.  


# Models

## Model U
The PT model U has a female micro usb port, which can be connected to USB A
or C ports on your computer via an adapter cable.  It has an OLED display,
LED indicators and and three buttons.  

## Model A
The PT model A is tiny and plugs directly into a USB A port.  It has LED
indicators but no display and only 2 buttons.  


# Buttons
The function of the buttons can be re-assigned (See the Configuration section). 
The default functions are shown below.  

Each button behaves differently depending on whether it was pressed a short
time (less than one second) or longer than a second.  

## Short press on Select 
Sends the password stored in the current slot (if any) to the computer via
the emulated keyboard interface. The password entry is followed by an emulated
Enter key.  

If there is a userid stored in the slot (see Configurationsection) then the
PT will send the userid, followed by a Tab character, followed by the
password, followed by Enter.  

## Short press on Next or Long press on Select
Advances to the next Slot.  The PT model A does not have a Next button, so
the only way to select a slot is via a long press on Select. 

## Short press on Generate
Sends the password in the current slot to the computer, but without sending
an Enter key press.  This feature often comes in handy when changing
passwords.  The userid is never sent. 

## Long press on Generate
Generates a new password for the current slot.  By default, a 20 character
alphanumeric password, including special characters, is generated. 


# LED indicators
The PT has two indicator LEDs.  One is a monochrome led, which is only used to
indicate that the software in the PT is running, by blinking on and off
every second.  

The second LED can display colors and is used to indicate the curent slot or
mode.  

## White
The LED briefly flashes white during startup.  When Configuration mode is
selected (see configuration on the left), the LED is solid white until a
serial terminal program is connected.  Once the terminal is connected, the
LED remains white but turns off briefly once a second. 

## Yellow
When the PT is passcode-locked, the RGB led will be Yellow.  

## Other colors
The rest of the colors indicate the current Slot, as shown in the following
table. 

|Slot|Color|
|---|---|
| 0 | Red |
| 1 | Yellow |
| 2 | Green |
| 3 | Magenta |
| 4 | Blue |
| 5 | Cyan|

When a slot contains a valid password, the LED shows a solid color,
otherwise it blinks at a fast rate. 

# OLED display

The OLED display on the Model U shows the current Slot number and indicates if the slot contains a 
**U**serid, **P**assword or both (**UP**).  Invalid / uninitialized slots are indicated by **I**. 

If the slot was named (See Configuration), the name will be displayed instead of the slot number.

# Best Practice

The PT can fail, or it can lose its nonvolatile memory due to a bug or other
issue.  It is therefore crucual to ensure that it is not a single point of
failure.  

For eaxample, think about what would happen if the only copy of your
password manager password is stored in your PT.  If you lose it, or it
breaks, you are locked out of all your passwords.  

1. When you generate a new important password, you should make a copy of the
password **immediately** and store it somehwere safe.  
2. Good practice is to store a copy of all your passwords on another PT and
to keeup that in a safe place offsite (like a safe deposit box).  
