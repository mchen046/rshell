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
### Bugs/Limitations/Issues for rshell
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

### Bugs/Limitations/Issues for the ls command

**1. `do_ypcall: clnt_call: RPC: Unable to send; errno = Operation not permitted`**

`ls -laR` seems to causes the error `do_ypcall: clnt_call: RPC: Unable to send; errno = Operation not permitted` on large/hidden files such as `.git` where the -R flag accesses some directories/files where read permission is not allowed.

**2. display order**

GNU ls displays files and directories in an order that first prints the `.` and `..` folders, then disregards whether or not the file is hidden or not; the non-case sensitive letter takes precedence, then the lower case letter, then the upper case letter.

The implemented subpart of the GNU ls command orders files and directories by ascii-betical algorithm; hidden folders take precedence, then upper case letters, and finally lower case letters.

**3. column width**

GNU ls is able to dynamically determine the width of a column based on the width of the current open terminal window.

The implemented subpart of the GNU ls command is set to a standard width of 80 characters.

**4. extra/missing new lines**

Various combinations of the `-l -a -R` flags impede proper newline placement between file and directory listings.

**5. symbolic links**

The implemented subpart of the GNU ls command is not able to display `->` for the parent link of symbolic links in the `-l` flag

**6. year and time display**

GNU ls occasionaly prints a files last modification date in the `month date time` format in the `-l` flag.

The implemented subpart of the GNU ls command only prints in the `month date time` format for the `-l` flag.  

**7. executing ls on an existing file**

When ls is called on an existing file, GNU ls prints out the file''s path.

The implemented subpart of the GNU ls command is not able to execute on an existing file.

### Bugs/Limitations/Issues for i/o redirection and piping

**1. `cat > eif`**

Standalone commands similar to cat > eif that wait on user input results in an input stream that can only be terminated by the ctrl c signal. Since proper signal capturing has not yet been implemented, the only way for the user to exit the input stream is by inputting ctrl c. However, this also ends the rshell program.

**2. invalid commands after a valid command in a command with multiple pipes**

For example, the command `ls -l cat < eif > eif2 | invalidcommand` will not output a proper error message for `invalidcommand` if it follows a valid command.

**3. memory leaks are present**

Due to multiple calls to new and memory allocation on the heap, rshell has not been properly sealed of memory leaks.

**4. bug with cppcheck**

Although not a bug with the rshell program itself, cppcheck prints an error messages on the call to `strcpy`, saying that the `Buffer is accessed out of bounds.` This is a known bug with cppcheck.

**5. improper pipe chaining with i/o redirection**

Commands similar to

`ls -l | grep eif | cat < eif > eif2 | cat < eif3 < eif4 eif5 < eif6`

are supposed to redirect the output of `cat eif` into `eif2` as well as executing `cat eif5`. However, rshell does not take into many similar accounts of pipe segments, specifically ones that contain i/o redirection and are chained side by side.




