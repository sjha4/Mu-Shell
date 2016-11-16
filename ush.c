#include "parse.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/time.h>
static void ignore_signals()
{
	signal(SIGINT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGTERM,SIG_IGN);

}

static void default_signals()
{
	signal(SIGINT,SIG_DFL);
}


static int hashString(char *cmd)
{
	if(strcmp(cmd,"pwd")== 0)
		return 1;
	else if(strcmp(cmd,"where")== 0)
		return 2;
	else if(strcmp(cmd,"setenv")== 0)
		return 3;
	else if(strcmp(cmd,"unsetenv")== 0)
		return 4;
	else if(strcmp(cmd,"nice")== 0)
		return 5;
	else if(strcmp(cmd,"cd")== 0)
		return 6;
	else if(strcmp(cmd,"echo")== 0)
		return 7;
	else if(strcmp(cmd,"logout") == 0)
		return 8;
	else return 0;
}
extern char **environ;
static int allBuiltins(Cmd c, int type)
{
	char **args = c->args;	
	//pwd
	if(type==1)
	{
		char pwd[1000];
		char* cwd = getcwd( pwd, 1000 );
	        printf( "%s\n", cwd );	

	}
	//where
	if(type == 2)
	{
		if(args==NULL) 
		{
			fprintf(stderr, "No agruments passed to where");
			return 0;
		}
		else
		{
						
			if(strcmp(args[1],"pwd") ==0 || strcmp(args[1],"where") == 0|| strcmp(args[1],"setenv") == 0 ||
			strcmp(args[1],"unsetenv")==0|| strcmp(args[1],"nice") == 0||strcmp(args[1],"cd") == 0 ||
			strcmp(args[1],"echo")==0 || strcmp(args[1],"logout") == 0)
			{
				fprintf(stdout, "Shell builtin: %s \n",args[1]);
			}
			char *binpath = (char*)malloc(100*sizeof(char));
			//loop over bin directory first..
			strcat(binpath,"/bin/");
			strcat(binpath,args[1]);
			if(access(binpath, F_OK) != -1 || access(binpath, X_OK) != -1)
			{
				fprintf(stdout,"%s\n",binpath);
			}
			//loop over home directory next..
			char *homepath = (char*)malloc(100*sizeof(char));
			//loop over bin directory first..
			strcat(homepath,"/");
			strcat(homepath,args[1]);
			if(access(homepath, F_OK) != -1 || access(homepath, X_OK) != -1)
			{
				fprintf(stdout,"%s\n",homepath);
			}
			free(binpath);
			free(homepath);
			return 0;
				
		}
		
			
	}
	//setenv
	if(type == 3)
	{
		//char **args = c->args;
		
		if(args[1] == NULL)
		{
			char **printEnv = environ;
			while(*printEnv!=NULL)
			{
				printf("%s\n", *printEnv);
				printEnv++;
			
			}
		return 0;
		}
		else if(args[2] == NULL)
		{
			setenv(args[1],"\0",1);
			return 0;
		}
		else if(args[3] == NULL || strcmp(args[3],"0"))
		{
			
			setenv(args[1],args[2],1);
			return 0;
		}
	return 0;	
	}
	//unsetenv
	if(type==4)
	{
		//char **args = c->args;		
		if(getenv(args[1])!=NULL)
		unsetenv(args[1]);
	}
	//nice
	if(type==5)
	{
		return -1;
	}
	//cd
	if(type == 6)
	{
		//char **args = c->args;		
		char *newDir = (args[1] == NULL?getenv("HOME"):args[1]);
		if(newDir!=NULL) 
		{
			if(chdir(newDir)==-1)
			{
				fprintf(stderr,"%s\n",strerror(errno));
				return -1;
			}
		}
	}
	//echo
	if(type==7)
	{
		//char **args = c->args;
		int i;
		for(i=1;args[i] != NULL;i++)
		{
		printf("%s ",args[i]);
		}
		printf("\n");	
	}
	//logout
	if(type==8) exit(0);
return 0;
	
}

static int runBuiltInCommand(Cmd c){

	int caseInt = hashString(c->args[0]);
	if(caseInt==0) return -1;
	int result = allBuiltins(c,caseInt);
	return result;	
}


