#include <stdio.h>
#include "parser.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int global_index = 0;
int global_bpid[50];

int *pipeArray[20];
int pipeArrayIndex = 0;


void background();
void pipeMain(input *myin);

/*----------helpers----------------*/

int quitChild() {
        int i,status;
	int limit = global_index;
	
	int stat,cno;
	
	i=0;
	while(i<limit) {
	      cno=wait(&stat);
	      printf("[%d] retval: %d\n",cno,WEXITSTATUS(status));
	      i++;
        }
        exit(0);        
}

void addToGlobal(int pid) {
        global_bpid[global_index]=pid;
	global_index++;
}

void printGlobal() {
        int i=0;
        for(;i<global_index;i++)
                printf("[%d]\n",global_bpid[i]);
}

void deleteFromGlobal(int cno) {
	int i,j;
	for(i=0;i<global_index;i++) {
		if(global_bpid[i]==cno) {
			for(j=i;j<global_index;j++) {
				global_bpid[j] = global_bpid[j+1];
			}
			break;	
		}
	}
}

/*-----signal handlers-----*/
void foreground() {
        signal(SIGCHLD,background);
}

void background() {
        //signal(SIGCHLD,foreground);
	int stat,cno;
	cno=wait(&stat);
	printf("[%d] retval: 0\n",cno);
	deleteFromGlobal(cno);
	global_index--;
}

/*------foreground command executions -----*/
void execute_single_command(char *command , char **arguments) {

	int childStat,status;       
        int pid = fork(); 
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		execvp(command,arguments);
		exit(0); 
	}
	waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);	
}

void just_output(char *command, char **arguments, char *output) {        
        int childStat;       
        int pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		FILE *output_fp;
		output_fp = fopen(output,"w");
		int fd_new = fileno(output_fp);
		dup2(fd_new,fileno(stdout));
		fclose(output_fp);
		execvp(command,arguments);
		close(fd_new);
	}
	waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

void just_input(char *command, char **arguments, char *input) {
        int childStat;
        int pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		FILE *input_fp;
		input_fp = fopen(input,"r");
		if(input_fp) {       
		        int fd_new = fileno(input_fp);
		        dup2(fd_new,fileno(stdin));
		        fclose(input_fp);
		        execvp(command,arguments);      
		        close(fd_new);
		}
		else {
			printf("%s not found\n", input);
			exit(0);
		}
	}
	waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

void input_output(char *command, char **arguments, char *input, char *output) {
        int childStat;        
        int pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		FILE *input_fp, *output_fp;
		input_fp = fopen(input,"r");
		if(input_fp) {
		        output_fp = fopen(output,"w");
		        int fd_newi = fileno(input_fp);
		        int fd_newo = fileno(output_fp);
		        dup2(fd_newi,fileno(stdin));
	                dup2(fd_newo,fileno(stdout));
		        fclose(input_fp);
		        fclose(output_fp);
		        execvp(command,arguments);
		        close(fd_newi);
                        close(fd_newo);
		}
		else {
			printf("%s not found\n", input);
			exit(0);
		} 
	}
	waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

/*------------------------------background command execution---------------------------------*/
void back_execute_single(char *command , char **arguments) {
	int childStat;
        int pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		execvp(command,arguments);
		exit(0); // in case of an error in execvp, the child process will be killed.
	}
	addToGlobal(pid);
	signal(SIGCHLD,background);
}

void back_execute_output(char *command , char **arguments, char *output) {
        int childStat; 
        int pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());	
		FILE *output_fp;		
		output_fp = fopen(output,"w");		
		int fd_new = fileno(output_fp);		
		dup2(fd_new,fileno(stdout));		
		fclose(output_fp);		
		execvp(command,arguments);		
		close(fd_new);
	}	
	addToGlobal(pid);
	signal(SIGCHLD,background);
}

