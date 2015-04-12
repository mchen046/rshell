#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

char** parseString(string cmd);

void checkExitCmd(char* cmdstrtok[]){ //exit command
	char exitText[] = {'e','x', 'i', 't'};
	bool same = true;
	for(int i = 0; cmdstrtok[0][i]!='\0' && same; i++){
		if(cmdstrtok[0][i]!=exitText[i])
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
		char cmdArray[cmd.size()];
		for(int i=0; i<cmd.size(); i++){
			cmdArray[i] = cmd.at(i);
		}
		
		//strtok (possible bug reading in '@' on the fifth token)
		char *cmdstrtok[cmd.size()];
		char *token = strtok(cmdArray, " ");
		for(int i = 0; token!='\0'; i++){
			cout << "token: " << token << endl;
			cmdstrtok[i] = token;
			token = strtok(NULL, " ");
			if(token == '\0'){
				cmdstrtok[i+1] = token;
			}
		}

		checkExitCmd(cmdstrtok);

		//process spawning
		int pid = fork();
		cout << "pid: " << pid << endl;
		if(pid == -1){ //error
			perror("fork() error");
			exit(1);
		}
		else if(pid == 0){ //child process
			cout << "in child process\n";
			if(execvp(cmdstrtok[0], cmdstrtok) == -1){
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
	return 0;
}
