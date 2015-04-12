#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

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

	cout << "cmdstrtok[0]: " << cmdstrtok[0] << endl;
	if(cmdstrtok[0] == "ls"){
		cout << "ls command called!\n";
	}

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
