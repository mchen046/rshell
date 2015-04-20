# rshell
rshell is a basic command shell using the syscalls fork, execvp, and wait.
### Installation
To install rshell, run these commands:
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
`$ echo hello | echo world`
rshell prints `hello | echo world`
Bash prints `world`
**3. `ctrl-z` and `fg`**
Pressing `ctrl-z` to put rshell in the background and pressing `fg` to return rshell to the foreground prevents rshell from outputting an initial prompt `$`.
**4. multiple rshell processes**
Running `rshell` within rshell spawns a new process that runs within the original rshell. Multiple entries of `exit` are required all processes.
**5. does not support output redirection
`echo > filename`
