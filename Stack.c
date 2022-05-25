#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For string copy
// Lib required for sleep: 
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "mlock.h"
#include <pthread.h>

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

int inter_pop(struct Stack *stack, int free_memory) // Pop without freeing memory
{
	if (!stack)
		return 0;
	// Pop here
	if (!stack->value) // If stack is empty
	{	
		return 0;
	}
	if (free_memory)
		mem_free(stack->value);
	stack->value = NULL;
	if (stack->next)
	{
		struct Stack *ptr = stack->next;
		stack->value = ptr->value;
		stack->next = ptr->next;
		if (free_memory)
			mem_free(ptr); // We are removing the next struct duplicate
	}
	return 1;
}

int pop(struct Stack *stack) // Unlike traditional high-level language, here pop is not returning the value as it needs to free it from the memory.
{	
	return inter_pop(stack, 1);
}

int pop_unmanaged(struct Stack *stack)
{
	return inter_pop(stack, 0);
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
		if (stack->value){
			mem_free(stack->value);
			stack->value = NULL;
		}
		pointer = stack->next;
		mem_free(stack);
		stack = pointer;
	}
}


