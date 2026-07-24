#include "tui.h"
#include "engine-core.h"
#include <ncurses.h>
#include <string.h>

int largura_grafico = 0;
int altura_grafico = 0;
int offset_y = 1; // Espaço para a borda superior
int offset_x = 1; // Espaço para a borda lateral

// Inicialização do nscurses, configura a abertura de tela e cores padrão do
// terminal para interface
void initTUI() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    use_default_colors();

    // Cores das equações matemáticas (1 a 6)
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_WHITE, -1);

    // Cores TEMÁTICAS DA UI
    init_pair(7, COLOR_CYAN, -1);
    init_pair(8, COLOR_WHITE, -1);
  }
}

// Reseta o a matriz de linhas e colunas mas não desenha isso na tela
void limparMatrizTela(double x_min, double x_max, double y_min, double y_max) {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  largura_grafico = cols - 2;
  altura_grafico = rows - 2;

  if (altura_grafico < 5)
    altura_grafico = 5;

  int col_zero = (int)((0.0 - x_min) / (x_max - x_min) * largura_grafico);
  int lin_zero = altura_grafico - 1 -
                 (int)((0.0 - y_min) / (y_max - y_min) * altura_grafico);

  // Liga a cor neutra do terminal para colocar o tracejado do plano cartesiano
  attron(COLOR_PAIR(8) | A_DIM);

  for (int i = 0; i < altura_grafico; i++) {
    for (int j = 0; j < largura_grafico; j++) {
      char c = ' ';
      if (i == lin_zero && j == col_zero)
        c = '+';
      else if (i == lin_zero)
        c = '-';
      else if (j == col_zero)
        c = '|';

      mvaddch(offset_y + i, offset_x + j, c);
    }
  }

  // Desenhando os Labels com a mesma cor neutra
  if (lin_zero >= 0 && lin_zero < altura_grafico) {
    mvprintw(offset_y + lin_zero + 1, offset_x + 2, "%.1f", x_min);
    char str_xmax[20];
    snprintf(str_xmax, 20, "%.1f", x_max);
    mvprintw(offset_y + lin_zero + 1,
             offset_x + largura_grafico - strlen(str_xmax) - 2, "%s", str_xmax);
  }

  if (col_zero >= 0 && col_zero < largura_grafico) {
    mvprintw(offset_y + 1, offset_x + col_zero + 1, "%.1f", y_max);
    mvprintw(offset_y + altura_grafico - 2, offset_x + col_zero + 1, "%.1f",
             y_min);
  }

  if (lin_zero >= 0 && lin_zero < altura_grafico && col_zero >= 0 &&
      col_zero < largura_grafico) {
    mvprintw(offset_y + lin_zero + 1, offset_x + col_zero + 1, "0");
  }

  // Desliga o efeito do plano de fundo
  attroff(COLOR_PAIR(8) | A_DIM);
}

void closeTUI() { endwin(); }

