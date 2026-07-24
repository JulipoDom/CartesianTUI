#include "engine-core.h"
#include "tui.h"
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void aplicarZoom(double *min, double *max, double fator) {
  double centro = (*max + *min) / 2.0;
  double alcance = (*max - *min) / 2.0;
  *min = centro - (alcance * fator);
  *max = centro + (alcance * fator);
}

void moverPlano(double *min, double *max, double direcao) {
  double deslocamento = (*max - *min) * 0.1 * direcao;
  *min += deslocamento;
  *max += deslocamento;
}

// RPN = Reverse Polish Notation (Notação Polonesa Inversa)
// Serve para gerar Stacks conservando Ordem de Operação

int main() {
  setlocale(LC_ALL, "");

  char historico_eq[MAX_EQUACOES][TAM_MAX_EQ];
  char ***rpn_cache = malloc(MAX_EQUACOES * sizeof(char **));
  int total_eqs = 0;
  bool modo_suave = true;
  printf("\033[?1003h");
  initTUI();
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  nodelay(stdscr, TRUE);
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  double x_min = -((cols - 2) / 4.0), x_max = ((cols - 2) / 4.0);
  double y_min = -((rows - 2) / 2.0), y_max = ((rows - 2) / 2.0);

  double x_min_alvo = x_min, x_max_alvo = x_max;
  double y_min_alvo = y_min, y_max_alvo = y_max;

  int mouse_x = -1, mouse_y = -1;
  bool rodando = true;
  MEVENT event;

  desenharTUI();
  limparMatrizTela(x_min, x_max, y_min, y_max);

  while (rodando) {
    bool redesenhar = false;
    // Aplica Suavização com velocidade de 20% por iteração
    if (modo_suave && (fabs(x_max_alvo - x_max) > 0.0001 ||
                       fabs(y_max_alvo - y_max) > 0.0001)) {
      double suavizacao = 0.2;
      x_min += (x_min_alvo - x_min) * suavizacao;
      x_max += (x_max_alvo - x_max) * suavizacao;
      y_min += (y_min_alvo - y_min) * suavizacao;
      y_max += (y_max_alvo - y_max) * suavizacao;
      redesenhar = true;
      // Caso o modo suave esteja desligado, teleporta para o ponto
    } else if (!modo_suave && (x_min != x_min_alvo || x_max != x_max_alvo ||
                               y_min != y_min_alvo || y_max != y_max_alvo)) {
      x_min = x_min_alvo;
      x_max = x_max_alvo;
      y_min = y_min_alvo;
      y_max = y_max_alvo;
      redesenhar = true;
    }
    int ch = getch();
    if (ch != ERR) {
      redesenhar = true; // Se o usuário apertou algo, redesenha imediatamente

      if (ch == KEY_MOUSE) {
        if (getmouse(&event) == OK) {
          if (event.bstate & BUTTON3_PRESSED ||
              event.bstate & BUTTON3_CLICKED) {
            mouse_x = -1;
            mouse_y = -1;
          } else {
            mouse_x = event.x;
            mouse_y = event.y;
          }
        }
      } else {
        // Funções da interface
        switch (ch) {
        case 'q':
          rodando = false;
          break;
        case 'h':
          // Pausa o nodelay momentaneamente para o menu de help aceitar input
          nodelay(stdscr, FALSE);
          mostrarHelpTUI();
          nodelay(stdscr, TRUE);
          break;
        case 'm':
          modo_suave = !modo_suave;
          if (!modo_suave) {
            x_min_alvo = x_min;
            x_max_alvo = x_max;
            y_min_alvo = y_min;
            y_max_alvo = y_max;
          }
          break;
        // Criar novas equações e seus tokens
        case 'n':
        case 's':
          nodelay(stdscr, FALSE);
          if (ch == 'n') {
            for (int i = 0; i < total_eqs; i++)
              liberarTokens(rpn_cache[i]);
            total_eqs = 0;
          }
          if (total_eqs < MAX_EQUACOES) {
            char temp_eq[TAM_MAX_EQ];
            lerEquacaoTUI(temp_eq,
                          (ch == 'n') ? "Nova Equacao" : "Sobrepor Equacao");
            if (strlen(temp_eq) > 0) {
              strcpy(historico_eq[total_eqs], temp_eq);
              char **tokens = inputReader(historico_eq[total_eqs]);
              if (tokens) {
                rpn_cache[total_eqs] = shuntingYard(tokens);
                liberarTokens(tokens);
                total_eqs++;
              }
            }
          }
          nodelay(stdscr, TRUE); // Retoma o fluxo fluido
          break;

        // Comandos de Zoom e Movimento alteram os alvos
        case 'x': {
          double cx = (x_max_alvo + x_min_alvo) / 2,
                 rx = (x_max_alvo - x_min_alvo) / 4;
          x_min_alvo = cx - rx;
          x_max_alvo = cx + rx;
          break;
        }
        case 'X': {
          double cx = (x_max_alvo + x_min_alvo) / 2,
                 rx = (x_max_alvo - x_min_alvo);
          x_min_alvo = cx - rx;
          x_max_alvo = cx + rx;
          break;
        }
        case 'y': {
          double cy = (y_max_alvo + y_min_alvo) / 2,
                 ry = (y_max_alvo - y_min_alvo) / 4;
          y_min_alvo = cy - ry;
          y_max_alvo = cy + ry;
          break;
        }
        case 'Y': {
          double cy = (y_max_alvo + y_min_alvo) / 2,
                 ry = (y_max_alvo - y_min_alvo);
          y_min_alvo = cy - ry;
          y_max_alvo = cy + ry;
          break;
        }
        case 'z': {
          double cx = (x_max_alvo + x_min_alvo) / 2,
                 rx = (x_max_alvo - x_min_alvo) / 4;
          double cy = (y_max_alvo + y_min_alvo) / 2,
                 ry = (y_max_alvo - y_min_alvo) / 4;
          x_min_alvo = cx - rx;
          x_max_alvo = cx + rx;
          y_min_alvo = cy - ry;
          y_max_alvo = cy + ry;
          break;
        }
        case 'Z': {
          double cx = (x_max_alvo + x_min_alvo) / 2,
                 rx = (x_max_alvo - x_min_alvo);
          double cy = (y_max_alvo + y_min_alvo) / 2,
                 ry = (y_max_alvo - y_min_alvo);
          x_min_alvo = cx - rx;
          x_max_alvo = cx + rx;
          y_min_alvo = cy - ry;
          y_max_alvo = cy + ry;
          break;
        }
        case KEY_RIGHT: {
          double d = (x_max_alvo - x_min_alvo) * 0.05;
          x_min_alvo += d;
          x_max_alvo += d;
          break;
        }
        case KEY_LEFT: {
          double d = (x_max_alvo - x_min_alvo) * 0.05;
          x_min_alvo -= d;
          x_max_alvo -= d;
          break;
        }
        case KEY_UP: {
          double d = (y_max_alvo - y_min_alvo) * 0.05;
          y_min_alvo += d;
          y_max_alvo += d;
          break;
        }
        case KEY_DOWN: {
          double d = (y_max_alvo - y_min_alvo) * 0.05;
          y_min_alvo -= d;
          y_max_alvo -= d;
          break;
        }
        case 'r':
          getmaxyx(stdscr, rows, cols);
          x_min_alvo = -((cols - 2) / 4.0);
          x_max_alvo = ((cols - 2) / 4.0);
          y_min_alvo = -((rows - 2) / 2.0);
          y_max_alvo = ((rows - 2) / 2.0);
          break;
        }
      }
    }

    // Só redesenha a tela se a câmera estiver se
    // movendo ou se houve input
    if (redesenhar || (modo_suave && fabs(x_max_alvo - x_max) > 0.0001)) {
      erase();
      limparMatrizTela(x_min, x_max, y_min, y_max);

      for (int i = 0; i < total_eqs; i++) {
        int cor_id = (i % 6) + 1;
        if (rpn_cache[i] != NULL) {
          plotarNaMatriz(rpn_cache[i], x_min, x_max, y_min, y_max, cor_id);
        }
      }

      desenharCaixaEquacoes(historico_eq, total_eqs);
      if (mouse_x != -1 && mouse_y != -1 && total_eqs > 0) {
        desenharHoverMouse(mouse_x, mouse_y, rpn_cache, total_eqs, x_min,
                           x_max);
      }

      desenharTUI();
    }

    // Dorme por 16 milissegundos por quadro para travar exatamente em 60 FPS e
    // não fritar tanto a CPU.
    usleep(16000);
  }

  // Caso saia do While ele termina o programa;

  closeTUI();
  printf("\033[?1003l\n");
  free(rpn_cache);
  return 0;
}
