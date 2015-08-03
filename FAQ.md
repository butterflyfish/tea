# FAQ

## Why **Tea** don't provide Log ?

Some Terminal provide this functionality, e.g. [iTerm2](https://www.iterm2.com).
If your Termimal don't provide it, you can use *tee* to log:

````sh
tea | tee > log.txt
````