void back_execute_input(char *command, char **arguments, char *input) {
        int childStat;
        int pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		FILE *input_fp;		
		input_fp = fopen(input,"r");
		if(input_fp) {		        
		        int fd_new = fileno(input_fp);		
		        dup2(fd_new,fileno(stdin));		
		        fclose(input_fp);		
		        execvp(command,arguments);		        
		        close(fd_new);		
		}
		else printf("%s not found\n", input);
	}	
	addToGlobal(pid);
	signal(SIGCHLD,background);
}

void back_execute_io(char *command, char **arguments, char *input, char *output) {
        int childStat;                
        int pid = fork();       
	if(!pid) {
		setpgid(getppid(),getpid());
		FILE *input_fp, *output_fp;		
		input_fp = fopen(input,"r");	
		if(input_fp) {
		        output_fp = fopen(output,"w");		
		        int fd_newi = fileno(input_fp);		
		        int fd_newo = fileno(output_fp);		
		        dup2(fd_newi,fileno(stdin));	
	                dup2(fd_newo,fileno(stdout));		
		        fclose(input_fp);		
		        fclose(output_fp);		
		        execvp(command,arguments);		
		        close(fd_newi);
                        close(fd_newo);
		}
		else printf("%s not found\n", input);
	}	
	addToGlobal(pid);
	signal(SIGCHLD,background);
}

void distinguish_single(command *commands) {
       
        if(commands->input == NULL) {
                if(commands->output == NULL) {
                        // there is no i/o redirection
                        execute_single_command(commands->info.com->name,commands->info.com->arguments);
                }
                 // output redirection
                else just_output(commands->info.com->name,commands->info.com->arguments,commands->output);
        }
        else { // there is input redirection.
                if(commands->output == NULL) {
                        // there is just input redirection
                        just_input(commands->info.com->name,commands->info.com->arguments,commands->input);
                }
                // input and output redirection
                else input_output(commands->info.com->name,commands->info.com->arguments,commands->input,commands->output);
        }
}

void background_commands(command *commands) {

        if(commands->input == NULL) {
                if(commands->output == NULL) {
                        // there is no i/o redirection
                        back_execute_single(commands->info.com->name,commands->info.com->arguments);
                }
                 // output redirection
                else back_execute_output(commands->info.com->name,commands->info.com->arguments,commands->output);
        }
        else { // there is input redirection.
                if(commands->output == NULL) {
                        // there is just input redirection
                        back_execute_input(commands->info.com->name,commands->info.com->arguments,commands->input);
                }
                // input and output redirection
                else back_execute_io(commands->info.com->name,commands->info.com->arguments,commands->input,commands->output);
        }
}

/********************SUBSHELL******************************/

void subMain(char *command) {

	int i;
 	input * myin = parse(command);
       
        for(i=0;i!=myin->num_of_commands;i++)
           	expand_arguments_null(myin->commands[i].info.com);
           	
        if(myin->del == '|') {
 		pipeMain(myin);	
	}     
        
	// single command 
       	if(myin->num_of_commands==1 && !myin->background) {
       		signal(SIGCHLD,foreground);
        	distinguish_single(myin->commands);
        }
	// multiple commands
	if(myin->del==';' && myin->background==0) {	
		for(i=0;i!=myin->num_of_commands;i++) {
			signal(SIGCHLD,foreground);
		        distinguish_single((myin->commands)+i);
	        }       
	}
	clear_input(myin);
}

void subshell_1(char *command) {
// just subshell
	int pid,childStatus;
	pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		subMain(command);
		exit(0);	
	}
	else waitpid(pid,&childStatus,0);
	signal(SIGINT, SIG_IGN);
}

void subshell_2(char *command, char *input) {
// subshell + input
	int pid,childStat;
        pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());			
		FILE *input_fp;		
		input_fp = fopen(input,"r");
		if(input_fp) {	        
		        int fd_new = fileno(input_fp);	
		        dup2(fd_new,fileno(stdin));
		        fclose(input_fp);
		        subMain(command);        
		        close(fd_new);       
		        exit(0);
		}
		else { 
			printf("%s not found\n", input);
			exit(0);
		}
	}
	else waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

