# Tea
Tea is a serial emulator designed for embedded system development.
It's a open source program licensed under four caluse BSD.


## Features

* open one idle seril port automatically
* exit program if serial port is removed
* control termial. Trigger key is Ctrl-]
    * jump back after press Enter

## How to build ?

Install dependency *libev* first, and then run *make*. Taking MAC OSX as an example:

```sh
brew install libev
git clone https://github.com/boot2linux/tea.git
cd tea
make
install -m 755 build/bin/tea /usr/local/bin
```