static void prPipe(Pipe p)
{
	int stdin_old = dup(0);
	int stdout_old = dup(1);
	int stderr_old = dup(2);
    	Cmd c;

  for( c = p->head; c->next != NULL; c = c->next ) 
  {	
	int pipefd[2];

	if(pipe(pipefd) == -1)
	{
               perror("pipe");
               exit(EXIT_FAILURE);
	}

	

	if(c == p->head && c->in == Tin)
	{
			if(access(c->infile, F_OK )==-1||access(c->infile, R_OK) == -1)
				return;
			FILE* f = fopen(c->infile, "r");
			if(f==NULL) {fprintf(stderr,"No such file or directory\n");return;}
			int fileNum = fileno(f);
			dup2(fileNum,0);
	}

	pid_t pid = fork();
	
	if(pid == -1)
	{
		fprintf(stderr,"Error in fork\n");
		return ;
	}

	if(pid == 0)
	{		

		default_signals();
		dup2(pipefd[1],1);		
		close(pipefd[0]);
		//close(pipefd[1]);
		if(runBuiltInCommand(c) != 0 && (execvp(c->args[0],c->args)==-1))
		{
			fprintf(stderr,"Error in piped child command!\n");
			exit(0);		
		}
		
	}
	
    	dup2(pipefd[0],0);
	close(pipefd[1]);
	close(pipefd[0]);
  } //end of for

	if(c == p->head && c->in == Tin)
	{
			if(access(c->infile, F_OK )==-1||access(c->infile, R_OK) == -1)
				return;
			FILE* f = fopen(c->infile, "r");
			if(f==NULL) {fprintf(stderr,"No such file\n");return;}
 			int f2 = fileno(f);
			dup2(f2,0);
	}	
		
	if(c->out == Tout||c->out == ToutErr)
	{
					
		int valid = 0;	
		int outFileNumber=-99;	
		FILE* fOut = fopen(c->outfile, "w");	
		if(fOut==NULL)
		{
			outFileNumber = creat(c->outfile,0666);
			if (outFileNumber==-1) valid =-1;
			//printf("here NULL");
		}
		if(valid ==-1) 
		{
			//printf("here NULL1");
			fprintf(stderr,"%s\n",strerror(errno));			
			exit(0);
		}
		if(outFileNumber == -99)		
		outFileNumber = fileno(fOut);
		if(c->out == ToutErr) dup2(outFileNumber,2);
		dup2(outFileNumber,1);
		close(outFileNumber);
	}
	else if(c->out == Tapp || c->out == TappErr)
	{
		//printf("Here");		
		int valid = 0;
		int outFileNumber = -99;		
		FILE* fOut = fopen(c->outfile, "a");	
		if(fOut==NULL)
		{
			outFileNumber = creat(c->outfile,0666);
			if (outFileNumber==-1) valid =-1;
		}
		if(valid ==-1) 
		{
			fprintf(stderr,"%s\n",strerror(errno));			
			exit(0);
		}
		if(fOut==NULL) return;
		if(outFileNumber == -99)
		outFileNumber = fileno(fOut);
		if(c->out == TappErr) dup2(outFileNumber,2);
		dup2(outFileNumber,1);
		close(outFileNumber);
	
	}


	if(runBuiltInCommand(c) != 0)
	{			
		//printf("In last command");		
		pid_t lastPid = fork();
		if(lastPid == 0){	

			default_signals();	
			if (execvp(c->args[0],c->args) == -1) { fprintf(stderr,"%s\n",strerror(errno)); exit(0); }//printf("In last command");}
			return;
		}
	}
	while (wait(NULL) > 0);	
	dup2(stdin_old,0);
	dup2(stdout_old,1);
	dup2(stderr_old,2);
	close(stdin_old);	
	close(stdout_old);
	close(stderr_old);
	
}

static void myShell()
{
	char host[256];
	gethostname(host, 255);	
	Pipe p;
	while ( 1 )
	{
		printf("%s%% ", host);	
		fflush(stdout);
	    	p = parse();
		while(p != NULL)
		{
			prPipe(p);
			p= p->next;
		}  
    		freePipe(p);	
	}
	
}


int main(int argc, char *argv[])
{
	char* ushrcFile = (char*)malloc(255*sizeof(char));
	strcpy(ushrcFile, getenv("HOME"));

	strcat(ushrcFile, "/.ushrc");
	//printf("%s\n", ushrcFile);
	if(access(ushrcFile, R_OK ) == -1)
	{
		//printf("Here");			
		myShell();
	}	
	else 
	{
		//printf("Here1");	
		//myShell();	
		Pipe setup;
	    	fflush(stdin);
	    	int stdin_old = dup(0);
	    	
		if(freopen(ushrcFile, "r", stdin)!=NULL)
		{
			while ( 1 ) 
			{			    	
				setup = parse();
				if(setup == NULL) break;
				while(setup!= NULL)
				{
					//printf("Here in while!");
					prPipe(setup);
					setup= setup->next;
				}  
			   	freePipe(setup);	
		  	}
		}
	    	fflush(stdin);
		fflush(stdout);
	    	dup2(stdin_old, 0);
	    	close(stdin_old);
		myShell();		
	}
				
}