void subshell_3(char *command, char *output) {
// subshell + output  
        int childStat; 
        int pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		FILE *output_fp;
		output_fp = fopen(output,"w");
		int fd_new = fileno(output_fp);
		dup2(fd_new,fileno(stdout));
		fclose(output_fp);
		subMain(command);
		close(fd_new);
		exit(0);
	}
	else waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

void subshell_4(char *command, char *input, char *output) {
// subshell + input/output
	int childStat;          
        int pid = fork();
	if(!pid) {
		signal(SIGINT, SIG_DFL);
		setpgid(getppid(),getpid());
		FILE *input_fp, *output_fp;
		input_fp = fopen(input,"r");
		if(input_fp) {
		        output_fp = fopen(output,"w");
		        int fd_newi = fileno(input_fp);
		        int fd_newo = fileno(output_fp);
		        dup2(fd_newi,fileno(stdin));
	                dup2(fd_newo,fileno(stdout));
		        fclose(input_fp);
		        fclose(output_fp);
		        subMain(command);
		        close(fd_newi);
                        close(fd_newo);
                        exit(0);
		}
		else printf("%s not found\n", input);
	}
	waitpid(pid,&childStat,0);
	signal(SIGINT, SIG_IGN);
}

void subshell_5(char *command) {
// subshell + background
	int pid,childStatus;
	pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		subMain(command);
		exit(0);	
	}
	else addToGlobal(pid);
}

void subshell_6(char *command,char *input) {
// subshell + background + input
	int childStat;
        int pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		FILE *input_fp;
		input_fp = fopen(input,"r");
		if(input_fp) {
		        int fd_new = fileno(input_fp);
		        dup2(fd_new,fileno(stdin));
		        fclose(input_fp);
		        subMain(command);
		        close(fd_new);
		        exit(0);
		}
		else printf("%s not found\n", input);
	}
	addToGlobal(pid);
}

void subshell_7(char *command, char *output) {
// subshell + background + output
 	int childStat;
        int pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		FILE *output_fp;
		output_fp = fopen(output,"w");
		int fd_new = fileno(output_fp);
		dup2(fd_new,fileno(stdout));
		fclose(output_fp);
		subMain(command);
		close(fd_new);
		exit(0);
	}
	addToGlobal(pid);
}

void subshell_8(char *command, char *input, char *output) {
// subshell + background + input + output
	int pid,childStat;
        pid = fork();
	if(!pid) {
		setpgid(getppid(),getpid());
		FILE *input_fp, *output_fp;
		input_fp = fopen(input,"r");
		if(input_fp) {
		        output_fp = fopen(output,"w");
		        int fd_newi = fileno(input_fp);
		        int fd_newo = fileno(output_fp);
		        dup2(fd_newi,fileno(stdin));
	                dup2(fd_newo,fileno(stdout));
		        fclose(input_fp);
		        fclose(output_fp);
		        subMain(command);
		        close(fd_newi);
                        close(fd_newo);
                        exit(0);
		}
		else printf("%s not found\n", input);
	}
	addToGlobal(pid);
}

void subDecider(char *input, char *output , int isbackground, char* subshell_command) {
       	
       	if(!isbackground) {
       		signal(SIGCHLD,foreground);
		if(!input && !output) {
			subshell_1(subshell_command);			
		}
		// subshell + input
		else if(input && !output) {
			subshell_2(subshell_command,input);	
		}
		// subshell + output
		else if(!input && output) {
			subshell_3(subshell_command,output);	
		}
		// subshell + input/output
		else if(input && output) {
			subshell_4(subshell_command,input,output);	
		}
	}
	else {
	        signal(SIGCHLD,background);
		// subshell + background
		if(!input && !output) {
			subshell_5(subshell_command);	
		}
		// subshell + background + input
		else if(input && !output) {
			subshell_6(subshell_command,input);		
		}
		// subshell + bg + output
		else if(!input && output) {
			subshell_7(subshell_command,output);			
		}
		// subshell + bg + input + output
		else if (input && output) {
			subshell_8(subshell_command,input,output);				
		} 
	}
}


