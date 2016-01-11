## version 2016.Jan.11

  - fix build failure on Mac OS
  - export serial ports over telnet
  - allow to daemon under telnet mode


## version 2015.Nov.14

  - add startup options for serial port
    * stopbits
    * parity
    * flow control type
    * csize
    * baudrate
  - add terminal commands
    * list: list serial port
    * connect: connect other serial port
    * speed: change baudrate
    * csize: change number of data bits
    * stopbits: change number of stopbits
    * parity: change parity type
    * flow: change flow type
    * ks: send file using Kermit
  - fix serial port can not be matched right
  - fix failed to open other serial ports if 1st port opened
  - consider ttyACM device as serial port


## version 2015.10.8

  - initial version
  - works on Linux and Mac OS X
  - open one idle seril port automatically
  - exit automatically if serial ports opened are removed
  - control termial. Trigger key is Ctrl-]
  - send file via serial communication protocol: kermit,xmodem,ymodem
