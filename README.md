# rshell is a basic command shell using the syscalls fork, execvp, and wait.

### Installation

	$ git clone https://github.com/mchen046/rshell.git
	$ cd rshell
	$ git checkout hw0
	$ make
	$ bin/rshell

### Bugs/Limitations/Issues

**1. commands with a single &**

`$ echo hello & echo world` 

rshell prints `hello & echo world`

Bash prints 

	[1] 29338
	world
	hello
	[1]+ Done					echo hello

See [documentation](http://bashitout.com/2013/05/18/Ampersands-on-the-command-line.html).

**2. commands with a single |**

`$ echo hello | echo world` prints `world`

rshell prints `hello | echo world`

Bash prints `world`

**3. **