/**************************PIPE********************************/

void output_pipe(char *command , char **arguments , char *output) {
	FILE *output_fp;	
	output_fp = fopen(output,"w");
	int fd_new = fileno(output_fp);	
	dup2(fd_new,fileno(stdout));		
	fclose(output_fp);	
	execvp(command,arguments);	
	close(fd_new);
}
void input_pipe(char *command , char **arguments , char *input) {
	FILE *input_fp;	
	input_fp = fopen(input,"r");
	if(input_fp) {		        
		int fd_new = fileno(input_fp);		
		dup2(fd_new,fileno(stdin));		
		fclose(input_fp);
		execvp(command,arguments);        
		close(fd_new);
		
	}
	else printf("%s not found\n", input);
}

void single_for_pipe(command *commands) {
	signal(SIGCHLD,SIG_DFL);
	 if(commands->input == NULL) {
		if(commands->output == NULL) { 	// there is no i/o redirection
			execvp(commands->info.com->name,commands->info.com->arguments);
		}
		// output redirection
	        else output_pipe(commands->info.com->name,commands->info.com->arguments,commands->output);
		}
	else { // there is input redirection.
		if(commands->output == NULL) {
		// there is just input redirection 
		input_pipe(commands->info.com->name,commands->info.com->arguments,commands->input);
		}
	}	
}

void pipeMain(input *myin) {
	signal(SIGCHLD,SIG_DFL);
	int i,j,pid,chl,k;
	int noc = myin->num_of_commands;
	for(k=0; k<noc-1 ; k++) {
		int * fd = malloc(sizeof(int)*2);
		pipe(fd);
		pipeArray[k] = fd;
	}
	for(i=0 ; i<noc ; i++) {
		pid = fork();
		if(!pid) {
			if(i==0) { // first command
				int j;
				for (j = 0; j<noc-1 ; j++) {
					if (j == i) {
						close(pipeArray[j][0]);
					}
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}	
				}	
				dup2(pipeArray[i][1],fileno(stdout));	
				close(pipeArray[i][1]);	
				single_for_pipe(myin->commands+i);	
			}
			else if(i==noc-1) { //last command						
				int j;
				for (j=0 ; j<noc-1 ; j++) {
					if (j == i-1) {
						close(pipeArray[j][1]);
					}
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}	
				}
				dup2(pipeArray[i-1][0],fileno(stdin));	
				close(pipeArray[i-1][0]);	
				single_for_pipe(myin->commands+i);	
			}
			else {	
				int j;
				for (j = 0; j<noc-1 ; j++) {
					if(j == i)
						close(pipeArray[j][0]);
					else if(j == i-1)
						close(pipeArray[j][1]);
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}
				}						
				dup2(pipeArray[i-1][0],fileno(stdin));						
				dup2(pipeArray[i][1],fileno(stdout));	
				close(pipeArray[i-1][0]);
				close(pipeArray[i][1]);
				single_for_pipe(myin->commands+i);	
			}		
		}
	}
	for (j = 0; j<noc-1 ; j++) { // parent closes all pipes
		close(pipeArray[j][0]);
		close(pipeArray[j][1]);
	}
	while(wait(&chl)>0) ; // parent will wait for all child
	signal(SIGINT, SIG_IGN);
return;

}

