#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#define size_len 8 // sizeof size_t is 8 bytes

void* mem_alloc(size_t size) {
    void *mem = mmap(NULL, size + size_len,
     PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
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
