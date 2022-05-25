#include <stddef.h>
void *mem_alloc(size_t size);
void *mem_calloc(size_t size);
void mem_free(void *ptr);
int init_mutex(pthread_mutex_t *mem_lock);
