# rshell
rshell is a basic command shell using the syscalls fork, execvp, and wait.
###Installation
	$ git clone https://github.com/mchen046/rshell.git
	$ cd rshell
	$ git checkout hw0
	$ make
	$ bin/rshell
###Bugs/Limitations/Issues
**1. commands with a single &**

`echo hello & echo world` outputs `hello & echo world` while bash outputs the pid, and each command from right to left. See [example](http://bashitout.com/2013/05/18/Ampersands-on-the-command-line.html).



