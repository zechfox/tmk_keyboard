Alternative Controller for Cherry G84-4125
============================================
Cherry G84-4125 was industry keyboard with RS485 interface. 
But someone make the new control board on atmega32 and sell it on taobao.com.
I want to add bluetooth for the keyboard, and the seller didn't provide source code.
So I made it base on tmk_keyboard project in github. 

##Features
* Customizable keymap
* 2 keymap layers(Fn keys)
* Mouse keys
* Bluetooth

See README of [tmk_keyboard] for more.

[tmk_keyboard]: http://github.com/tmk/tmk_keyboard
 
###Pros
* No risks: Everything is all reversible
* No need for PCB trace patching, case cutting or any other destructive mod
* Can keep original controller intact
* Can change all keyboard behavior as you like

###Cons
* Void your warranty

##DISCLAIMER
I'm not a professional of electronics nor MCU programming. This may damage your keyboard.
And my English writing is poor, I'm not sure I can convey my notions accurately.


##Documents
[doc] contains circuit of controller board and GPIO connection. 
There maybe some mistake and missing information. 
Please double check when you make your own project.


##Build Firmware & Program
See [this document](../../doc/build.md) first.

### Configuration
Set `MCU`, `BOOTLOADER_SIZE` and other build options in `Makefile` and `config.h`. 

### Build 
Just `make` like:

    $ make 


### Program
Programming the g84-4125.hex via ISP. 
The seller enabled chip encryption and enable bootloader. 
Please erase the chip before your programming. And it's NOT reversible. 
The project need disable bootloader due to limited FLASH space. You can move keymap to eeprom to save FLASH space.
You can reference the [doc/fuse.jpg] for fuse configuration.


##Keymap
To define your own keymap create file named `keymap_<name>.c` and see [keymap document](../../doc/keymap.md) and existent keymap files.


##Hardware
You have some options for hardware. Development boards with USB AVR family(ATMega32U4, AT90USB1286) like Teensy will work while MegaAVR with [V-USB] library is also cheaper option for DIY.

[LUFA]: http://www.fourwalledcubicle.com/LUFA.php
[PJRC]: http://www.pjrc.com/teensy/usb_keyboard.html
[dfu-programmer]: http://dfu-programmer.sourceforge.net/
[FLIP]: http://www.atmel.com/tools/FLIP.aspx
[Teensy Loader]: http://www.pjrc.com/teensy/loader.html
[V-USB]: http://www.obdev.at/products/vusb/index.html
