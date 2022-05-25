struct Stack;
struct Stack *create_stack(void);
void free_stack(struct Stack *stack);
int push(struct Stack *stack, char *val);
int push_copy(struct Stack *stack, char *val, int size);
int pop(struct Stack *stack);
char *top(struct Stack *stack);
int is_empty(struct Stack *stack);
int has_next(struct Stack *stack);
int pop_unmanaged(struct Stack *stack); // Pop without freeing memory
