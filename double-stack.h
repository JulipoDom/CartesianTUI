#ifndef DOUBLE_STACK_H
#define DOUBLE_STACK_H

#include <stdbool.h>

typedef struct DoubleStack {
  double *items;
  int capacity;
  int top;
} DoubleStack;

void initDoubleStack(DoubleStack *stack, int size);
bool pushDoubleStack(DoubleStack *stack, double val);
double popDoubleStack(DoubleStack *stack);
void freeDoubleStack(DoubleStack *stack);

#endif
