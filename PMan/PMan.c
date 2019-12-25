#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>

//node created for linked list of processes
typedef struct node{
	pid_t pid;			//process id
	char* process;		//process name
	char path[512];		//path to process
	int running; 		//0 = off, 1 = on
	struct node* next;	//link to next node
}node;

//reference for linked list head
node* processList = NULL;

//list of accepted commands
char* valid_cmd[] = {
	"bg",
	"bglist",
	"bgkill",
	"bgstop",
	"bgstart",
	"pstat",
};

//find process in linked list
node* findNode(pid_t pid){
	node* p = processList;
	while(p != NULL){
		if(p->pid == pid){
			return p;
		}
		p = p->next;
	}
	return NULL;
}

//add input process to linked list
void addProcess(pid_t pid, char* process, char* cwd){
	node* p = (node*)malloc(sizeof(node));
	p->pid = pid;
	p->process = process;
	strcpy(p->path, cwd);
	p->running = 1;
	p->next = NULL;
	if(processList == NULL){
		processList = p;
	}
	else{
		node* temp = processList;
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = p;
	}
}

//remove process from linked list
void removeProcess(pid_t pid){
	if(findNode(pid) == NULL){
		return;
	}
	node* p = processList;
	node* p2 = NULL;
	while(p != NULL){
		if(p->pid == pid){
			if(p == processList){
				processList = processList->next;
			}
			else{
				p2->next = p->next;
			}
			free(p);
			return;
		}
		p2 = p;
		p = p->next;
	}
}

//add a new process id
void bg(char** input){
	pid_t pid = fork();
	if(pid >= 0){//fork() was a success
		if(pid == 0){//pid is child
			char* cmd = input[1];
			execvp(cmd, &input[1]);
			printf("Error: failed to execute command %s\n", cmd);
			exit(1);
		} 
		else{//pid is parent
			int status;
			int val = waitpid(pid, &status, WNOHANG | WUNTRACED| WCONTINUED);
			if(val == -1){
				printf("Error: waitpid failed\n");
				exit(1);
			}
			char cwd[256];
			getcwd(cwd, sizeof(cwd));
			addProcess(pid, input[1], cwd);
			printf("Background process %d was started.\n", pid);
			node* p = findNode(pid);
			p->running = 1;
			usleep(100);
			sleep(1);
		}
	}
	else{//fork() failed
		printf("Error: failed to fork\n");
	}
}

//shows all running process ids
void bglist(){
	int active = 0;
	int total = 0;
	printf("=== Active Processes ===\n");
	node* p = processList;
	while(p != NULL){
		if(p->running){
			printf("%d:\t %s/%s\n", p->pid, p->path, p->process);
			active++;
		}
		total++;
		p = p->next;
	}
	if(active != total){
		printf("=== Inactive Processes ===\n");
		p = processList;
		while(p != NULL){
			if(!p->running){
				printf("%d:\t %s/%s\n", p->pid, p->path, p->process);
			}
			p = p->next;
		}
	}
	printf("Total background jobs active:\t%d\n", active);
}

//terminates a process id
void bgkill(pid_t pid){
	if(findNode(pid) == NULL){
		printf("Error: invalid pid\n");
		return;
	}
	if(kill(pid, SIGTERM) == 0){
		usleep(100);
	}
	else{
		printf("Error: failed to execute bgkill.\n");
	}
}

//stops a running process id
void bgstop(pid_t pid){
	if(findNode(pid) == NULL){
		printf("Error: invalid pid\n");
		return;
	}
	if(kill(pid, SIGSTOP) == 0){
		usleep(100);
	}
	else{
		printf("Error: failed to execute bgstop.\n");
	}
}

//starts a stopped process id
void bgstart(pid_t pid){
	if(findNode(pid) == NULL){
		printf("Error: invalid pid\n");
		return;
	}
	if(kill(pid, SIGCONT) == 0){
		printf("Background process %d was started.\n", pid);
		node* p = findNode(pid);
		p->running = 1;
		usleep(100);
	}
	else{
		printf("Error: failed to execute bgstart\n");
	}
}

