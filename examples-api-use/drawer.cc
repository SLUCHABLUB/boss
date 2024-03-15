// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the input bits
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "common.h"
#include "led-matrix.h"
#include "graphics.h"

#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <deque>

#define UP_KEY 'w'
#define LEFT_KEY 'a'
#define DOWN_KEY 's'
#define RIGHT_KEY 'd'
#define PEN_UP_KEY 'e'
#define PEN_DOWN_KEY 'r'
#define ERASER_KEY 't'
#define CLEAR_KEY 'p'

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static void InteractiveUseMessage() {
  fprintf(stderr,
          "Move around with common movement keysets \n"
          " W,A,S,D (gaming move style) or\n"
          " Set mode with E(pen down), R(pen up), T(eraser)\n"
          " Clear screen with P\n"
          " Change color with C\n"
          " Rainbow mode with X\n"
          " Quit with Q or <ESC>\n"
          "The pixel position cannot be less than 0 or greater than the "
          "display height and width.\n");
}

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Display single pixel with any colour.\n");
  InteractiveUseMessage();
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

static char getch() {
  static bool is_terminal = isatty(STDIN_FILENO);

  struct termios old;
  if (is_terminal) {
    if (tcgetattr(0, &old) < 0)
      perror("tcsetattr()");

    // Set to unbuffered mode
    struct termios no_echo = old;
    no_echo.c_lflag &= ~ICANON;
    no_echo.c_lflag &= ~ECHO;
    no_echo.c_cc[VMIN] = 1;
    no_echo.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &no_echo) < 0)
      perror("tcsetattr ICANON");
  }

  char buf = 0;
  if (read(STDIN_FILENO, &buf, 1) < 0)
    perror ("read()");

  if (is_terminal) {
    // Back to original terminal settings.
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
      perror ("tcsetattr ~ICANON");
  }

  return buf;
}

static void set_color_matrix(Color **c_mat, int r_size, int c_size, Color val){
  for(int r = 0; r < r_size; r++){
    for(int c = 0; c < c_size; c++){
      c_mat[r][c] = val;
    }
  }
}

static Color ask_color(){
  printf("Enter a color <r,g,b>: ");
  uint8_t r, g, b;
  scanf("%hhu,%hhu,%hhu", &r, &g, &b);
  return Color(r,g,b);
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
	matrix_options.hardware_mapping = HW_ID;
	matrix_options.rows = LED_MATRIX_HEIGHT;
	matrix_options.cols = LED_MATRIX_WIDTH;
	matrix_options.chain_length = BOSS_WIDTH;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  Color draw_color(242, 128, 161);
  Color cursor_color(255, 255, 0);
  Color background_color(0, 0, 0);

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return usage(argv[0]);

  //rgb_matrix::FrameCanvas *canvas = matrix->CreateFrameCanvas();
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  int x_pos = 0;
  int y_pos = 0;

  InteractiveUseMessage();
  const bool output_is_terminal = isatty(STDOUT_FILENO);

  enum pen_mode{
    penUp,
    penDown,
    eraser
  };

  pen_mode mode{penDown};

  Color **current_color = new Color*[matrix->height()];
  for(int r = 0; r < matrix->height(); r++){
    current_color[r] = new Color[matrix->width()];
  }

  bool running = true;
  bool rainbow_mode = false;
  short angle = 0;
  while (!interrupt_received && running) {
    matrix->SetPixel(x_pos, y_pos, cursor_color.r, cursor_color.g, cursor_color.b);
    if(rainbow_mode){
      draw_color = trueHSV(angle);
      angle = (angle + 1) % 360;
    }

    printf("%sX,Y = %d,%d%s",
           output_is_terminal ? "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" : "",
           x_pos, y_pos,
           output_is_terminal ? " " : "\n");
    fflush(stdout);

    const char c = tolower(getch());
    if(mode == penDown){
      matrix->SetPixel(x_pos, y_pos, draw_color.r, draw_color.g, draw_color.b);
      current_color[y_pos][x_pos] = draw_color;
    } else if(mode == eraser){
      matrix->SetPixel(x_pos, y_pos, background_color.r, background_color.g, background_color.b);
      current_color[y_pos][x_pos] = background_color;
    } else if(mode == penUp){
      const Color old_color = current_color[y_pos][x_pos];
      matrix->SetPixel(x_pos, y_pos, old_color.r, old_color.g, old_color.b);
    }
    switch (c) {
    case 'w':   // Up
      if (y_pos > 0) {
        y_pos--;
      }
      break;
    case 's':  // Down
      if (y_pos < matrix->height() - 1) {
        y_pos++;
      }
      break;
    case 'a':  // Left
      if (x_pos > 0) {
        x_pos--;
      }
      break;
    case 'd':  // Right
      if (x_pos < matrix->width() - 1) {
        x_pos++;
      }
      break;
    case 'e':
      mode = penDown;
      break;
    case 'r':
      mode = penUp;
      break;
    case 't':
      mode = eraser;
      break;
    case 'p':
      matrix->Clear();
      set_color_matrix(current_color, matrix->height(), matrix->width(), background_color);
      break;
    case 'c':
      draw_color = ask_color();
      rainbow_mode = false;
      break;
    case 'x':
      rainbow_mode = true;
      break;

    // All kinds of conditions which we use to exit
    case 0x1B:           // Escape
    case 'q':            // 'Q'uit
    case 0x04:           // End of file
    case 0x00:           // Other issue from getch()
      running = false;
      break;
    }
  }

  // Finished. Shut down the RGB matrix.
  for(int r = 0; r < matrix->height(); r++){
    delete[] current_color[r];
  }
  delete[] current_color;
  delete matrix;
  printf("\n");
  return 0;
}
