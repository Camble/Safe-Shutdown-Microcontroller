# Safe Shutdown Microcontroller

Background
----------
To extend the functionality of my Safe Shutdown boards, I have decided to make use of a microcontroller to actively monitor the power switch, as well as adding functionality such as analog battery voltage monitoring.

Originally using an ATTiny85 chip and the I²C bus, it is possible to request information from the MCU using only two GPIO pins. The limitation of the ATTiny85 is the storage space. 6kb with 2kb reserved for the bootloader. While the final product would not have a bootloader, this is still very restrictive. The ATTiny85 also only has 6 available I/O pins.

Required Hardware and Components
--------------------------------
- [Raspberry Pi Zero](https://www.raspberrypi.org/products/pi-zero/) (or Model B+, Raspberry Pi 2 and Pi 3)
- [ATTiny167 Development Board](http://digistump.com/getpro) (or an alternative Arduino compatible board with ADC pins)

Current State
-------------
At the moment, the C code creates two independent tasks, one to check the power switch and one to read and store the battery voltage. The timing of these can be modified. I have debugging through the DigiKeyboard library, which allows output via USB to a notepad window.

I'm currently working on making sure the battery voltages are in a usable format.

Next Up...
-------------
I'll be working on the python script to compliment the MCU code. Information stored in a struct is written to a byte array and sent via I2C when requested. Because this version uses an ATTiny167, there are more pins and storage space (14kb) to add extra functionality if required.

Links
-----
More detail can be found on this thread:
http://sudomod.com/forum/viewtopic.php?f=20&t=1768

Feel free to contact me on the [Sudomod forums] (www.sudomod.com/forum) or on the [Sudomod Discord channel] (https://discordapp.com/channels/188359728454303744/188359728454303744)
