#include "common.h"
#include "led-matrix.h"
#include "graphics.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <atomic>
#include <chrono>

#include <pthread.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

using namespace rgb_matrix;
using std::vector;
using std::string;

class Pipe
{
public:
    Pipe(int col, int low, int high) : col{col}, low{low}, high{high} {}
    int col, low, high;
};

std::atomic<char> last_char;

volatile bool interrupt_received = false; // atomic?
static void InterruptHandler(int signo)
{
    interrupt_received = true;
}

static int usage(const char *progname)
{
    fprintf(stderr, "usage: %s [options] <text>\n", progname);
    fprintf(stderr, "Takes text and scrolls it with speed -s\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "\t-s <speed>        : Approximate letters per second. "
            "(Zero for no scrolling)\n"
            "\t-l <loop-count>   : Number of loops through the text. "
            "-1 for endless (default)\n"
            "\t-f <font-file>    : Use given font.\n"
            "\t-x <x-origin>     : X-Origin of displaying text (Default: 0)\n"
            "\t-y <y-origin>     : Y-Origin of displaying text (Default: 0)\n"
            "\t-t <track=spacing>: Spacing pixels between letters (Default: 0)\n"
            "\n"
            "\t-C <r,g,b>        : Text-Color. Default 255,255,0\n"
            "\t-B <r,g,b>        : Background-Color. Default 0,0,0\n"
            "\n");
    rgb_matrix::PrintMatrixFlags(stderr);
    return 1;
}

void *pthread_task_list_keypress(void *)
{
    static bool is_terminal = isatty(STDIN_FILENO);

    struct termios old;
    if (is_terminal)
    {
        if (tcgetattr(0, &old) < 0)
            perror("tcsetattr()");

        // Set to unbuffered mode
        struct termios no_echo = old;
        no_echo.c_lflag &= ~ICANON;
        no_echo.c_lflag &= ~ECHO;
        no_echo.c_cc[VMIN] = 0;
        no_echo.c_cc[VTIME] = 1;
        if (tcsetattr(0, TCSANOW, &no_echo) < 0)
            perror("tcsetattr ICANON");
    }

    while (!interrupt_received)
    {
        char buf = 0;
        if (read(STDIN_FILENO, &buf, 1) < 0)
        {
            perror("read()");
        }
        else
        {
            last_char.store(buf);
        }
    }

    if (is_terminal)
    {
        // Back to original terminal settings.
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
            perror("tcsetattr ~ICANON");
    }
    return NULL;
}

