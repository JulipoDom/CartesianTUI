#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// implementação generica de stack de strings

typedef struct Stack {
  int top;
  int capacity;
  char **items;
} StringStack;

void initStack(StringStack *stack, int size) {
  stack->top = -1;
  stack->capacity = size;
  stack->items = (char **)malloc(size * sizeof(char *));
}

bool isEmpty(StringStack *stack) { return stack->top == -1; }

bool pushStack(StringStack *stack, char *str) {
  if (stack->top == stack->capacity - 1) {
    int newCapacity = stack->capacity * 2;
    char **newItems =
        (char **)realloc(stack->items, newCapacity * sizeof(char *));
    if (!newItems) {
      printf("Failed to grow stack array memory!\n");
      return false;
    }
    stack->items = newItems;
    stack->capacity = newCapacity;
  }

  stack->top++;
  stack->items[stack->top] = strdup(str);
  if (!stack->items[stack->top]) {
    printf("Failed to allocate memory for string!\n");
    stack->top--; // Rollback index on failure
    return false;
  }

  return true;
}

char *popStack(StringStack *stack) {
  if (isEmpty(stack)) {
    printf("Stack Underflow!\n");
    return NULL;
  }

  char *poppedData = stack->items[stack->top];
  stack->items[stack->top] = NULL; // Clear the pointer
  stack->top--;

  return poppedData;
}

char *peek(StringStack *stack) {
  if (isEmpty(stack))
    return NULL;
  return stack->items[stack->top];
}

void freeStack(StringStack *stack) {
  if (stack == NULL)
    return;

  // Libera todas as strings que ainda estão na pilha
  for (int i = 0; i <= stack->top; i++) {
    if (stack->items[i] != NULL) {
      free(stack->items[i]);
    }
  }

  // Libera o array de ponteiros
  if (stack->items != NULL) {
    free(stack->items);
  }

  // Reseta os valores por segurança
  stack->top = -1;
  stack->capacity = 0;
  stack->items = NULL;
}
