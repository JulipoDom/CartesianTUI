#ifndef TUI_H
#define TUI_H

#include <stdbool.h>

#define MAX_EQUACOES 10
#define TAM_MAX_EQ 256

// Adicione as variáveis globais de dimensões se não estiverem exportadas
extern int largura_grafico;
extern int altura_grafico;
extern int offset_x;
extern int offset_y;

void initTUI();
void closeTUI();
void limparMatrizTela(double x_min, double x_max, double y_min, double y_max);
void plotarNaMatriz(char **tokens, double x_min, double x_max, double y_min,
                    double y_max, int cor_id);
void desenharTUI();
void lerEquacaoTUI(char *buffer, const char *titulo_janela);
void mostrarHelpTUI();

// NOVAS FUNÇÕES
void desenharCaixaEquacoes(char historico[MAX_EQUACOES][TAM_MAX_EQ], int total);
void desenharHoverMouse(int mx, int my, char ***rpn_lista, int total,
                        double x_min, double x_max);

#endif