// Função que pega os rpn_tokens e plota na Matriz sem desenhar
void plotarNaMatriz(char **tokens, double x_min, double x_max, double y_min,
                    double y_max, int cor_id) {
  double passo_x = (x_max - x_min) / largura_grafico;
  double passo_y = (y_max - y_min) / altura_grafico;
  // Modificação que trata equações bivariadas
  bool tem_igual = false, tem_x = false, tem_y = false;
  for (int i = 0; tokens[i] != NULL; i++) {
    if (strcmp(tokens[i], "=") == 0)
      tem_igual = true;
    if (strcmp(tokens[i], "x") == 0)
      tem_x = true;
    if (strcmp(tokens[i], "y") == 0)
      tem_y = true;
  }
  // Atribui a nova cor para função
  attron(COLOR_PAIR(cor_id));

  for (int row = 0; row < altura_grafico; row++) {
    for (int col = 0; col < largura_grafico; col++) {
      double real_x_base = x_min + (col * passo_x);
      double real_y_base = y_max - (row * passo_y);

      // Dividimos cada célula do terminal em uma mini-grade de 2x2

      int sub_div = 2;
      bool deve_plotar = false;

      // Loop de iteração que analisa celula à celula se o ponto é solução para
      // equação, caso for ele é plotado na matriz e segue para as proximas
      // celulas
      for (int sy = 0; sy < sub_div; sy++) {
        for (int sx = 0; sx < sub_div; sx++) {
          double sub_x = real_x_base + (sx * (passo_x / sub_div));
          double sub_y = real_y_base - (sy * (passo_y / sub_div));

          double v_atual = evalRPN(tokens, sub_x, sub_y);
          double v_dir = evalRPN(tokens, sub_x + (passo_x / sub_div), sub_y);
          double v_baixo = evalRPN(tokens, sub_x, sub_y - (passo_y / sub_div));

          if (!tem_igual) {
            if (tem_y && !tem_x) {
              v_atual = sub_x - v_atual;
              v_dir = (sub_x + (passo_x / sub_div)) - v_dir;
              v_baixo = sub_x - v_baixo;
            } else {
              v_atual = sub_y - v_atual;
              v_dir = sub_y - v_dir;
              v_baixo = (sub_y - (passo_y / sub_div)) - v_baixo;
            }
          }

          bool cruza_x =
              (v_atual > 0 && v_dir <= 0) || (v_atual <= 0 && v_dir > 0);
          bool cruza_y =
              (v_atual > 0 && v_baixo <= 0) || (v_atual <= 0 && v_baixo > 0);

          if (cruza_x || cruza_y) {
            deve_plotar = true;
            break;
          }
        }
        if (deve_plotar)
          break;
      }

      if (deve_plotar) {
        mvprintw(offset_y + row, offset_x + col, "•");
      }
    }
  }
  attroff(COLOR_PAIR(cor_id));
}
// Sim é impressionante que uma função com tantos for loops consiga rodar muitas
// vezes por segundo

// Desenha alterações e o layout externo da interface
void desenharTUI() {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  // Liga a cor de Destaque da UI
  attron(COLOR_PAIR(7));

  box(stdscr, 0, 0); // Borda principal

  const char *titulo = " CartesianTUI ";
  int padding = (cols - strlen(titulo)) / 2;
  attron(A_BOLD);
  mvprintw(0, padding > 0 ? padding : 0, "%s", titulo);
  attroff(A_BOLD);

  const char *cmds = " [n] Nova | [s] Sobrepor | [Setas] Mover | [x/y/z] Zoom "
                     "| [m] Suavizar: ON/OFF "
                     "| [r] Reset | [h] Help | [q] Sair ";
  int padding_cmds = (cols - strlen(cmds)) / 2;

  // Imprime a barra de comandos
  mvprintw(rows - 1, padding_cmds > 0 ? padding_cmds : 1, "%s", cmds);

  // Desliga a cor de Destaque da UI
  attroff(COLOR_PAIR(7));

  refresh();
}

