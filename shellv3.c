#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "

int BGflag;
int isInternal(char* arglist[]);
void executeInternal(char* arglist[], char*);

int IORedirection(char* arglist[]);
int pipes(char* arglist[]);

int SemiColon(char* arglist[]); //new added function in v4
int execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);

int main(){
	BGflag=0;
	char* cmdline;
	char** arglist;
	char* prompt = PROMPT;
	while((cmdline = read_cmd(prompt,stdin)) != NULL){
		if((arglist = tokenize(cmdline)) != NULL){
			BGflag=0; // check for &
			int flg = SemiColon(arglist);
			if(flg ==0)
			{
				int rv = isInternal(arglist);
				if(rv == 1){ //internal command
					executeInternal(arglist, prompt);
				}
				else{
					execute(arglist);
				}
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

	int rv = IORedirection(arglist);
	if(rv==0)
	{
		return 0;
	}
	int rv2 = pipes(arglist);
	if(rv2==0)
	{
		return 0;
	}
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
		if(BGflag ==0)
		{		
			waitpid(cpid, &status, 0);	
		}
		return 0;
	}
}
int IORedirection(char* arglist[]) 
{
	int i=0;
	for(i=0;arglist[i]!=NULL;i++)
	{
		if(strcmp(arglist[i],"<")==0 || strcmp(arglist[i],"0<")==0)
		{		
			int saved_stdin = dup(0);
			int saved_stdout = dup(1);
			close(0);
			int fd= open(arglist[i+1],O_RDONLY);	
			for(i=0;arglist[i]!=NULL;i++)
			{
				if(strcmp(arglist[i],">")==0 || strcmp(arglist[i],"1>")==0)
				{
					close(1);
					int fd= open(arglist[i+1],O_WRONLY,O_CREAT,O_TRUNC);
				}
			}
			if(fork()==0)
			{
				printf("here");
				execlp(arglist[0],"cmd",NULL);
			}
			else
			{
				wait(NULL);		
				dup2(saved_stdin,0);
				dup2(saved_stdout,1);
				close(saved_stdout);
				close(saved_stdin);
				return 0;
			}		
		}
		else if(strcmp(arglist[i],">")==0 || strcmp(arglist[i],"1>")==0)
		{
			int saved_stdin = dup(0);
			int saved_stdout = dup(1);
			close(1);
			int fd= open(arglist[i+1],O_WRONLY);
			for(i=0;arglist[i]!=NULL;i++)
			{
				if(strcmp(arglist[i],"<")==0 || strcmp(arglist[i],"0<")==0)
				{
					close(0);
					int fd= open(arglist[i+1],O_RDONLY);
				}
			}
			if(fork()==0)
			{	
				execlp(arglist[0],"cmd",NULL);		
			}
			else
			{
				wait(NULL);				
				dup2(saved_stdin,0);
				dup2(saved_stdout,1);
				close(saved_stdout);
				close(saved_stdin);
				return 0;
			}		
		}
	}
	return 1;
}
int pipes(char* args[])
{	
	int filedes[2]; 
	int filedes2[2];
	int num_cmds = 0;
	char *command[256];
	pid_t pid;
	int err = -1;
	int end = 0;
	int i = 0, j = 0, k = 0, l = 0;
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	if(num_cmds ==1)//no pipe used
	{
		return 1;
	}
	while (args[j] != NULL && end != 1){
		k = 0;

		while (strcmp(args[j],"|") != 0){ // command found
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				end = 1;
				k++;
				break;
			}
			k++;
		}
		command[k] = NULL;
		j++;
		if (i % 2 != 0){
			pipe(filedes); 
		}else{		
			pipe(filedes2);
		}
		pid=fork();
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return 0;
		}
		if(pid==0){
			// If we are in the first command
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}

			}else{ // for odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ // for even i
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}	
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}	
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}
		else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}
			else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}		
		waitpid(pid,NULL,0);			
		i++;	
	}
	return 0;
}
int SemiColon(char* arglist[])
{	
	int i=0;int k=0;int counter=0;
	int flag = 0;
	for(i=0;arglist[i]!=NULL;i++)
	{	
		if(strcmp(arglist[i],";")==0)
		{
			char** semiArglist = (char**)malloc(sizeof(char*)* (MAXARGS+1));
			for(int j=0; j < MAXARGS+1; j++){
				semiArglist[j] = (char*)malloc(sizeof(char)* ARGLEN);
				bzero(semiArglist[j], ARGLEN);
			}
			k=0;	
			for(k=0;counter<i;counter++,k++)
			{
				strcpy(semiArglist[k],arglist[counter]);	
			}
			counter++;
			semiArglist[k]=NULL;
			execute(semiArglist);
			flag=1;
		}
	}
	if(strcmp(arglist[i-1],"&")==0)
	{
		arglist[i-1]=NULL;
		BGflag=1;
	}
	return flag;
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
//these two lines are added incase user press ctrl+d to exit the shell
	if(c == EOF && pos == 0)
		return NULL;
	cmdline[pos] = '\0';
	return cmdline;	
}
//whether the command is intenal or not
int isInternal(char* arglist[])
{		
	if((strcmp(arglist[0],"cd") && strcmp(arglist[0],"exit") && strcmp(arglist[0],"pwd") && strcmp(arglist[0],"help") && strcmp(arglist[0], "kill") && strcmp(arglist[0], "jobs")) == 0){
		//printf("It is Internal Command\n");
		return 1;
	}
	return 0;
}
void executeInternal(char* arglist[], char* prompt)
{
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
	if(strcmp(arglist[0], "help") == 0)
	{
		printf("cd\nkill\njobs\nexit\nhelp\n");		
	}
	if(strcmp(arglist[0], "kill") == 0)
	{
		int signum;
		if(arglist[1][2] == '\0')
			signum = arglist[1][1] - '0';
		else
			signum = (arglist[1][1] - '0') * 10 + (arglist[1][2] - '0');
		kill(atoi(arglist[2]), signum);
		
	}
	if(strcmp(arglist[0], "jobs") == 0)
	{
		system("jobs");
	}
}
