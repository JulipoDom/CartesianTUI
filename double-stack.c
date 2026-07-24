#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// implementação generica de stack de double

typedef struct DoubleStack {
  double *items;
  int capacity;
  int top;
} DoubleStack;

void initDoubleStack(DoubleStack *stack, int size) {
  stack->capacity = size;
  stack->items = (double *)malloc(size * sizeof(double));
  stack->top = -1;
}

bool pushDoubleStack(DoubleStack *stack, double val) {
  if (stack->top == stack->capacity - 1) {
    int newCapacity = stack->capacity * 2;
    double *newItems =
        (double *)realloc(stack->items, newCapacity * sizeof(double));
    if (!newItems) {
      printf("Failed to grow stack array memory!\n");
      return false;
    }
    stack->items = newItems;
    stack->capacity = newCapacity;
  }

  stack->top++;
  stack->items[stack->top] = val;
  return true;
}

double popDoubleStack(DoubleStack *stack) {
  if (stack->top == -1) {
    printf("Stack Underflow!\n");
    return NAN;
  }

  double poppedData = stack->items[stack->top];
  stack->items[stack->top] = NAN;
  stack->top--;
  return poppedData;
}

void freeDoubleStack(DoubleStack *stack) {
  if (stack == NULL)
    return;
  if (stack->items != NULL) {
    free(stack->items);
  }
  stack->items = NULL;
  stack->capacity = 0;
  stack->top = -1;
}
