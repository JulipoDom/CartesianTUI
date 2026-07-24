#include "engine-core.h"
#include "double-stack.h"
#include "string-stack.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dicionário de funções e constantes suportadas
const char *palavras_conhecidas[] = {
    "sin",    "arcsin", "sinh",   "cos",  "arccos", "cosh",   "tan",  "arctan",
    "tanh",   "cot",    "arccot", "coth", "sec",    "arcsec", "sech", "csc",
    "arccsc", "csch",   "log",    "ln",   "sqrt",   "pi",     "e"};
const int num_palavras = 23;

// Verifica se o texto começa com uma palavra do dicionário.
// Retorna o tamanho da palavra encontrada ou 0.
int tamanhoPalavraConhecida(const char *texto) {
  int max_tam = 0;
  for (int i = 0; i < num_palavras; i++) {
    int tam = strlen(palavras_conhecidas[i]);
    if (strncmp(texto, palavras_conhecidas[i], tam) == 0) {
      if (tam > max_tam)
        max_tam = tam;
    }
  }
  return max_tam;
}

// Pula os espaços em branco e retorna o próximo caractere real
char proximoCaractereReal(const char *expr, int indice_atual) {
  int k = indice_atual;
  while (expr[k] == ' ')
    k++;
  return expr[k];
}
// faz a leitura da string de equação inicial e limpa ela transformando em
// tokens
char **inputReader(const char *expression) {
  int capacity = 100;
  char **token_array = (char **)malloc(capacity * sizeof(char *));
  int token_count = 0;

  int j = 0;
  while (expression[j] != '\0') {
    // Redimensiona se necessário
    if (token_count >= capacity - 3) {
      capacity *= 2;
      token_array = (char **)realloc(token_array, capacity * sizeof(char *));
    }

    char c = expression[j];

    // Ignora espaços
    if (c == ' ') {
      j++;
      continue;
    }

    bool e_fator = false; // Flag para saber se precisamos injetar '*' depois

    // Lê numeros
    if (isdigit(c) || c == '.') {
      char buffer[256];
      int buf_idx = 0;
      while (isdigit(expression[j]) || expression[j] == '.') {
        buffer[buf_idx++] = expression[j++];
      }
      buffer[buf_idx] = '\0';
      token_array[token_count++] = strdup(buffer);

      e_fator = true;
    }

    // Lê letras
    else if (isalpha(c)) {
      int tam = tamanhoPalavraConhecida(&expression[j]);

      if (tam > 0) {
        // Se achou  uma palavra do dicionário
        char buffer[256];
        strncpy(buffer, &expression[j], tam);
        buffer[tam] = '\0';
        token_array[token_count++] = strdup(buffer);

        j += tam; // Pula o tamanho da função no indice

        // Funções como "sin" não multiplicam o que vem na frente
        // mas constantes como "pi" e "e" sim (ex: "pi x" -> "pi * x")
        if (strcmp(buffer, "pi") == 0 || strcmp(buffer, "e") == 0) {
          e_fator = true;
        }
      } else {
        // Letra desconhecida. Assume que é variável única (ex: "x", "y")
        char op_str[2] = {c, '\0'};
        token_array[token_count++] = strdup(op_str);

        j++;
        e_fator =
            true; // Variáveis podem ser multiplicadas (ex: "xy" -> "x * y")
      }
    }

    // Lê Operadores e Parênteses
    else {
      if (c == '-') {
        bool is_unary = (token_count == 0);
        if (!is_unary) {
          char *ultimo = token_array[token_count - 1];
          if (strchr("+-*/^(~=", ultimo[0]) != NULL)
            is_unary = true;
        }

        if (is_unary)
          token_array[token_count++] = strdup("~");
        else
          token_array[token_count++] = strdup("-");
      } else {
        char op_str[2] = {c, '\0'};
        token_array[token_count++] = strdup(op_str);
      }

      if (c == ')' || c == ']')
        e_fator = true; // Parêntese fechando ativa a multiplicação

      j++;
    }

    // Injeção da Multiplicação Implícita
    if (e_fator) {
      char proximo = proximoCaractereReal(expression, j);
      // Se o próximo for letra, abrir parêntese, ou um número avulso
      if (isalpha(proximo) || proximo == '(' || proximo == '[' ||
          isdigit(proximo)) {
        token_array[token_count++] = strdup("*");
      }
    }
  }

  token_array[token_count] = NULL;
  return token_array;
}

void liberarTokens(char **tokens) {
  if (tokens == NULL)
    return;

  // Libera cada string (token) individualmente gerada pelo strdup
  int i = 0;
  while (tokens[i] != NULL) {
    free(tokens[i]);
    i++;
  }

  // Libera o array principal (o contêiner) gerado pelo malloc
  free(tokens);
}

