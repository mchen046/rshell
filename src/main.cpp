#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <boost/tokenizer.hpp>
#include <boost/token_iterator.hpp>

using namespace std;
using namespace boost;

char** parseString(string cmd);

void checkExitCmd(char* cmdText){ //exit command
	char exitText[] = {'e','x', 'i', 't'};
	bool same = true;
	for(int i = 0; cmdText[i]!='\0' && same; i++){
		if(cmdText[i]!=exitText[i])
			same = false;
	}
	if(same)
		exit(1);
}

int main(){
	while(1){
		string cmd;
		cout << "$ ";
		getline(cin, cmd);

		//convert string to char[]
		char *cmdArray = new char[cmd.size()];
		for(int i=0; i<static_cast<int>(cmd.size()); i++){
			cmdArray[i] = cmd.at(i);
		}
		//cout << "cmdArray: " << cmdArray << endl;
		
		bool cmdLineDone = false;
		char *cmdLine = strtok(cmdArray, ";"); //parsing ";"
		while(!cmdLineDone){
			//cout << "cmdLine: " << cmdLine << endl;
			
			//parsing " "
			string str = static_cast<string>(cmdLine);
			char_separator<char> delim(" ");
			tokenizer< char_separator<char> > mytok(str, delim);

			//creating cmdLinePart
			char **cmdLinePart = new char*[cmd.size()];
			int i = 0;
			for(tokenizer <char_separator<char> >::iterator it = mytok.begin(); it!=mytok.end(); it++){
				char *token = new char[cmd.size()];
				string cmdString = static_cast<string>(*it);
				for(unsigned int j = 0; j!=cmdString.size(); j++){
					token[j] = cmdString[j];
				}
				cmdLinePart[i] = token;
				i++;
			}

			cmdLine = strtok(NULL, ";"); //parse ";"
			if(cmdLine==NULL){
				cmdLineDone = true;
			}
			checkExitCmd(cmdLinePart[0]); //check for exit command

			//process spawning
			int pid = fork();
			//cout << "pid: " << pid << endl;
			if(pid == -1){ //error
				perror("fork() error");
				exit(1);
			}
			else if(pid == 0){ //child process
				//cout << "in child process\n";
				if(execvp(cmdLinePart[0], cmdLinePart) == -1){
					perror("error in execvp");
				}
				exit(1);
			}
			else if(pid > 0){ //parent process
				if(wait(0) == -1){
					perror("error in wait()");
				}
			}
		}
		//delete[] *cmdstrtok; //deallocate cmdstrtok
		//delete[] cmdArray;
	}
	return 0;
}
