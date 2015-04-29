/*
Michael Chen mchen046@ucr.edu
SID: 861108671
CS100 Spring 2015: hw1-ls
https://www.github.com/mchen046/rshell/src/ls.cpp
*/
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <string.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <boost/tokenizer.hpp>
#include <boost/token_iterator.hpp>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <set>
#include <pwd.h>
#include <grp.h>
#include <time.h>

using namespace std;
using namespace boost;

void printVect(vector<string> cmd, bool a){ 
	for(unsigned int i = 0; i<cmd.size(); i++){
		if(a || cmd[i][0]!='.'){
			cout << cmd[i] << "  ";
		}
	}
	cout << endl;
}

bool hasText(string cmdText, string text){
	unsigned int count = 0;
	int j = 0;
	for(unsigned int i = 0; i<cmdText.size(); i++){
		if(cmdText[i]!=text[j]){
			count = 0;
			j = 0;
		}
		if(cmdText[i]==text[j]){
			count++;
			j++;
		}
		if(count == text.size()){
			return true;
		}
	}
	return false;
}

bool isText(char* cmdText, string text){
	if(!(cmdText[text.size()-1]==text[text.size()-1] && cmdText[text.size()]=='\0' && text[text.size()]=='\0')){
		return false;
	}
	for(unsigned int i = 0; cmdText[i]!='\0' && i<text.size(); i++){
		if(cmdText[i]!=text[i])
			return false;
	}
	return true;
}

int lsOrg(vector<string> cmd, vector<string> &cmdFiles, set<char> &cmdFlags){
	for(unsigned int i = 1; i<cmd.size(); i++){
		if(cmd[i][0]=='-'){ //is a flag
			for(int j = 1; cmd[i][j]!='\0'; j++){
				if(cmd[i][j]=='a' || cmd[i][j]=='l' || cmd[i][j]=='R'){
					cmdFlags.emplace(cmd[i][j]);
				}
				else{
					cmdFlags.clear();
					cout << "ls: invalid option -- '" << cmd[i][j] << '\'' << endl;
					return -1;
				}
			}
		}
		else{ //is a file name
			cmdFiles.push_back(cmd[i]);
		}
	}
	return 1;
}

void getFiles(vector<string> cmdFiles, vector<string> &fileList){
	if(cmdFiles.empty()){
		cmdFiles.push_back(".");
	}
	for(unsigned int i = 0; i<cmdFiles.size(); i++){
		DIR *dirp;
		//converting string to char*
		char *dir = new char[cmdFiles[i].size()+1];
		for(unsigned int j = 0; j<cmdFiles[i].size()+1; j++){
			dir[j] = cmdFiles[i][j];
			if(j==cmdFiles[i].size()){
				dir[j] = '\0';
			}
		}

		if(NULL == (dirp = opendir(dir))){
			cout << "ls: cannot access " << cmdFiles[i] << ": No such file or directory" << endl;
			perror("opendir");
		}

		errno = 0;
		struct dirent *filespecs;
		while( NULL != (filespecs = readdir(dirp))){
			fileList.push_back(filespecs->d_name);
		}

		if(errno!=0){
			perror("readdir");
			exit(1);
		}

		if(closedir(dirp) == -1){
			perror("closedir");
			exit(1);
		}
	}
}

void flag_l(vector<string> fileList, bool a){
	struct stat s;
	struct passwd *userID;
	struct group *groupID;

	//total number of blocks
	int total = 0;
	for(unsigned i  = 0; i<fileList.size(); i++){
		if(a || fileList[i][0] != '.'){
			if(stat(fileList[i].c_str(), &s)<0){
				perror("stat");
				exit(1);
			}
			total+=s.st_blocks;
		}
	}
	cout << "total " << total/2 << endl;

	for(unsigned int i = 0; i<fileList.size(); i++){
		if(a || fileList[i][0]!='.'){
			if(stat(fileList[i].c_str(), &s)<0){
				perror("stat");
				exit(1);
			}
			
			//permissions
			cout << ((S_IFDIR & s.st_mode)?"d":"-")
				<< ((S_IRUSR & s.st_mode)?"r":"-")
				<< ((S_IWUSR & s.st_mode)?"w":"-")
				<< ((S_IXUSR & s.st_mode)?"x":"-")
				<< ((S_IRGRP & s.st_mode)?"r":"-")
				<< ((S_IWGRP & s.st_mode)?"w":"-")
				<< ((S_IXGRP & s.st_mode)?"x":"-")
				<< ((S_IROTH & s.st_mode)?"r":"-")
				<< ((S_IWOTH & s.st_mode)?"w":"-")
				<< ((S_IXOTH & s.st_mode)?"x":"-");

			//number of links
			cout << ' ' << s.st_nlink << ' '; 

			//userID
			if(!(userID = getpwuid(s.st_uid))){
				perror("getpwuid");
				exit(1);
			}
			cout << userID->pw_name << ' ';

			//groupID
			if(!(groupID = getgrgid(s.st_gid))){
				perror("getgrgid");
				exit(1);
			}
			cout << groupID->gr_name << ' ';

			//size of file/dir
			cout.width(4); cout << right << s.st_size << ' '; //cout width temporary fix

			//time using struct tm timeInfo
			//error check with NULL
			char date[15];
			struct tm *timeInfo;
			if(NULL == (timeInfo = localtime(&(s.st_mtime)))){
				perror("localtime");
				exit(1);
			}
			strftime(date, 15, "%b %d %H:%M", localtime(&(s.st_mtime)));
			cout << date << ' ' << fileList[i] << endl;
		}
	}
}

void lsExec(vector<string> cmdFiles, string flags){
	vector<string> fileList;
	getFiles(cmdFiles, fileList);

	//checking which flags to use
	if(hasText(flags, "R")){
		if(hasText(flags, "l") && !hasText(flags, "a")){ // -lR
		}
		else if(hasText(flags, "a") && !hasText(flags, "l")){ // -aR
		}
		else if(hasText(flags, "a") && hasText(flags, "l")){ // -alR
		}
		else if(!hasText(flags, "a") && !hasText(flags, "l")){ // -R

		}
	}
	else if(hasText(flags, "l") && !hasText(flags, "a")){ // -l
		flag_l(fileList, false);
	}
	else if(hasText(flags, "a") && !hasText(flags, "l")){ // -a
		printVect(fileList, true);
	}
	else if(hasText(flags, "a") && hasText(flags, "l")){ // -la
		flag_l(fileList, true);
	}
	else if(!hasText(flags, "a") && !hasText(flags, "l") && !hasText(flags, "R")){ // ls
		printVect(fileList, false);
	}
}

int ls(vector<string> cmd){
	//conduct error checking here

	vector<string> cmdFiles;
	set<char> cmdFlags;
	if(lsOrg(cmd, cmdFiles, cmdFlags)==-1){
		return -1;
	}

	//printVect(cmdFiles);
	/*for(auto it = cmdFlags.begin(); it!=cmdFlags.end(); it++){
		cout << *it << endl;
	}*/
	
	//converting set<char> to string
	string flags;
	for(auto it = cmdFlags.begin(); it!=cmdFlags.end(); it++){
		flags.push_back(*it);
	}

	lsExec(cmdFiles, flags);

	return 1;
}
int main(int argc, char** argv){
	vector<string> cmd;
	for(int i = 0; i<argc; i++){
		cmd.push_back(argv[i]);
	}

	ls(cmd);
	
	return 0;
}
