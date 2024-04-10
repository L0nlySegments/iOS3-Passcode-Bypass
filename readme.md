## Prologue
This tool will bypass the lock-screen passcode on old iOS 3.1.3 devices (would pobably also work on lower versions, didn't try tho). 
The code is heavly based on the Spirit Jailbreak sourcecode by comex <3. 

This is an implementation of the MobileBackupCopy Exploit. It is used to null-out the /var/Keychains/keychain-2.db file, effecifly removing the lock-screen passcode without any further loss of data. 

The only downside of this method is that all WiFi Passwords will also be lost in the process. Everything else will be accessable. 

TODO: The current version will not re-activate a disabled iPhone/iPod Touch. I will try to implement that in a later version.

## Requirements:
This tool currently only works on OS X (tested on 12.6 with an m1 macbook pro).
Since I highly doubt that there is any demand for a iOS 3.1.3 passcode removal tool, it will stay this way. I only wrote this because I got a locked iPod form a flee-maket.

## Usage: 
Connect your iPhone/iPod touch to your computer and simply run ./bypass

There will be a restore message on the device follwed by a reboot. 
After this reboot the passcode should be removed. 

Enjoy :-)
l0nlySegments
