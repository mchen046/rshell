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

#define STR_SIZE "%7ld "

using namespace std;
using namespace boost;

void chop(string text){
	char_separator<char> delim("/");
	tokenizer<char_separator<char> >mytok(text, delim);
	auto it = mytok.begin();
	for(; it != mytok.end(); it++){
		auto itA = it;
		if(++itA==mytok.end()){
			cout << *it;
		}
	}
}

int fileWidth(vector<string> fileList){
	unsigned int max = 0;

	for(unsigned i = 0; i<fileList.size(); i++){
		if(fileList[i].size()>max){
			max = fileList[i].size();
		}
	}
	return max;
}

void printVect(vector<string> cmd, bool a){ 
	int width = fileWidth(cmd);
	unsigned int j = 0;
	cout << width << endl;
	for(unsigned int i = 0; i<cmd.size(); i++){
		if(a || (cmd[i][0]=='.' && cmd[i][1]=='/') || cmd[i][0]!='.'){
			if((j+=width)>80){
				cout << endl;
				j = 0;
			}
			cout.width(width); cout << left;
			if(cmd[i][0]=='.'){
				chop(cmd[i]);
			}
			else{
				cout << cmd[i];
			}
			cout << ' ';
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
	for(unsigned int i = 0; i<cmd.size(); i++){
		//cout << cmd[i] << endl;
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
		else{ //is a file/dir name
			cmdFiles.push_back(cmd[i]);
		}
	}

	return 1;
}

void getFiles(string dirString, vector<string> &fileList){
	DIR *dirp;
	//converting string to char*
	char *dir = new char[dirString.size()+1];
	for(unsigned int i = 0; i<dirString.size()+1; i++){
		dir[i] = dirString[i];
		if(i==dirString.size()){
			dir[i] = '\0';
		}
	}

	if(NULL == (dirp = opendir(dir))){
		cout << "ls: cannot access " << dirString << ": No such file or directory" << endl;
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
	sort(fileList.begin(), fileList.end());
}

int linkWidth(vector<string> fileList, string parent, bool a){
	unsigned int max = 0;
	struct stat s;
	string absolName;
	
	for(unsigned i = 0; i<fileList.size(); i++){
		absolName = parent + '/' + fileList[i];
		if(a || (fileList[i][0]=='.' && fileList[i][1]=='/') || fileList[i][0]!='.'){
			if(stat(absolName.c_str(), &s)<0){
				perror("stat");
				exit(1);
			}

			if(to_string(s.st_nlink).size()>max){
				max = to_string(s.st_nlink).size();
			}
		}
	}
	return max;
}

int sizeWidth(vector<string> fileList, string parent, bool a){
	unsigned int max = 0;
	struct stat s;
	string absolName;
	
	for(unsigned i = 0; i<fileList.size(); i++){
		absolName = parent + '/' + fileList[i];
		if(a || (fileList[i][0]=='.' && fileList[i][1]=='/') || fileList[i][0]!='.'){
			if(stat(absolName.c_str(), &s)<0){
				perror("stat");
				exit(1);
			}

			if(to_string(s.st_size).size()>max){
				max = to_string(s.st_size).size();
			}
		}
	}
	return max;
}

void flag_l(vector<string> fileList, string parent, bool a){ //add parent parameter
	struct stat s;
	struct passwd *userID;
	struct group *groupID;

	int linkSpace = linkWidth(fileList, parent, a);
	int	sizeSpace = sizeWidth(fileList, parent, a);

	//total number of blocks
	int total = 0;
	//cout << "fileList.size(): " << fileList.size() << endl;
	
	string absolName;
	for(unsigned i  = 0; i<fileList.size(); i++){
		absolName = parent + '/' + fileList[i];
		//cout << "absolName: " << absolName << endl;
		if(a || (fileList[i][0]=='.' && fileList[i][1]=='/') || fileList[i][0]!='.'){
			if(stat(absolName.c_str(), &s)<0){
				perror("stat");
				exit(1);
			}
			total+=s.st_blocks;
		}
	}
	cout << "total " << total/2 << endl;

	for(unsigned int i = 0; i<fileList.size(); i++){
		absolName = parent + '/' + fileList[i];
		if(a || (fileList[i][0]=='.' && fileList[i][1]=='/') || fileList[i][0]!='.'){
			//cout << "fileList[" << i << "]: " << fileList[i] << endl;
			if(stat(absolName.c_str(), &s)<0){
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
				<< ((S_IXOTH & s.st_mode)?"x":"-")
				<< ' ';

			//number of hard links
			cout.width(linkSpace); cout << right << s.st_nlink << ' '; 

			//userID
			if(NULL == (userID = getpwuid(s.st_uid))){
				perror("getpwuid");
				exit(1);
			}
			cout << userID->pw_name << ' ';

			//groupID
			if(NULL == (groupID = getgrgid(s.st_gid))){
				perror("getgrgid");
				exit(1);
			}
			cout << groupID->gr_name << ' ';

			//size of file/dir
			cout.width(sizeSpace); cout << right << s.st_size << ' '; //cout width temporary fix

			//time using struct tm
			//error check with NULL
			char date[15];
			struct tm *timeInfo;
			if(NULL == (timeInfo = localtime(&(s.st_mtime)))){
				perror("localtime");
				exit(1);
			}
			strftime(date, 15, "%b %d %H:%M", localtime(&(s.st_mtime)));
			cout << date << ' ';
			chop(fileList[i]);
			cout << endl;
		}
	}
}

void flag_R(vector<string> fileList, string parent, bool a, bool l){
	struct stat s;
	vector<string> fileListNew;

	for(unsigned int i = 0; i<fileList.size(); i++){
		if((fileList[i]!="." && fileList[i]!="..") && (a || (fileList[i][0]=='.' && fileList[i][1]=='/') || fileList[i][0]!='.')){
			string absolName = parent + "/" + fileList[i];
			//cout << "absolName: " << absolName << endl;
			if(stat(absolName.c_str(), &s)<0){
				perror("stat");
				exit(1);
			}
			if(S_IFDIR & s.st_mode){ //is a dir
				cout << absolName << ':' << endl;

				getFiles(absolName, fileListNew);
				if(l){
					flag_l(fileListNew, absolName, a);
				}
				else{
					printVect(fileListNew, a);
				}
				cout << endl;
				flag_R(fileListNew, absolName, a, l);
			}
		}
		fileListNew.clear();
	}
}

void lsExec(vector<string> cmdFiles, string flags){
	vector<string> fileList;
	string parent;
	struct stat s;
	if(cmdFiles.empty()){
		cmdFiles.push_back(".");
	}


	for(unsigned int i = 0; i<cmdFiles.size(); i++){
		cout << cmdFiles[i] << endl;
		if(stat(cmdFiles[i].c_str(), &s)<0){
			perror("stat");
		}
		
		if(S_IFDIR & s.st_mode){ //only if a dir
			getFiles(cmdFiles[i], fileList);
		}
		else{
			fileList.push_back(cmdFiles[i]);
		}

		//cout << "fileList: ";
		//printVect(fileList, true);
		//cout << flags << endl;

		if(fileList.empty()){
			parent = ".";
		}
		else{
			parent = cmdFiles[i];
		}

		if(cmdFiles.size()!=1 || hasText(flags, "R")){
			cout << parent << ':' << endl;
		}

		//checking which flags to use
		if(hasText(flags, "R")){
				if(hasText(flags, "l") && !hasText(flags, "a")){ // -lR
				flag_l(fileList, parent, false);
				cout << endl;
				flag_R(fileList, parent, false, true);	
			}
			else if(hasText(flags, "a") && !hasText(flags, "l")){ // -aR
				printVect(fileList, true);
				cout << endl;
				flag_R(fileList, parent, true, false);	
			}
			else if(hasText(flags, "a") && hasText(flags, "l")){ // -alR
				flag_l(fileList, parent, true);
				cout << endl;
				flag_R(fileList, parent, true, true);
			}
			else if(!hasText(flags, "a") && !hasText(flags, "l")){ // -R
				printVect(fileList, false);
				//cout << endl;
				flag_R(fileList, parent, false, false);
			}
		}
		else if(hasText(flags, "l") && !hasText(flags, "a")){ // -l
			flag_l(fileList, parent, false);
		}
		else if(hasText(flags, "a") && !hasText(flags, "l")){ // -a
			printVect(fileList, true);
		}
		else if(hasText(flags, "a") && hasText(flags, "l")){ // -la
			flag_l(fileList, parent, true);
		}
		else if(!hasText(flags, "a") && !hasText(flags, "l") && !hasText(flags, "R")){ // ls
			printVect(fileList, false);
		}
		fileList.clear();
		if(i+1 != cmdFiles.size()){
			cout << endl;
		}
	}
}

int ls(vector<string> cmd){
	//conduct error checking here


	vector<string> cmdFiles;
	set<char> cmdFlags;
	if(lsOrg(cmd, cmdFiles, cmdFlags)==-1){
		return -1;
	}

	cout << "cmdFiles: ";
	printVect(cmdFiles, true);
	cout << "flags: ";
	for(auto it = cmdFlags.begin(); it!=cmdFlags.end(); it++){
		cout << *it << endl;
	}
	cout << endl;


	
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
	for(int i = 1; i<argc; i++){
		//cout << argv[i] << endl;
		if(argv[i][0]!='.' && argv[i][0]!='/' && argv[i][0]!='-'){
			cmd.push_back("./" + static_cast<string>(argv[i]));
		}
		else{
			cmd.push_back(argv[i]);
		}
	}

	ls(cmd);
	
	return 0;
}