// Retorna a precedência do operador (maior número = maior prioridade)
int getPrecedencia(const char *op) {
  if (strcmp(op, "=") == 0)
    return 1; // '=' tem a MENOR prioridade de todas
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
    return 2;
  if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0)
    return 3;
  if (strcmp(op, "^") == 0)
    return 4;
  if (strcmp(op, "~") == 0)
    return 5;
  if (isalpha(op[0]) && strlen(op) > 1)
    return 6;
  return 0;
}

bool isOperador(const char *token) {
  return strchr("+-*/^~=", token[0]) != NULL && strlen(token) == 1;
}

// Retorna true se o operador é associativo à direita
bool isAssociativoDireita(const char *op) {
  if (strcmp(op, "^") == 0 || strcmp(op, "~") == 0)
    return true;
  return false;
}

bool isFuncao(const char *token) {
  // Exceções: constantes matemáticas não são funções
  if (strcmp(token, "pi") == 0 || strcmp(token, "e") == 0) {
    return false;
  }

  // Se tem mais de 1 letra e não caiu na exceção acima, é função (sin, cos,
  // log)
  return isalpha(token[0]) && strlen(token) > 1;
}

char **shuntingYard(char **tokens) {
  // Inicializa a Pilha de Operações e a Fila de Saída (RPN)
  StringStack opStack;
  initStack(&opStack, 100);

  int capacidade_saida = 100;
  char **saida = (char **)malloc(capacidade_saida * sizeof(char *));
  int count = 0;

  // Loop principal: avalia token por token
  for (int i = 0; tokens[i] != NULL; i++) {
    char *token = tokens[i];

    // Se a fila de saída estiver enchendo, realoca memória
    if (count >= capacidade_saida - 2) {
      capacidade_saida *= 2;
      saida = (char **)realloc(saida, capacidade_saida * sizeof(char *));
    }

    // Número, variável (x, y) ou constante (pi, e) vão para saida
    if (isdigit(token[0]) || token[0] == '.' ||
        (isalpha(token[0]) && strlen(token) == 1) || strcmp(token, "pi") == 0 ||
        strcmp(token, "e") == 0) {

      saida[count++] = strdup(token);
    }

    // Função (sin, cos, tan) vai pra pilha.
    else if (isFuncao(token)) {
      pushStack(&opStack, token);
    }

    // Abre parênteses/colchetes vai pra pilha.
    else if (strcmp(token, "(") == 0 || strcmp(token, "[") == 0) {
      pushStack(&opStack, token);
    }

    // Fecha parênteses/colchetes
    else if (strcmp(token, ")") == 0 || strcmp(token, "]") == 0) {
      char abre_paren = (token[0] == ')') ? '(' : '[';

      // Desempilha tudo até achar o parêntese de abertura correspondente
      while (!isEmpty(&opStack) && peek(&opStack)[0] != abre_paren) {
        saida[count++] = popStack(&opStack); // Transfere o ponteiro
      }

      // Remove e descarta o parêntese de abertura da pilha
      if (!isEmpty(&opStack)) {
        char *lixo = popStack(&opStack);
        free(lixo); // Fazemos free porque não vai para a saída
      }

      // Se o topo da pilha for uma função (ex: o 'sin' de 'sin(x)'), desempilha
      // pra saída
      if (!isEmpty(&opStack) && isFuncao(peek(&opStack))) {
        saida[count++] = popStack(&opStack);
      }
    }

    // Operador (+, -, *, /, ^, ~)
    else if (isOperador(token)) {
      while (!isEmpty(&opStack)) {
        char *topo = peek(&opStack);

        // Parênteses na pilha bloqueiam a avaliação de precedência
        if (strcmp(topo, "(") == 0 || strcmp(topo, "[") == 0)
          break;

        int prec_token = getPrecedencia(token);
        int prec_topo = getPrecedencia(topo);

        // Desempilha se o operador no topo tiver precedência maior,
        // ou se tiver precedência igual mas for associativo à esquerda.
        if (prec_topo > prec_token ||
            (prec_topo == prec_token && !isAssociativoDireita(token))) {
          saida[count++] = popStack(&opStack);
        } else {
          break;
        }
      }
      // Finalmente, empilha o operador atual
      pushStack(&opStack, token);
    }
  }

  // 3. Fim da expressão: desempilha todos os operadores restantes
  while (!isEmpty(&opStack)) {
    char *restante = popStack(&opStack);

    // Tratamento de erro: se sobrou parêntese na pilha, a expressão original
    // estava mal formatada
    if (strcmp(restante, "(") == 0 || strcmp(restante, "[") == 0) {
      printf("Erro de Sintaxe: Parênteses desbalanceados.\n");
      free(restante);
      continue;
    }
    saida[count++] = restante;
  }

  // Adiciona a NULL no final do array de saída para sinalizar o final
  saida[count] = NULL;

  // Limpa a estrutura da pilha (apenas os ponteiros internos, não as strings em
  // si)
  free(opStack.items);

  return saida;
}