// Painel de Input para novas equações, a leitura é jogada no parametro buffer
void lerEquacaoTUI(char *buffer, const char *titulo_janela) {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  int w = 50, h = 5;
  int start_y = (rows - h) / 2;
  int start_x = (cols - w) / 2;

  WINDOW *input_win = newwin(h, w, start_y, start_x);

  wattron(input_win, COLOR_PAIR(5));
  box(input_win, 0, 0);
  wattroff(input_win, COLOR_PAIR(5));

  wattron(input_win, COLOR_PAIR(5) | A_BOLD);
  mvwprintw(input_win, 0, 2, " %s ", titulo_janela);
  wattroff(input_win, COLOR_PAIR(5) | A_BOLD);

  mvwprintw(input_win, 2, 2, "Digite: ");

  noecho();
  keypad(input_win, TRUE);
  curs_set(1);

  int str_len = 0;
  int cursor_pos = 0;
  int offset = 0;
  int max_w = 37;

  buffer[0] = '\0';

  while (1) {
    // 1. Limpa e desenha apenas a janela visível da string
    mvwprintw(input_win, 2, 10, "%-*.*s", max_w, max_w, &buffer[offset]);

    // 2. Coloca o cursor piscando na posição correta na tela
    wmove(input_win, 2, 10 + (cursor_pos - offset));
    wrefresh(input_win);

    // 3. Aguarda o usuário digitar
    int ch = wgetch(input_win);

    if (ch == '\n' || ch == '\r') {
      break;
    } else if (ch == KEY_LEFT) {
      if (cursor_pos > 0) {
        cursor_pos--;
        // Se o cursor for para trás da parte visível, arrasta o scroll para a
        // esquerda
        if (cursor_pos < offset) {
          offset = cursor_pos;
        }
      }
    } else if (ch == KEY_RIGHT) {
      if (cursor_pos < str_len) {
        cursor_pos++;
        // Se o cursor ultrapassar a margem direita, empurra o scroll
        if (cursor_pos >= offset + max_w) {
          offset = cursor_pos - max_w + 1;
        }
      }
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (cursor_pos > 0) {
        // Puxa tudo que está à direita do cursor uma casa para a esquerda
        // (apagando o caractere)
        memmove(&buffer[cursor_pos - 1], &buffer[cursor_pos],
                str_len - cursor_pos + 1);
        cursor_pos--;
        str_len--;

        // Ajusta o scroll caso o cursor volte demais
        if (cursor_pos < offset) {
          offset = cursor_pos;
        }
      }
    } else if (str_len < TAM_MAX_EQ - 1 && ch >= 32 && ch <= 126) {
      // Abre espaço empurrando tudo que está na frente do cursor uma casa para
      // a direita
      memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos],
              str_len - cursor_pos + 1);

      // Insere o novo caractere na posição exata do cursor
      buffer[cursor_pos] = ch;
      cursor_pos++;
      str_len++;

      // Atualiza o scroll se a digitação passar do limite visível
      if (cursor_pos >= offset + max_w) {
        offset = cursor_pos - max_w + 1;
      }
    }
  }

  curs_set(0);
  delwin(input_win);
  clear();
  refresh();
}

