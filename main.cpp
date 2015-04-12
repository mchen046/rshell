#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char **argv){
	//char cmd[];
	//string cmdParsed[];	
	string cmd;
	cout << "$ ";
	getline(cin, cmd);
	//cout << "cmd.size(): " << cmd.size() << endl;	
	
	//convert string to char[]
	char cmdArray[cmd.size()];
	for(int i=0; i<cmd.size(); i++){
		cmdArray[i] = cmd.at(i);
	}
	

	//output cmdArray
	cout << "cmdArray: ";
	for(int i = 0; i<cmd.size(); i++){
		cout << cmdArray[i];
	}
	cout << endl;

	//strtok (possible bug reading in '@' on the fifth token)
	vector <char*> cmdstrtok;
	char *token = strtok(cmdArray, " ");
	while(token!='\0'){
		cout << "token: " << token << endl;
		cmdstrtok.push_back(token);
		token = strtok(NULL, " ");
	}

	return 0;
}
