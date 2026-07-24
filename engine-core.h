#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <stdbool.h>

char **inputReader(const char *expression);
void liberarTokens(char **tokens);
char **shuntingYard(char **tokens);
double evalRPN(char **rpnTokens, double x, double y);
#endif