// Mostra o painel de ajuda na interface
void mostrarHelpTUI(void) {
  // Define o tamanho da janela modal
  int altura = 19;
  int largura = 60;
  int starty = (LINES - altura) / 2;
  int startx = (COLS - largura) / 2;

  // Cria a janela e habilita a leitura de setas (KEY_RIGHT / KEY_LEFT)
  WINDOW *win_help = newwin(altura, largura, starty, startx);
  keypad(win_help, TRUE);

  int pagina_atual = 1;
  int total_paginas = 4;
  int ch;

  // Loop da janela de ajuda
  while (1) {
    wclear(win_help);
    box(win_help, 0, 0); // Desenha a borda

    if (pagina_atual == 1) {
      mvwprintw(win_help, 1, (largura - 23) / 2, "=== AJUDA (Pag 1/4) ===");

      int linha = 3;
      mvwprintw(win_help, linha++, 2, "Navegacao e Controle:");
      ;
      mvwprintw(win_help, linha++, 2, "  Setas      : Move o grafico (Pan)");
      mvwprintw(win_help, linha++, 2, "  + / -      : Zoom in / Zoom out");
      mvwprintw(win_help, linha++, 2, "  R          : Reseta a visualizacao");

      linha++;
      mvwprintw(win_help, linha++, 2, "Interacao:");
      mvwprintw(win_help, linha++, 2, "  I          : Inserir nova equacao");
      mvwprintw(win_help, linha++, 2,
                "  Mouse Hover: Mostra valor exato de y no grafico");
      mvwprintw(win_help, linha++, 2, "  Click Dir. : Limpa a mira do mouse");

      linha++;
      mvwprintw(win_help, linha++, 2, "Sistema:");
      mvwprintw(win_help, linha++, 2,
                "  H          : Mostra esta tela de ajuda");
      mvwprintw(win_help, linha++, 2, "  Q / ESC    : Sair do programa");
    } else if (pagina_atual == 2) {
      mvwprintw(win_help, 1, (largura - 25) / 2, "=== FUNCOES (Pag 2/4) ===");

      int linha = 3;
      mvwprintw(win_help, linha++, 2, "Funcoes Matematicas Suportadas:");
      linha++;
      mvwprintw(win_help, linha++, 2, "Operadores: +, -, *, /, ^ (potencia)");
      linha++;
      mvwprintw(win_help, linha++, 2, "Funcoes Diversas : sqrt(), log(), ln()");
      mvwprintw(win_help, linha++, 2, "Trig. Basicas    : sin(), cos(), tan()");
      mvwprintw(win_help, linha++, 2, "                   sec(), csc(), cot()");
      mvwprintw(win_help, linha++, 2,
                "Trig. Inversas   : arcsin(), arccos(), arctan(),");
      mvwprintw(win_help, linha++, 2,
                "                   arcsec(), arccsc(), arccot()");
      mvwprintw(win_help, linha++, 2,
                "Trig. Hiperbol.  : sinh(), cosh(), tanh()");

      mvwprintw(win_help, linha++, 2,
                "                   sech(), csch(), coth()");

      mvwprintw(win_help, linha++, 2, "Constantes       : pi, e");
    } else if (pagina_atual == 3) {
      mvwprintw(win_help, 1, (largura - 34) / 2,
                "=== MODO DE OPERACAO (Pag 3/4) ===");

      int linha = 3;
      mvwprintw(win_help, linha++, 2, "Valores Implicitos em Equação:");
      mvwprintw(win_help, linha++, 2, "1. Multiplicação");
      mvwprintw(win_help, linha++, 2,
                "   Ex: 12x -> 12 * x; piexsin(x) -> pi * e * x * sin(x)");
      mvwprintw(win_help, linha++, 2, "2. Negatividade ( '-' Unario )");
      mvwprintw(win_help, linha++, 2,
                "   Ex: -(x - 1) -> -x + 1; -(-(-(-1))) -> 1");
      mvwprintw(win_help, linha++, 2,
                "Ordem de Operacoes (Maior > Menor prioridade):");
      mvwprintw(win_help, linha++, 2, "  1. ( )   Agrupamentos e Parenteses");
      mvwprintw(win_help, linha++, 2,
                "  2. f(x)  Funcoes (sin, log, sqrt, etc)");
      mvwprintw(win_help, linha++, 2,
                "  3.  -    Sinal Negativo Unario (Ex: -x)");
      mvwprintw(win_help, linha++, 2, "  4.  ^    Potenciacao");
      mvwprintw(win_help, linha++, 2,
                "  5. * /   Multiplicacao (inclui implícita) e Divisao");
      mvwprintw(win_help, linha++, 2, "  6. + -   Adicao e Subtracao");
      mvwprintw(win_help, linha++, 2,
                "  7.  =    Igualdade (Equacoes Implicitas)");
    } else if (pagina_atual == 4) {
      mvwprintw(win_help, 1, (largura - 37) / 2,
                "=== SINTAXE DE EQUACOES (Pag 4/4) ===");

      int linha = 3;
      mvwprintw(win_help, linha++, 2, "1. Funcoes de X (y = f(x))");
      mvwprintw(win_help, linha++, 2,
                "   Ex: x^2 + 2x  (O programa assume y= automaticamente)");

      linha++;
      mvwprintw(win_help, linha++, 2, "2. Funcoes de Y (x = f(y))");
      mvwprintw(win_help, linha++, 2,
                "   Ex: y^2 - 4   (O programa assume x= automaticamente)");

      linha++;
      mvwprintw(win_help, linha++, 2, "3. Multivariaveis / Implicitas");
      mvwprintw(win_help, linha++, 2,
                "   Misture x e y usando o sinal de '='.");
      mvwprintw(win_help, linha++, 2,
                "   Ex: x^2 + y^2 = 25 (Desenha um circulo)");

      linha++;
      mvwprintw(win_help, linha++, 2, "4. Retas Constantes");
      mvwprintw(win_help, linha++, 2,
                "   Use a variavel e o valor. Ex: x = 5 ou y = -2");
    }

    // Rodapé interativo instruindo o usuário
    mvwprintw(win_help, altura - 2, 2, "[<-/->] Mudar Pagina   [Q/ESC] Fechar");

    wrefresh(win_help);

    // Aguarda a ação do usuário dentro do menu
    ch = wgetch(win_help);

    // Regras de navegação
    if (ch == 'q' || ch == 'Q' || ch == 27) { // 27 = ESC
      break;                                  // Sai do loop e fecha o help
    } else if (ch == KEY_RIGHT) {
      if (pagina_atual < total_paginas)
        pagina_atual++;
    } else if (ch == KEY_LEFT) {
      if (pagina_atual > 1)
        pagina_atual--;
    }
  }

  // Limpa a memória da janela e força a tela principal a ser redesenhada
  delwin(win_help);
  clear();   // Limpa o fundo para remover resquícios do modal
  refresh(); // Atualiza a tela base
}