static bool parseColor(Color *c, const char *str)
{
    return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

Pipe generatePipe(int rows, int cols)
{
    int pipe_hole_size = (rand() % 10) + 10; // TODO: base this off screen size
    int pipe_hole_top = (rand() % (rows - pipe_hole_size));
    int pipe_hole_bottom = pipe_hole_top + pipe_hole_size;
    return Pipe(cols, pipe_hole_bottom, pipe_hole_top);
}

struct player_info{
    string name;
    int score;
};

std::istream& operator>>(std::istream& is, player_info& p){
    is >> p.name;
    is >> p.score;
    return is;
}

vector<player_info> readHighScores(const string path){
    vector<player_info> highScores;
    std::ifstream in_file{path};
    player_info player;
    while(in_file){
        in_file >> player;
        highScores.push_back(player);
    }
    return highScores;
}

int main(int argc, char *argv[])
{
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;
	matrix_options.hardware_mapping = HW_ID;
	matrix_options.rows = LED_MATRIX_HEIGHT;
	matrix_options.cols = LED_MATRIX_WIDTH;
	matrix_options.chain_length = BOSS_WIDTH;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                           &matrix_options, &runtime_opt))
    {
        return usage(argv[0]);
    }

    Color bird_color(255, 255, 0);
    Color sky_color(0, 255, 255);
    Color pipe_color(0, 153, 0);
    Color dirt_color(102, 102, 0);
    Color grass_color(0, 102, 0);

    int delay_speed_usec = 80000;
    int pipe_distance = 15;
    float jump_velocity = 2;
    float acceleration = -1;
    const char *big_font_file = "../fonts/spleen-16x32.bdf";
    const char *small_font_file = "../fonts/tom-thumb.bdf";

    const char *highscores_file = "flappy-bird-highscores.txt";

    int opt;
    while ((opt = getopt(argc, argv, "C:c:p:d:v:a:h:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (!parseColor(&pipe_color, optarg))
            {
                fprintf(stderr, "Invalid color spec: %s\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'C':
            if (!parseColor(&bird_color, optarg))
            {
                fprintf(stderr, "Invalid color spec: %s\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'c':
            if (!parseColor(&sky_color, optarg))
            {
                fprintf(stderr, "Invalid color spec: %s\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'd':
            delay_speed_usec = std::max(1000, atoi(optarg) * 1000);
            break;
        case 'v':
            jump_velocity = atof(optarg);
            break;
        case 'a':
            acceleration = atof(optarg);
            break;
        case 'h':
            pipe_distance = atoi(optarg);
            break;
        default:
            return usage(argv[0]);
        }
    }

    // Seed random number generator with (hopefully) unique seed
    srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

    // Load font
    rgb_matrix::Font big_font, small_font;
    bool big_font_loaded = big_font.LoadFont(big_font_file);
    bool small_font_loaded = small_font.LoadFont(small_font_file);

    float velocity = jump_velocity;

    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL)
        return 1;

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    printf("CTRL-C for exit.\n");

    // Create canvases to be used with led_matrix_swap_on_vsync
    FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
    matrix->CreateFrameCanvas();

    // create thread to listen for key presses
    pthread_t thread;
    pthread_create(&thread, NULL, pthread_task_list_keypress, NULL);

    const int screen_width = matrix_options.cols * matrix_options.chain_length;
    const int screen_height = matrix_options.rows;

    int x = screen_width / 2;
    int y = screen_height / 2;

    const int pipe_width = 3;
    const int bird_size = pipe_width;

    const int ground_height = 3;
    const int ground_level = screen_height - ground_height;

    vector<Pipe> pipes;
    int turns = 0;

    bool lost = false;
    while (!interrupt_received) // Should probably re-arrange so all calculations happen first, would ease handling pipe crashes
    {
        // Generate pipe
        if ((turns % pipe_distance) == 0)
        {
            Pipe new_pipe = generatePipe(ground_level, screen_width);
            pipes.push_back(new_pipe);
        }

        // Move shift all pipes to the left
        for (Pipe &p : pipes)
        {
            --(p.col);
        }

        // Remove pipes that have already passed the screen, currently can only be the case for the left-most one
        if (pipes.front().col < 0)
        {
            // Removes first element in the vector
            pipes.erase(pipes.begin());
        }

        // Fill in background
        offscreen_canvas->Fill(sky_color.r, sky_color.g, sky_color.b);

        // Draw ground
        DrawRectangle(offscreen_canvas, 0, ground_level, screen_width - 1, ground_level, grass_color);
        DrawRectangle(offscreen_canvas, 0, ground_level + 1, screen_width - 1, ground_level + ground_height - 1, dirt_color);

        // Draw pipes
        for (Pipe p : pipes)
        {
            DrawRectangle(offscreen_canvas, p.col, 0, p.col + pipe_width - 1, p.high, pipe_color);               // top part
            DrawRectangle(offscreen_canvas, p.col, p.low, p.col + pipe_width - 1, ground_level - 1, pipe_color); // bottom part
        }

        // Check if user sent input
        const char c = last_char.exchange('\0');

        // Calculate bird position and velocity
        if (c == ' ')
        {
            velocity = jump_velocity;
        }
        y -= static_cast<int>(velocity + 0.5);
        velocity += acceleration;

        // Check if bird crashed into anything
        if (y < 0)
        { // crashed into ceiling
            y = 0;
            velocity = 0;
        }
        else if (y + (bird_size - 1) >= ground_level)
        { // crashed into ground, die.  TODO do the same for pipe
            y = ground_level - (bird_size - 1) - 1;
            velocity = 0;
            lost = true;
        }
        for (Pipe pipe : pipes)
        {
            // bird crashed into pipe
            if((x >= pipe.col && x - (bird_size - 1) <= pipe.col + (pipe_width - 1)) && (y <= pipe.high || y + (bird_size - 1) >= pipe.low)){
                lost = true;
                // bird crashed into side of pipe
                if(x == pipe.col){
                    x--;
                // bird crashed into bottom of top pipe
                } else if(y <= pipe.high){
                    y = pipe.high + 1;
                // bird crashed into top of bottom pipe
                } else if(y + (bird_size - 1) >= pipe.low){
                    y = pipe.low - bird_size;
                } else{
                    printf("Unexpected crash, please debug.\n");
                }
            }
        }

        // Draw bird
        DrawRectangle(offscreen_canvas, x, y, x - (bird_size - 1), y + (bird_size - 1), bird_color);

        // string for printing score
        std::stringstream text;
        text << "SCORE: " << turns;
        const char *score_string = text.str().c_str();

        if (lost)
        {
            // std::string text;
            interrupt_received = true;
            if (big_font_loaded)
            {
                rgb_matrix::DrawText(offscreen_canvas, big_font,
                                     1, big_font.baseline(),
                                     Color(0, 0, 0), nullptr,
                                     score_string);
            }
            else
            {
                printf("%s\n", score_string);
            }
        }
        else
        {
            if (small_font_loaded)
            {
                int y_level = small_font.baseline()+1;
                rgb_matrix::DrawText(offscreen_canvas, small_font,
                                     1, y_level,
                                     Color(0, 0, 0), nullptr,
                                     score_string);
                vector<player_info> highScores = readHighScores(highscores_file);
                if(highScores.size() > 3){
                    highScores.resize(3);
                }
                for(player_info player : highScores){
                    std::stringstream text;
                    text << player.name << ": " << player.score;
                    y_level += small_font.height() + 1;
                    rgb_matrix::DrawText(offscreen_canvas, small_font,
                                     1, y_level,
                                     Color(0, 0, 0), nullptr,
                                     text.str().c_str());
                }
            }
            else
            {
                printf("%s\n", score_string);
            }
        }

        // Swap the offscreen_canvas with canvas on vsync, avoids flickering
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);

        if (lost)
        {
            usleep(2000000); // don't hardcode
        }

        ++turns;

        usleep(delay_speed_usec);
    }

    // pthread_cancel(thread);
    pthread_join(thread, NULL);

    // Finished. Shut down the RGB matrix.
    delete matrix;

    return 0;
}
