## tea don't bring response to telnet client

### environment
* OS: Mac OS X 10.11.2
* USB serial bridge chip: PL2303
* driver: [PL2303_MacOSX_1.6.0_20151022.zip](http://www.prolific.com.tw/UserFiles/files/PL2303_MacOSX_1_6_0_20151022.zip)

### reproduce procedure
* connect two PL2303 ports
* open two telnet client to access PL2302
* remove one serial port
* other client have no response

But if you add serial port back, telnet client come back again. So I think it's bug serial driver.


## echo does not work

### reproduce procedure
* tea don't listen on standard telent port 23
* open telnet client and press Ctrl-]. I use Mac OSX built-in telnet client
* send escape(Ctrl-]) to tea and then it will jump to tea's control terminal
* input chars
* you will find tea don't echo user inputs


When telnet client does nto connect to port 23, it send chars only after user press Enter,
but tea only echo char one by one interactively. Even tea can echo user inputs, it lose
interactive mode.
