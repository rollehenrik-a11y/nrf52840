1. Plug in the board

When you connect it via USB, it will create a USB serial device (usually /dev/ttyACM0).
Check with:

ls /dev/ttyACM*


You should see something like:

/dev/ttyACM0

2. Make sure you have permission

By default, Ubuntu may block access unless you’re in the dialout group.
Add your user:

sudo usermod -a -G dialout $USER


Then log out and back in.

3. Connect with a terminal tool

You can use any serial terminal program. Common ones:

screen (simple and installed by default on many systems):

screen /dev/ttyACM0 115200