//gives stats and status of a process id
void pstat(pid_t pid){
	if(findNode(pid) == NULL){
		printf("Error: process %d does not exist.\n", pid);
		return;
	}
	char statPath[32];
	char statusPath[32];
	sprintf(statPath, "/proc/%d/stat", pid);
	sprintf(statusPath, "/proc/%d/status", pid);
	FILE* fp = fopen(statPath, "r");
	if(fp != NULL){
		char ret = 0;
		char data[100];
		int a = 1;
		do{
			ret = fscanf(fp, "%s", data);
			if(a == 2){
				printf("comm:\t%s\n", data);
			}
			if(a == 3){
				printf("state:\t%s\n", data);
			}
			if(a == 14){
				float utime = atof(data)/sysconf(_SC_CLK_TCK);
				printf("utime:\t%f\n", utime);
			}
			if(a == 15){
				float stime = atof(data)/sysconf(_SC_CLK_TCK);
				printf("stime:\t%f\n", stime);
			}
			if(a == 24){
				printf("rss:\t%s\n", data);
			}
			a++;
		}while(ret != EOF);
		fclose(fp);
	}
	else{
		printf("Error: could not read stat file.\n");
	}
	fp = NULL;
	fp = fopen(statusPath, "r");
	if(fp != NULL){
		char data[100];
		while(fgets(data, sizeof(data), fp)){
			char* c = strtok(data, "\t");
			if(strcmp(c, "voluntary_ctxt_switches:") == 0){
				printf("voluntary_ctxt_switches:\t%s\n", strtok(strtok(NULL, "\t"), "\n"));
			}
			else if(strcmp(c, "nonvoluntary_ctxt_switches:") == 0){
				printf("nonvoluntary_ctxt_switches:\t%s\n", strtok(strtok(NULL, "\t"), "\n"));
			}
		}
		fclose(fp);
	}
	else{
		printf("Error: could not read status file.\n");
	}
}

void updateProcess(){
	pid_t pid;
	int status;
	while(1){
		pid = waitpid(-1, &status, WCONTINUED | WNOHANG | WUNTRACED);
		if(pid > 0){
			if(WIFSTOPPED(status)){
				printf("Background process %d was stopped.\n", pid);
				node* p = findNode(pid);
				p->running = 0;
			}
			else if(WIFCONTINUED(status)){
				printf("Background process %d was started.\n", pid);
				node* p = findNode(pid);
				p->running = 1;
			}
			else if(WIFSIGNALED(status)){
				printf("Background process %d was killed.\n", pid);
				removeProcess(pid);
			}
			else if(WIFEXITED(status)){
				printf("Background process %d was terminated.\n", pid);
				removeProcess(pid);
			}
		}
		else{
			break;
		}
	}
}

//check if input is valid
int validInput(char* input){
	if(input == NULL){
		return 0;
	}
	int a;
	for(a = 0; a < strlen(input); a++){
		if(!isdigit(input[a])){
			return 0;
		}
	}
	return 1;
}

//executes command to do particular task
void executeInput(char** input){
	int cmd = -1;
	int a;
	for(a = 0; a < 6; a++){
		if(!strcmp(input[0], valid_cmd[a])){
			cmd = a;
			break;
		}
	}
	switch(cmd){
		case 0:{//bg <command>
			if(input[1] != NULL){
				bg(input);
				break;
			}
			printf("Error: invalid command to background\n");
			return;
			
		}
		case 1:{//bglist
			bglist();
			break;
		}
		case 2:{//bgkill <process id>
			if(validInput(input[1])){
				pid_t pid = atoi(input[1]);
				if(pid != 0){
					bgkill(pid);
				}
				break;
			}
			printf("Error: invalid pid\n");
			return;
		}
		case 3:{//bgstop <process id>
			if(validInput(input[1])){
				pid_t pid = atoi(input[1]);
				bgstop(pid);
				break;
			}
			printf("Error: invalid pid\n");
			return;
		}
		case 4:{//bgstart <process id>
			if(validInput(input[1])){
				pid_t pid = atoi(input[1]);
				bgstart(pid);
				break;
			}
			printf("Error: invalid pid\n");
			return;
		}
		case 5:{//pstat <process id>
			if(input[1] != NULL){
				pid_t pid = atoi(input[1]);
				pstat(pid);
				break;
			}
			printf("Error: invalid pid\n");
			return;
		}
		default:{
			printf("PMan: > %s:\tcommand not found\n", input[0]);
			break;
		}
	}
}

//prompt a user for input
int getInput(char** input){
	char* read = readline("PMan: > ");
	if(!strcmp(read, "exit")){
		return -1;
	}
	if(!strcmp(read, "")){
		return 0;
	}
	char* token = strtok(read, " ");
	int a;
	for(a = 0; a < sizeof(read)/sizeof(read[0]); a++){
		input[a] = token;
		token = strtok(NULL, " ");
		if(!token){
			break;
		}
	}
	return 1;
}

//main function to control program
int main(){
	while(1){
		char* input[64];
		int success = getInput(input);
		updateProcess();
		if(success){
			executeInput(input);
		}
		updateProcess();
	}
	return 0;
}