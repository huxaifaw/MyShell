#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "

int isInternal(char* arglist[]);
void executeInternal(char* arglist[], char*);

int execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
int main(){
	char* cmdline;
	char** arglist;
	char* prompt = PROMPT;
	while((cmdline = read_cmd(prompt,stdin)) != NULL){
		if((arglist = tokenize(cmdline)) != NULL){
			int rv = isInternal(arglist);
			if(rv == 1){ //internal command
				executeInternal(arglist, prompt);
			}
			else{
				int i=0;
				execute(arglist);
			}
			for(int j=0; j< MAXARGS+1; j++)
				free(arglist[j]);
			free(arglist);
			free(cmdline);
		}
	}
	printf("\n");
	return 0;
}
int execute(char* arglist[]){
	int status;
	int cpid = fork();
	switch(cpid){
		case -1:
			perror("fork failed");
			exit(1);
		case 0:
			execvp(arglist[0], arglist);
			perror("Command not found...");
			exit(1);
		default:
			waitpid(cpid, &status, 0);
			return 0;
	}
}
char** tokenize(char* cmdline)
{
	char** arglist = (char**)malloc(sizeof(char*)* (MAXARGS+1));
	for(int j=0; j < MAXARGS+1; j++){
		arglist[j] = (char*)malloc(sizeof(char)* ARGLEN);
		bzero(arglist[j], ARGLEN);
	}
	if(cmdline[0] == '\0')//if user has entered nothing and pressed enter key
		return 0;
	int argnum = 0; //slots used
	char* cp = cmdline; //pos in string
	char* start;
	int len;
	while(*cp != '\0'){
		while(*cp == ' ' || *cp == '\t') //skip leading spaces
			cp++;
		start = cp; //start of the word
		len = 1;
		//find the end of the word
		while(*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
			len++;
		strncpy(arglist[argnum], start, len);
		arglist[argnum][len] = '\0';
		argnum++;
	}
	arglist[argnum] = NULL;
	return arglist;
}
char* read_cmd(char* prompt, FILE* fp)
{
	printf("%s", prompt);
	int c; //input character
	int pos = 0; //position of character in cmdline
	char* cmdline = (char*) malloc(sizeof(char)*MAX_LEN);
	while((c = getc(fp)) != EOF){
		if(c == '\n')
		break;
		cmdline[pos++] = c;
	}
	if(c == EOF && pos == 0)
		return NULL;
	cmdline[pos] = '\0';
	return cmdline;	
}
int isInternal(char* arglist[]){
		
	if((strcmp(arglist[0],"cd") && strcmp(arglist[0],"exit") && strcmp(arglist[0],"pwd") && strcmp(arglist[0],"help")) == 0){
		return 1;
	}
	return 0;	
}
void executeInternal(char* arglist[], char* prompt){
	int i=0;
	if(strcmp(arglist[i],"exit") == 0)
	{	
		exit(1);
	}
	if(strcmp(arglist[i],"cd") == 0)
	{		
		chdir(arglist[i+1]);		
	}
	if(strcmp(arglist[0],"pwd") == 0)
	{
		char cwd[1024];
   		if (getcwd(cwd, sizeof(cwd)) != NULL)
		printf("%s\n",cwd);
	}	
}
