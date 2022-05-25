#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "Stack.h"
#include "stackShellLib.h"
#include "mlock.h"
#include <string.h>


#ifndef MAX_SIZE
#define MAX_SIZE 1032
#endif

// Recycled from Shell assignment, minor changes (such as length)
void get_command(char **cmd, int *cmd_len)
{
	char *c = mem_calloc(MAX_SIZE);
	if (!c){
		printf("ERROR: Error allocating memory for command input\n");
		exit(0);
	}
	// Get input from the user
	if (!fgets(c, MAX_SIZE, stdin))
	{
		mem_free(c);
	}
	*cmd_len = strlen(c);
	if (*cmd_len > 0)
	{ // Remove trailling (new line etc)
		while ((char *)cmd_len > (char *)c && c[*cmd_len] == 0)
			*cmd_len = *cmd_len - 1;
	}
	*(c+*cmd_len) = 0;
	*cmd = c;
}


// START Recycling | Recycled from Shell assignment
int cmd_cmp(char *actual, char *expected, int starts_with){ // Assuming <expected> is lowered case.
	int pos = 0;
	int d = 'A' - 'a';
	while (actual[pos] != 0){
		if (expected[pos] == 0 || !(actual[pos] == expected[pos] || expected[pos] + d == actual[pos]))
			return starts_with && expected[pos] == 0;
		pos += 1;
	}
	return expected[pos] == 0 && actual[pos] == 0;
}

int cmd_eq(char *cmd, char *cmp){ // Command equals to
	return cmd_cmp(cmd, cmp, 0);
}

int cmd_sw(char *cmd, char *cmp){ // Command Starts with
	return cmd_cmp(cmd, cmp, 1);
}
// END Recycling | End of recycling from Shell assignment

int stack_command_handler(pthread_mutex_t *lock,
 			  int (* push_req)(char *ptr, int size), int (* pop_req)(), char *stack_top,
			  int *action_status, int *stack_empty,
 			  char *cmd, int size, // Orginal command
 			  char output[MAX_SIZE], int *write_size // Output
 			  )
{	*action_status = -1; // Per client variable thus we can change it safely.
	if (cmd_eq(cmd, "exit"))
	{
		*write_size = 0;
		return 0;
	}
	if (cmd_sw(cmd, "push"))
	{
		int offset = sizeof "push";
		int arg_size = size - offset;
		cmd = cmd + offset;
		if (arg_size > 0)
		{
			// Copy data to server user_input, signal the main-process to push and wait for confirmation signal.
			push_req(cmd, arg_size);
			
			if (*action_status)
				*write_size = sprintf(output, "OUTPUT: Push failed\n");
			else
				*write_size = sprintf(output, "OUTPUT: Pushed\n");
				
		}
		else
		{
			printf("DEBUG: Push size is lower than 0\n");
			*write_size = sprintf(output, "OUTPUT: Can't be empty\n");
		}
	}
	else if (cmd_eq(cmd, "pop"))
	{
		if (! *stack_empty)
			pop_req(); // request pop form main-process and wait for confirmation signal.
		if (*action_status == 0)
			*write_size = sprintf(output, "OUTPUT: Popped\n");
		else
			*write_size = sprintf(output, "OUTPUT: Failed / Empty\n");
	
	}
	else if (cmd_eq(cmd, "top"))
	{
		pthread_mutex_lock(lock);
		if (!*stack_empty) // To prevent trying to print NULL
			*write_size = sprintf(output, "OUTPUT: %s\n", stack_top);
		else
			*write_size = sprintf(output, "OUTPUT: Stack is empty\n");
		pthread_mutex_unlock(lock);
	}
	return 1;
}