double evalRPN(char **rpnTokens, double x, double y) {
  DoubleStack stack;
  initDoubleStack(&stack, 50);
  int i = 0;
  while (rpnTokens[i] != NULL) {
    char *token = rpnTokens[i];
    // Variaveis
    if (strcmp(token, "x") == 0) {
      pushDoubleStack(&stack, x);
    } else if (strcmp(token, "y") == 0) {
      pushDoubleStack(&stack, y);
    } // Constantes
    else if (strcmp(token, "pi") == 0) {
      pushDoubleStack(&stack, M_PI);
    } else if (strcmp(token, "e") == 0) {
      pushDoubleStack(&stack, M_E);
    }
    // Operadores Binários (+, -, *, /, ^, =)
    else if (strlen(token) == 1 && strchr("+-*/^=", token[0]) != NULL) {
      double b = popDoubleStack(&stack); // Lado direito
      double a = popDoubleStack(&stack); // Lado esquerdo

      if (token[0] == '+')
        pushDoubleStack(&stack, a + b);
      else if (token[0] == '-' || token[0] == '=')
        pushDoubleStack(&stack, a - b);
      else if (token[0] == '*')
        pushDoubleStack(&stack, a * b);
      else if (token[0] == '/')
        pushDoubleStack(&stack, a / b);
      else if (token[0] == '^')
        pushDoubleStack(&stack, pow(a, b));
    }
    // Funções Unárias (sin, cos, sqrt, etc)
    else if (isFuncao(token) || strcmp(token, "~") == 0) {
      double a = popDoubleStack(&stack);

      // Trigonométricas básicas
      if (strcmp(token, "sin") == 0)
        pushDoubleStack(&stack, sin(a));
      else if (strcmp(token, "cos") == 0)
        pushDoubleStack(&stack, cos(a));
      else if (strcmp(token, "tan") == 0)
        pushDoubleStack(&stack, tan(a));

      // Trigonométricas inversas (arco)
      else if (strcmp(token, "arcsin") == 0)
        pushDoubleStack(&stack, asin(a));
      else if (strcmp(token, "arccos") == 0)
        pushDoubleStack(&stack, acos(a));
      else if (strcmp(token, "arctan") == 0)
        pushDoubleStack(&stack, atan(a));

      // Trigonométricas hiperbólicas
      else if (strcmp(token, "sinh") == 0)
        pushDoubleStack(&stack, sinh(a));
      else if (strcmp(token, "cosh") == 0)
        pushDoubleStack(&stack, cosh(a));
      else if (strcmp(token, "tanh") == 0)
        pushDoubleStack(&stack, tanh(a));

      // Trigonométricas recíprocas (calculadas via divisão)
      else if (strcmp(token, "csc") == 0)
        pushDoubleStack(&stack, 1.0 / sin(a));
      else if (strcmp(token, "sec") == 0)
        pushDoubleStack(&stack, 1.0 / cos(a));
      else if (strcmp(token, "cot") == 0)
        pushDoubleStack(&stack, 1.0 / tan(a));

      // Trigonométricas recíprocas inversas
      else if (strcmp(token, "arccsc") == 0)
        pushDoubleStack(&stack, asin(1.0 / a));
      else if (strcmp(token, "arcsec") == 0)
        pushDoubleStack(&stack, acos(1.0 / a));
      else if (strcmp(token, "arccot") == 0)
        pushDoubleStack(&stack,
                        M_PI / 2.0 - atan(a)); // Identidade do arco-cotangente

      // Trigonométricas recíprocas hiperbólicas
      else if (strcmp(token, "csch") == 0)
        pushDoubleStack(&stack, 1.0 / sinh(a));
      else if (strcmp(token, "sech") == 0)
        pushDoubleStack(&stack, 1.0 / cosh(a));
      else if (strcmp(token, "coth") == 0)
        pushDoubleStack(&stack, 1.0 / tanh(a));

      // Logaritmos e Raiz
      else if (strcmp(token, "log") == 0)
        pushDoubleStack(&stack, log10(a));
      else if (strcmp(token, "ln") == 0)
        pushDoubleStack(&stack, log(a));
      else if (strcmp(token, "sqrt") == 0)
        pushDoubleStack(&stack, sqrt(a));

      // Menos unário
      else if (strcmp(token, "~") == 0)
        pushDoubleStack(&stack, -a);
    } else {
      pushDoubleStack(&stack, atof(token));
    }
    i++;
  }

  // O último valor restante na pilha é o resultado final
  double resultadoFinal = popDoubleStack(&stack);

  // Limpa a casa antes de sair
  freeDoubleStack(&stack);

  return resultadoFinal;
}
