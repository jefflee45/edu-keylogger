
A simple Linux kernel keylogger that writes the keystrokes to a folder in /sys/kernel/debug. The keylogger works by adding a listener to the list of keyboard event notifiers by using register_keyboard_notifier(). Instead of getting the raw scancode, we take the already translated keycode, and convert it to a string. The keymap for a US keyboard is outlined in usr/include/linux/input-event-codes.h. This has only been tested on Linux 5.11 on a VM, but probably works for other versions of Linux 5.X

This was meant to be an educational exercise into learning about kernel modules, and not something to be used maliciously.

References and Tutorials used:
https://sysprog21.github.io/lkmpg/
https://packetstormsecurity.com/files/26467/writing-linux-kernel-keylogger.txt.html
http://redgetan.cc/lets-write-a-kernel-keylogger/
http://lxr.linux.no/


Future Improvement Ideas:
If we assume we already have root access on a machine, we could create a reverse shell so we can send back the data from the keylogger. 
Could definitely improve the stealthiness of the module, and make it harder to remove the module.
The makefile could also be expanded on. 

