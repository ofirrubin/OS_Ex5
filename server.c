#include <stdio.h>
#include <signal.h> // Override signal
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h> // Socket close
#include "TCPServer.h"
#include "Stack.h"
#include "stackShellLib.h"
#include "mlock.h"

#define BACKLOG 10     // how many pending connections queue will hold
#define MAX 1032
#define MAXVAL 1024

//
//
//
//
//
//
//
// NEW - SHARED MEMORY

struct Stack *s;
static int server_pid;


char *shared_top;
int *shared_status;

char *user_input;
int *user_input_size;

pthread_mutex_t lock;
struct sigaction *mem_handler;
static struct sigaction *user_action_handler;

static int user_signal = -1;

void on_action_complete(int signum)
{
	if (signum == SIGUSR1)
		user_signal = 0; // Sucess
	else if (signum == SIGUSR2)
		user_signal = 1; // Fail
	else
		user_signal = -1; // Unknown
}

void memory_controller(int signum, siginfo_t *info, void *context)
{
	pthread_mutex_lock(&lock); // Locking the mutex
	int action_status = 0;
	int req_pid = info->si_pid; // PID of process requested action
	if (req_pid == getpid()) // Ignore calls from yourself if any
		return;
		
	if (signum == SIGUSR1)
	{
		if (user_input[0]) // Push if not empty.
			action_status = push_copy(s, user_input, *user_input_size); 
	}
	else if (signum == SIGUSR2)
	{
		action_status = pop(s);
	}
	if (action_status) // If stack actually changed
	{
		*shared_status = is_empty(s);
		if (! *shared_status)
			strcpy(shared_top, top(s));
	}
	if (action_status)
		kill(req_pid, SIGUSR1);
	else
		kill(req_pid, SIGUSR2);	
	pthread_mutex_unlock(&lock); // Locking the mutex
}


int create_mem_handlers()
{
	mem_handler = (struct sigaction *)mem_calloc(sizeof(struct sigaction));
	if (!mem_handler)
		return 0;
	mem_handler->sa_flags =  SA_RESTART | SA_SIGINFO;   
	sigemptyset (&mem_handler->sa_mask);
	mem_handler->sa_sigaction = memory_controller;
	if ((sigaction(SIGUSR1, mem_handler, NULL) == -1 ||(sigaction(SIGUSR2, mem_handler, NULL) == -1)))
	{
	 printf("Error: Unable to set signals!\n");
	 return 0;
	}
	//signal(SIGINT, SIG_IGN); // Ignore interruptions

	// For each client created, set this handler so it'll know the result is set.
	user_action_handler = (struct sigaction *)mem_calloc(sizeof(struct sigaction));
	if (!user_action_handler)
		return 0;
	user_action_handler->sa_handler = on_action_complete;
	return 1;
}

int set_global_memory()
{
	server_pid = getpid();
	s = (struct Stack *)create_stack();

	// Server to client relation
	shared_top = (char *)mem_calloc(MAXVAL);
	shared_status = (int *)mem_calloc(sizeof(int));

	// Client to server relation
	user_input = (char *)mem_calloc(MAXVAL);
	user_input_size = (int *)mem_calloc(sizeof(int));

	if (!s || !shared_top || !shared_status || !user_input || !user_input_size)
	{
		perror("ERROR: Unable to allocate memory!\n");
		return 0;
	}
	*shared_status = 1; // Stack is set to be empty by default.
	return 1;
}

int set_global_stack_memory()
{
	printf("DEBUG: Setting Signals handler\n");
	if (!create_mem_handlers())
		return 0;

	printf("DEBUG: Allocating & Setting memory....\n");
	// Stack - Although it is managed to be shared memory, I can choose to make all non-top variables as private.
	if (!set_global_memory())
		return 0;

	printf("DEBUG: Creating shared mutex...\n");
	if (!init_mutex(&lock)){
		printf("Failed to create process safe mutex!\n");
		return 0;
	} // Initate serverwide mutex
    	return 1;
}

int client_push(char *ptr, int size)
{
	pthread_mutex_lock(&lock); // Locking the mutex
	// Pass data to server
	memset(user_input, 0, MAXVAL); // ZERO-TRUST
	strcpy(user_input, ptr);
	*user_input_size = size;
	kill(server_pid, SIGUSR1); // REQUEST PARENT TO PUSH
	pause(); // Wait to receive a signal
	pthread_mutex_unlock(&lock); // Locking the mutex
	return 1;
}

int client_pop(struct Stack *stack)
{
	//set_handlers();
	pthread_mutex_lock(&lock); // Locking the mutex
	kill(server_pid, SIGUSR2);
	pause();
	pthread_mutex_unlock(&lock); // Locking the mutex
	return 1;
}

///
///
///
///
/// MAIN


void *client_handler(void *args)
{
	int sockfd = *(int *)args;
	if (sigaction(SIGUSR1, user_action_handler, NULL))
	{
		printf("Failed to set signal handler!\n");
		close(sockfd);
		return 0;
		
	}
        signal(SIGINT, SIG_IGN);
	// Input variables        
        char cmd[MAX + 1];
        int size;
	
	// Output variables
	int write_size;
	char buffer[MAX]; // Output to user
	
	int fb = 1; // feedback
	do
	{       // Reset input & output buffers
		memset(cmd, 0, MAX); 
		memset(buffer, 0, MAX);
                receive(sockfd, (char **)&cmd, MAX + 1, &size);
                if (size > 0)
                {
			fb = stack_command_handler(&lock, client_push, client_pop, shared_top,
						&user_signal, shared_status, 
						cmd, size, buffer, &write_size);
			buffer[write_size + 1] = 0;  // Make sure it's printable
			if (fb && write_size > 0)
			{
				//printf("DEBUG: Client %d request: |%s| Respond: %s\n", sockfd, cmd, buffer);
				sock_send(buffer, write_size+1, sockfd);
			}
			else if (fb)
			{
				//printf("DEBUG: Sending no respond [Client %d]\n", sockfd);
				sock_send("No respond\n\0", 12, sockfd);
			}
		}
	}
	while(fb);
	printf("DEBUG: Server closing connection to client %d\n", sockfd);
	close(sockfd);
	return 0;
}

int main(void)
{
    if (!set_global_stack_memory())
    	return 0;
    
    struct sigaction *sa = calloc(sizeof(struct sigaction), 1);
    char s[INET_ADDRSTRLEN];
    
    // Create and bind server socket
    int sockfd = create_server(sa, (char **)&s);
    if (sockfd <= 0 )
    {
    	printf("Error creating socket\n");
    }
    // Listen to the server
    server_listen(sockfd, BACKLOG);
    // Reap dead processes
    reap_dead_processes(sa);

    printf("DEBUG: Waiting for connections...\n");
    handle_forever_process(sockfd, (char **)&s, client_handler, mem_handler);
    free(sa);
    return 0;
}

