Instructions on how to install the Base2Cart package so that the Base Unit will be configured to validate a game cartridge. 

How to install package:

A.  Using an Automated FLASHING ATAP version 11 or higher

    Requires: An Automated Flashing ATAP version 011 or newer
	      One Didj Unit
	      One Didj USB cable 
	      One Didj Power supply
	      One PC

	1) Before powering on, plug a USB cable into the Unit with the Flashing ATAP configured to boot from the ATAP
	2) Plug the other end into a Host PC
	3) Power on the unit.  Two USB drives should appear after power on (Didj and MFG_PAYLOAD)
	4) Place this package in the 'Packages' directory of the /MFG_PAYLOAD usb drive.
	5) Eject the drive by right clicking the icon and selecting 'Eject' or 'Unmount'
	6) The ATAP will now install the package automatically and configure the device to be a Cart validator
	7) Put the newly configured ATAP into the Base Unit you want to program as a Cart validator
	8) Without any USB cable plugged in, power on the unit (the flashing process will occure)
	9) After flashing, remove the ATAP card and reboot the unit
	10) Plug in the unit through USB to the host PC
	11) Put the Cartridge content package (.lfp file) in the 'Cart' folder on the /Didj USB drive.
		The 'Cart' folder should contain only the package files representing what is on the cartridge.  	
	12) Power off the unit.
	13) The unit will now automatically validate any cartridge against the packages in the Cart directory.


B.  Manually installing the package on a pre-programmed base unit

    Requires: An ATAP card
	      One Preprogrammed Didj Unit with the correct firmware as per the manufacturing test specs
	      One Didj USB cable 
	      One Didj Power supply
	      One Serial Port connection from ATAP to PC
	      One PC

	1) Configure an ATAP to be a UART Only and insert into the pre-programmed unit you want to configure as the ATAP Programmer
	2) Connect the unit to a serial terminal 
	3) Start the serial terminal and boot up the unit with the ATAP installed (you should see the startup sequence in the serial terminal).
	4) Connect the unit to a host PC through USB.  
	5) In the serial terminal, type the following command to gain access to the USB drive:
		usbctl -d mass_storage -a unlock
		usbctl -d mass_storage -a enable  <-- if these don't work after 3 seconds, unplug the USB and replug it back in
	6) Copy the Base2Cart-xxx.lfp to the /Didj USB drive that shows up
	7) Create a folder called "Cart" (case sensative) on the /Didj USB drive.
	8) Put the Cartridge content package (.lfp file) in the 'Cart' folder on the /Didj USB drive.
		The 'Cart' folder should contain only the package files representing what is on the cartridge.
	7) DO NOT UNPLUG THE USB CABLE but eject the drive by right clicking the icon and selecting 'Eject' or 'Unmount'
	8) In the serial terminal, install the package by typing the following command:
		usbctl -d mass_storage -a disable
		lfpkg -a install /Didj/Base2Cart-*.lfp
	9) Once it is done, power off the unit and unplug the USB cable. 
	10) The unit will now automatically validate any cartridge against the packages in the Cart directory.
