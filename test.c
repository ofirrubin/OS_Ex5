#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
 #include <sys/prctl.h>
 #include <sys/signal.h>


#define SER "./server > trash.txt"
#define CL "./client 127.0.0.1 < test_input.txt > trash.txt"
#define ASSERT "./client 127.0.0.1 < assert.txt"
#define N_CL 2
#define MAX_PUSH 20
#define MIN_PUSH 1

int complete = 0;


void deploy_clients()
{
	for(int i=0; i < N_CL; i++)
	{
		int pid;
		if ((pid = fork()) < 0)
		{
			printf("ERROR %d\n", pid);
			perror("Error deploying a client...\n");
			exit(-1);
		}
		else if (pid != 0)
		{
			system(CL); // System call to client 127.0.0.1 (this process is now the client process)
			return;
		}
	}
	printf("The server suppose to be empty now...\n");
	printf("The server will now create a client to verify and will output the stack output,\n");
	printf("Once you get the result, terminate the server by CTRL + C or continue using it.\n");
	system(ASSERT);
	exit(0);
}

int main()
{
	printf("Stack Server & Client Tester\n");
	printf("The test divides into a server and clients;\nMake sure no server is running, the test will create the server & clients\n");
	int _p;
	if ((_p = fork()) < 0)
	{
		printf("Error: main fork() \n");
		exit(-1);
	}
	if (_p != 0)
	{
		if ((_p = fork()) < 0)
		{
		printf("Error: main fork() \n");
		exit(-1);
		}
		else if (_p == 0)
		{
			sleep(3);
			printf("Deploying %d clients\n", N_CL);
			deploy_clients();
		}
		else
		{		
			prctl(PR_SET_PDEATHSIG, SIGHUP);
			printf("Creating server..\nDeploying clients in 3 seconds...\n");
			system(SER);
		}
	}
}
