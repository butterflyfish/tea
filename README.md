# Tea
[ Tea ]( https://github.com/boot2linux/tea ) - A simple terminal program for the serial port.

## Features

* works on Linux and Mac OS X
* open one idle seril port automatically
* exit automatically if serial ports opened are removed
* control termial. Trigger key is Ctrl-]
* send file via serial communication protocol: kermit

## How to build ?

Install dependency *libev* first, and then run *make*. Taking MAC OSX as an example:

```sh
brew install libev
git clone https://github.com/boot2linux/tea.git
cd tea
make
install -m 755 build/bin/tea /usr/local/bin
```

## License
licensed under four caluse BSD.