void desenharCaixaEquacoes(char historico[MAX_EQUACOES][TAM_MAX_EQ],
                           int total) {
  if (total == 0)
    return;

  int start_y = offset_y + 1;
  int start_x = offset_x + 2;
  int largura = 25;

  // Desenha a borda da caixa flutuante com a cor de destaque
  attron(COLOR_PAIR(7));
  mvhline(start_y, start_x, ACS_HLINE, largura);
  mvhline(start_y + total + 1, start_x, ACS_HLINE, largura);
  mvvline(start_y, start_x, ACS_VLINE, total + 2);
  mvvline(start_y, start_x + largura, ACS_VLINE, total + 2);
  mvaddch(start_y, start_x, ACS_ULCORNER);
  mvaddch(start_y, start_x + largura, ACS_URCORNER);
  mvaddch(start_y + total + 1, start_x, ACS_LLCORNER);
  mvaddch(start_y + total + 1, start_x + largura, ACS_LRCORNER);
  attroff(COLOR_PAIR(7));

  // Preenche com as equações ativas e suas respectivas cores
  for (int i = 0; i < total; i++) {
    int cor_id = (i % 6) + 1;
    attron(COLOR_PAIR(cor_id));
    mvprintw(start_y + i + 1, start_x + 1, " f%d(x): %.15s ", i + 1,
             historico[i]);
    attroff(COLOR_PAIR(cor_id));
  }
}

void desenharHoverMouse(int mx, int my, char ***rpn_lista, int total,
                        double x_min, double x_max) {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  // Verifica se o mouse está estritamente dentro da área do gráfico
  if (mx < offset_x || mx >= offset_x + largura_grafico || my < offset_y ||
      my >= offset_y + altura_grafico)
    return;

  double passo_x = (x_max - x_min) / largura_grafico;
  double real_x = x_min + ((mx - offset_x) * passo_x);

  // 1. Desenha uma "Mira" vertical (Crosshair)
  attron(COLOR_PAIR(8) | A_DIM);
  for (int i = 0; i < altura_grafico; i++) {
    mvaddch(offset_y + i, mx, ACS_VLINE);
  }
  attroff(COLOR_PAIR(8) | A_DIM);

  int bal_y = my + 1;
  int bal_x = mx + 2;

  if (bal_x + 20 > cols)
    bal_x = mx - 22;

  attron(COLOR_PAIR(7) | A_REVERSE);
  mvprintw(bal_y, bal_x, " X: %+.3f ", real_x);
  attroff(COLOR_PAIR(7) | A_REVERSE);

  for (int i = 0; i < total; i++) {
    double val_y = evalRPN(rpn_lista[i], real_x, 0.0);
    int cor_id = (i % 6) + 1;

    attron(COLOR_PAIR(cor_id) | A_REVERSE);
    mvprintw(bal_y + i + 1, bal_x, " f%d: %+.3f ", i + 1, val_y);
    attroff(COLOR_PAIR(cor_id) | A_REVERSE);
  }
}
