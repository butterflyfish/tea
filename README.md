# Tea
[Tea] - A simple terminal program for the serial port.

## Features

* works on Linux and Mac OS X
* open one idle seril port automatically
* exit automatically if serial ports opened are removed
* control termial. Trigger key is Ctrl-]
* send file via serial communication protocol: kermit,xmodem,ymodem
* configure serial via option or terminal command


## How to build ?

[Tea] have zero dependency. Just need to *make* it.

```sh
git clone https://github.com/butterflyfish/tea.git
cd tea
make
make install
```

## Acknowledgement
* [E-Kermit]


## License
Licensed under four caluse BSD.

Copyright © 2015, Michael Zhu  boot2linux at gmail.com

All rights reserved.

[Tea]: https://github.com/butterflyfish/tea
[E-Kermit]: http://www.columbia.edu/kermit/ek.html
