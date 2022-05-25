#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include<sys/wait.h>

#include <signal.h>

#define size_len 8 // sizeof size_t is 8 bytes


static pthread_mutex_t lock;
static struct Stack *s = NULL;

void* mem_alloc(size_t size) {
    void *mem = mmap(NULL, size + size_len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == NULL)
    	return NULL;
    size_t *size_ptr = mem;
    *size_ptr = size; // Set first [sizeof size] bytes to size.
    return (void *)mem + size_len; // Give the user the memroy afterwards
}


void *mem_calloc(size_t size)
{
	void *ptr = mem_alloc(size);
	if (ptr != NULL)
		memset(ptr, 0, size);
	return ptr;
}


void mem_free(void *ptr)
{
	if (ptr == NULL)
		return;
	ptr = ptr - size_len; // Go to the beginning of the memory
	size_t s = *(size_t *)ptr; // Get the size of the memory block
	if (munmap(ptr, s + size_len))
		perror("Error freeing memory");
}

int init_mutex(pthread_mutex_t *mem_lock)
{
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) || pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
	{
		perror("Error initiating mutex shared processed attr");
		return 0;
	}
	if (pthread_mutex_init(mem_lock, &attr) != 0) {
		perror("\n *memory mutex init has failed*\n");
		return 0;
	}	
	return 1;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For string copy
// Lib required for sleep: 
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


struct Stack
{
	char *value;
	struct Stack *next;	
};

struct Stack *create_stack(void)
{
	struct Stack *s = NULL;
	s = mem_calloc(sizeof(struct Stack));
	if (!s)
		return NULL;
	s->value = NULL;
	s->next = NULL;
	return s;
}	

// Essentialy - read command is free to use but write commands are not thread safe by nature
int push(struct Stack *stack, char *val)
{
	if (!stack)
	{
		return 0;
	}
	// Push here
	if (! stack->value) // Empty stack
	{
		stack->value = val;
		return 1;
	}
	struct Stack *n = create_stack();
	if (!n) // If unable to allocate stack new element
	{
		return 0;
	}
	n->value = stack->value;
	n->next = stack->next;
	
	stack->next = n;
	stack->value = val;
	
	return 1;
}

int push_copy(struct Stack *stack, char *val, int size)
{
	char *tmp = mem_calloc(size + 1);
	if (!tmp)
		return 0;
	strncpy(tmp, val, size);
	return push(stack, tmp);
}


int pop(struct Stack *stack) // Unlike traditional high-level language, here pop is not returning the value as it needs to free it from the memory.
{	
	if (!stack)
		return 0;

	// Pop here
	if (!stack->value) // If stack is empty
	{	
		return 0;
	}
	mem_free(stack->value);
	stack->value = NULL;	
	if (stack->next)
	{
		struct Stack *ptr = stack->next;
		stack->value = ptr->value;
		stack->next = ptr->next;
		mem_free(ptr); // We are removing the next struct duplicate
	}
	return 1;
}

char *top(struct Stack *stack)
{
	return stack->value;
}

int is_empty(struct Stack *stack)
{
	return !(stack && stack->value);
}

int has_next(struct Stack *stack)
{
	return !stack->value;
}


void free_stack(struct Stack *stack)
{
	struct Stack *pointer;
	while(stack)
	{
		if (stack->value)
			mem_free(stack->value);
		pointer = stack->next;
		mem_free(stack);
		stack = pointer;
	}
}

void request_push()
{
	kill(getppid(), SIGUSR1);
}

void request_pop()
{
	kill(getppid(), SIGUSR1);
	pthread_mutex_unlock(&lock);
}

void memory_controller(int signum)
{	// Assuming caller locked memory.
	if (signum == SIGUSR1)
	{
		push_copy(s, "", 1024); 
	}
	else if (signum == SIGUSR2)
	{
		// Free top memory
		pop(s); // Pop and free memory
	}
}


int set_top_val(struct Stack *stack, char *val, int size)
{
	if (is_empty(stack))
		return 0;
	strncpy(stack->value, val, size);
	return 1;
}

int client_push(struct Stack *stack, char *val, int size)
{
	pthread_mutex_lock(&lock); // Locking the mutex
	request_push(); 
	set_top_val(s, val, size);
	pthread_mutex_unlock(&lock); // Unlocking the mutex
}

int client_pop(struct Stack *stack)
{
	pthread_mutex_lock(&lock); // Locking the mutex
	if (is_empty(stack))
		return -1; // Empty
	char *old = *stack->val;
	request_pop();
	char *pnew = *stack->val;
	pthread_mutex_unlock(&lock); // Unlocking the mutex
	return old == pnew;
}

int main()
{

	struct sigaction mem_handler;
	printf("Server PID %d\n", getpid());

	mem_handler.sa_handler = memory_controller;
	if (sigaction(SIGUSR1, &mem_handler, NULL) == -1 ||(sigaction(SIGUSR2, &mem_handler, NULL) == -1))
	{
	printf("Error: Unable to set signals!\n");
	return 0;
	}
	
	if (!init_mutex(&lock))
		printf("Failed to create mutex\n");
	s = create_stack();
	int p = fork();
	int st;
	if (p == 0)
	{	
		char *tmp = mem_calloc(6);
		strcpy(tmp, "Hello");
		printf("F Requesting push...\n");
		
		printf("F Pushed... Checking...\n");
		//mpush_copy(s, tmp, 6, &m);
		printf("Child Top = %s\n", top(s));
		printf("F is entering 5sec sleep\n");
		sleep(5);
		printf("F is out of sleep\n");
		printf("Child Top = %s\n", top(s));
		
		printf("N is entering 3sec sleep\n");
		request_pop();
		sleep(10);
		printf("Top: %s\n", top(s));
		
	}
	waitpid(p, &st, 0);
	return 0;
}
