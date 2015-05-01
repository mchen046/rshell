# rshell
rshell is a basic command shell that uses the syscalls fork, execvp, and wait. Please see the *Bugs/Limitations/Issues* section describing unwanted behavior.
- prompts the user with `login@hostname$ ` 
- handles a combination of `;`, `&&`, and `||` connectors

### Installation
To install and execute rshell, run these commands in your favourite bash shell:
```
$ git clone https://github.com/mchen046/rshell.git
$ cd rshell
$ git checkout hw0
$ make
$ bin/rshell
```
### Bugs/Limitations/Issues
**1. commands with a single &**

`$ echo hello & echo world` 

- rshell prints `hello & echo world`

- bash prints 
```
[1] 29338
world
hello
[1]+ Done					echo hello
```
See [documentation](http://bashitout.com/2013/05/18/Ampersands-on-the-command-line.html).

**2. commands with a single |**

`$ echo hello | echo world`

- rshell prints `hello | echo world`

- bash prints `world`

**3. does not support output redirection**

`$ echo hello world > filename`
 
- rshell prints `hello world > filename`

- bash redirects the output content of `echo` to a file `filename`

Output redirection will be implemented in the future.

**4. improper && and || connector entries**

`ls |& echo hello |& echo world`

- rshell prints `rshell: syntax error near unexpected token '&'`

- bash executes the rightmost command, in this case `echo world`, and prints `world`

`ls &&&&&&&`

- rshell prints `rshell: syntax error near unexpected token '&'`

- bash prints `bash: syntax error near unexpected token '&&'`

- Note the discrepancy between the unexpected tokens '&' and '&&'

**5. `ctrl-z` and `fg`**

Pressing `ctrl-z` to put rshell in the background and pressing `fg` to return rshell to the foreground prevents rshell from outputting an initial prompt `$`.

**6. multiple rshell processes**

Running `rshell` within rshell spawns a new process that runs within the original rshell. Multiple entries of `exit` are required to end all instances of the rshell process.

**7. does not support the bash command `cd`**

`cd` will be implemented in the future.

**8. execution of && and || connectors**

rshell executes commands with both && and || connectors a little differently than bash. rshell takes the command

`echo hello && echo world && sdsdsd || echo my || sdsdsd && echo michael` 

and logically separates it into 

`(echo hello && echo world && sdsdsd) || (echo my) || (sdsdsd && echo michael)`

without actually coding in the parentheses. rshell logically divides the command into a list of commands that contain only &&, which are then separated by ||, and vice versa. To clarify, here is another example:

`echo hello || echo world || sdsdsd && echo my && sdsdsd || echo michael` 

becomes

`(echo hello) || (echo world) || (sdsdsd && echo my && sdsdsd) || (echo michael)`

Commands still follow normal connector logic:
- If a command is followed by `||`, then the next command is executed only if the first one failed.
- If a command is followed by `&&`, then the next command is executed only if the first one succeeded.
- If a command is followed by `;`, then the next command is always executed.
--------------------------------------------------------------------------------
ls -l / displays years on some timestamps instead of the H:M:s format
ls -l displays 08 instead of 8 on day of month