void pipeSubMain(input *myin) {
	signal(SIGCHLD,SIG_DFL);
	char *input = myin->commands->input;
	char *output = myin->commands->output;
	int isbackground = myin->background;
	int i,j,pid,chl,k;
	int noc = myin->num_of_commands;
	
	for(k=0; k<noc-1 ; k++) {
		int * fd = malloc(sizeof(int)*2);
		pipe(fd);
		pipeArray[k] = fd;
	}
	for(i=0 ; i<noc ; i++) {
		pid = fork();
		if(!pid) {
			if(i==0) { // first command
				int j;
				for (j = 0; j<noc-1 ; j++) {
					if (j == i) {
						close(pipeArray[j][0]);
					}
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}	
				}	
				dup2(pipeArray[i][1],fileno(stdout));	
				close(pipeArray[i][1]);	
				subDecider(input,output,isbackground,myin->commands[i].info.subshell);
			}
			else if(i==noc-1) { //last command						
				int j;
				for (j=0 ; j<noc-1 ; j++) {
					if (j == i-1) {
						close(pipeArray[j][1]);
					}
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}	
				}
				dup2(pipeArray[i-1][0],fileno(stdin));	
				close(pipeArray[i-1][0]);	
				subDecider(input,output,isbackground,myin->commands[i].info.subshell);	
			}
			else {	
				int j;
				for (j = 0; j<noc-1 ; j++) {
					if(j == i)
						close(pipeArray[j][0]);
					else if(j == i-1)
						close(pipeArray[j][1]);
					else {
						close(pipeArray[j][0]);
						close(pipeArray[j][1]);
					}
				}						
				dup2(pipeArray[i-1][0],fileno(stdin));						
				dup2(pipeArray[i][1],fileno(stdout));	
				close(pipeArray[i-1][0]);
				close(pipeArray[i][1]);
				subDecider(input,output,isbackground,myin->commands[i].info.subshell);		
			}
			exit(0);		
		}
	}
	for (j = 0; j<noc-1 ; j++) { // parent closes all pipes
		close(pipeArray[j][0]);
		close(pipeArray[j][1]);
	}
	while(wait(&chl)>0) ; // parent will wait for all child
return;

}

int main() {
	// INT signal handler
	signal(SIGINT, SIG_IGN);
	char command[513];
	int i = 0;
	while(1) {
	        printf("> ");
	        fflush(stdout);
	        scanf("%c",&command[0]);
	        if (command[0] == '\n') {
	                continue;
	        }
	        for(i=1;command[i-1]!='\n';i++) {
	                scanf("%c", &command[i]); 
	        }	        	        
	        command[i] = '\0';
	        i=0;	
	        if(!strcmp(command,"quit\n")) {
		        signal(SIGCHLD,SIG_DFL);
		        quitChild();
	        }        
	        if(!strcmp(command,"lbp\n")) {
	                printGlobal();
	        }
	        
	        input * myin = parse(command);
	        	      
	      
/******************************************SUBSHELL*******************************************************************************/	        
	        if(myin->commands->type==SUBSHELL) {
	        	if(myin->del == '|') {
	        		pipeSubMain(myin);
	        	}
	        	//signal(SIGCHLD,foreground);
	        	else {
				char *input = myin->commands->input;
	    			char *output = myin->commands->output;
	       			int isbackground = myin->background;
	       			char *subshell_command = myin->commands->info.subshell;	 
				subDecider(input,output,isbackground,subshell_command);
	        	}
	        	clear_input(myin);
			continue;
	        }
/****************************************** SUBSHELL END *******************************************************************************/
                 
               // put a null string at the end of arguments array.
               for(i=0;i!=myin->num_of_commands;i++)
               		expand_arguments_null(myin->commands[i].info.com);
               		
/****************************************** PIPE *******************************************************************************/           
 		if(myin->del == '|') {
 			pipeMain(myin);
		}            

/****************************************** PIPE END *******************************************************************************/    
                                          
                if(myin->num_of_commands==1 && !myin->background) {
                	// SIGCHLD handler for foreground
                	signal(SIGCHLD,foreground);
                        distinguish_single(myin->commands);
                }

	        // multiple commands without background processes.
	        if(myin->del==';' && myin->background==0) {
		        int i;	
		        
		        for(i=0; (i!=myin->num_of_commands); i++) {
		        	signal(SIGCHLD,foreground);
			        distinguish_single((myin->commands)+i);
		        }
	        }
	      
	        if(myin->background) {
	                background_commands(myin->commands);            
	        }     
	       clear_input(myin);
        }
        return 0 ;
}
