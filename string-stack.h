#ifndef STRING_STACK_H
#define STRING_STACK_H

#include <stdbool.h>

// Definição da estrutura
typedef struct Stack {
  int top;
  int capacity;
  char **items;
} StringStack;

// Assinaturas das funções do módulo
void initStack(StringStack *stack, int size);
bool isEmpty(StringStack *stack);
bool pushStack(StringStack *stack, char *str);
char *popStack(StringStack *stack);
char *peek(StringStack *stack);
void freestack(StringStack *stack);

#endif // STRING_STACK_H
