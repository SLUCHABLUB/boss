#include "led-matrix.h"
#include "graphics.h"
#include "common.h"

#include <string>
#include <algorithm>

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

using namespace rgb_matrix;

int rows = BOSS_MATRIX_HEIGHT;
int cols = BOSS_MATRIX_WIDTH;
int step_delay = 1000;

int framerate_fraction = 50;

Color foreground_color;
Color background_color;

bool **matrix; // create array ROWS x COLS matrix in main
// bool matrix = [32][192];    //doesn't fit in the stack

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
    interrupt_received = true;
}

static int usage(const char *progname)
{
    fprintf(stderr, "usage: %s [options]\n", progname);
    fprintf(stderr, "Visualize sorting algorithms.\n");
    fprintf(stderr, "Options:\n\n");
    fprintf(stderr,
            "\t-C <r,g,b>                : Color of bars\n"
            "\t-c <r,g,b>                : Color of background\n\n"
            "\t-d <delay>                : Delay between sorting steps\n\n"
            "\t-s <sorting-algorithm>    : Sorting algorithm to use (default to cocktail)\n");
    rgb_matrix::PrintMatrixFlags(stderr);
    return 1;
}

void initMatrix()
{
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            matrix[r][c] = false;
        }
    }
}

FrameCanvas *drawArray(int data[], int size, RGBMatrix *canvas, FrameCanvas *offscreen_canvas){
    offscreen_canvas->Fill(background_color.r, background_color.g, background_color.b);
    for(int c = 0; c < cols; c++){
        //printf("%d ", data[c]);
        //DrawLine(offscreen_canvas, c, rows-data[c], c, rows-1, foreground_color);
        for (int r = rows - 1; r >= rows - data[c]; r--) // changed r to start at ROWS-1, since matrix[ROWS] doesn't exist
        {
            //printf("Setting pixel: %d,%d\n", c, r);
            offscreen_canvas->SetPixel(c, r, foreground_color.r, foreground_color.g, foreground_color.b);
        }
    }
    return canvas->SwapOnVSync(offscreen_canvas, framerate_fraction);
}

static bool parseColor(Color *c, const char *str)
{
    return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

void swap(int &a, int &b)
{
    int tmp = a;
    a = b;
    b = tmp;
}

void insertionSort(int array[], int size, RGBMatrix *canvas, FrameCanvas *offscreen_canvas)
{
    for (int step = 1; step < size; step++)
    {
        if (interrupt_received)
        {
            return;
        }
        int key = array[step];
        int j = step - 1;

        // Compare key with each element on the left of it until an element smaller than
        // it is found.
        // For descending order, change key<array[j] to key>array[j].
        while (key < array[j] && j >= 0)
        {
            array[j + 1] = array[j];
            --j;
        }
        array[j + 1] = key;
        offscreen_canvas = drawArray(array, size, canvas, offscreen_canvas);
    }
}

void cocktailSort(int array[], int n, RGBMatrix *canvas, FrameCanvas *offscreen_canvas)
{
    bool swapped = true;
    int start = 0;
    int end = n - 1;

    while (swapped)
    {
        if (interrupt_received)
        {
            return;
        }
        swapped = false;

        for (int i = start; i < end; ++i)
        {
            if (array[i] > array[i + 1])
            {
                swap(array[i], array[i + 1]);
                swapped = true;
            }
        }

        if (!swapped)
        {
            break;
        }

        swapped = false;
        --end;

        for (int i = end - 1; i >= start; --i)
        {
            if (array[i] > array[i + 1])
            {
                swap(array[i], array[i + 1]);
                swapped = true;
            }
        }
        offscreen_canvas = drawArray(array, n, canvas, offscreen_canvas);

        ++start;
    }
}

int main(int argc, char *argv[])
{
    //no idea why default options don't work
    RGBMatrix::Options matrix_options;
    matrix_options.hardware_mapping = HW_ID;
    matrix_options.rows = LED_MATRIX_HEIGHT;
    matrix_options.cols = LED_MATRIX_WIDTH;
    matrix_options.chain_length = BOSS_WIDTH;
    rgb_matrix::RuntimeOptions runtime_opt;
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                           &matrix_options, &runtime_opt))
    {
        return usage(argv[0]);
    }

    foreground_color = Color(74, 46, 102);
    background_color = Color(0, 0, 0);

    std::string sorting_algorithm{"cocktail"};

    int opt;
    while ((opt = getopt(argc, argv, "C:c:s:d:")) != -1)
    {
        switch (opt)
        {
        case 's':
            sorting_algorithm = optarg;
            break;
        case 'C':
            if (!parseColor(&foreground_color, optarg))
            {
                fprintf(stderr, "Invalid color spec: %s\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'c':
            if (!parseColor(&background_color, optarg))
            {
                fprintf(stderr, "Invalid color spec: %s\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'd':
            framerate_fraction = atoi(optarg);
            //step_delay = std::max(1000, atoi(optarg) * 1000);
            break;
        default:
            return usage(argv[0]);
        }
    }

    RGBMatrix *canvas = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (canvas == NULL)
        return 1;

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Create array new canvas to be used with led_matrix_swap_on_vsync
    FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

    matrix = new bool *[rows];
    for (int r = 0; r < rows; r++)
    {
        matrix[r] = new bool[cols];
    }

    int data[cols];
    for (int i = 0; i < cols; i++)
    {
        int r = (rand() % rows) + 1;
        data[i] = r;
    }
    int size = sizeof(data) / sizeof(data[0]);
    initMatrix();

    offscreen_canvas = drawArray(data, size, canvas, offscreen_canvas);


    // matrixify(data, size);
    // drawMatrix(canvas, offscreen_canvas);

    const char *c_string = sorting_algorithm.c_str();

    if (strcmp(c_string, "cocktail") == 0)
    {
        cocktailSort(data, size, canvas, offscreen_canvas);
    }
    else if (strcmp(c_string, "insertion") == 0)
    {
        insertionSort(data, size, canvas, offscreen_canvas);
    }
    else
    {
        fprintf(stderr, "Invalid sorting algorithm: %s\n", c_string);
    }

    for (int r = 0; r < rows; r++)
    {
        delete[] matrix[r];
    }

    delete[] matrix;
    delete canvas;

    return 0;
}
