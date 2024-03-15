#include "led-matrix.h"
#include "graphics.h"
#include "common.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

using namespace rgb_matrix;

//#define DEBUG

int height;
int width;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
	interrupt_received = true;
}

static int usage(const char *progname)
{
	fprintf(stderr, "usage: %s [options]\n", progname);
	fprintf(stderr, "Visualize the mandelbrot set.\n");
	fprintf(stderr, "Options:\n\n");
	fprintf(stderr,
			"\t-i <iterations>                : Color of bars\n"
			"\t-t <color_threshold>                : Color of background\n\n"
			"\t-d <delay>                : Delay between sorting steps\n\n"
			"\t-s <sorting-algorithm>    : Sorting algorithm to use (default to cocktail)\n");
	rgb_matrix::PrintMatrixFlags(stderr);
	return 1;
}

int mandelbrot(double real, double imag, int start)
{
	int limit = 100;
	double zReal = real;
	double zImag = imag;

	for (int i = start; i < limit; ++i)
	{
		double r2 = zReal * zReal;
		double i2 = zImag * zImag;

		if (r2 + i2 > 4.0)
			return i;

		zImag = 2.0 * zReal * zImag + imag;
		zReal = r2 - i2 + real;
	}
	return limit;
}

void draw(FrameCanvas *offscreen_canvas, double x_start, double x_fin, double y_start, double y_fin)
{
	double dx = (x_fin - x_start) / (width - 1);
	double dy = (y_fin - y_start) / (height - 1);

	Color black{0, 0, 0};
	Color red{255, 0, 0};
	Color l_red{255, 127, 127};
	Color green{0, 255, 0};
	Color l_green{127, 255, 127};
	Color orange{255, 165, 0};
	Color yellow{255, 255, 0};
	Color blue{0, 0, 255};
	Color l_blue{127, 127, 255};
	Color magenta{255, 0, 255};
	Color l_magenta{255, 127, 255};
	Color cyan{0, 255, 255};
	Color l_cyan{127, 255, 255};
	Color gray{220, 220, 220};
	Color white{255, 255, 255};

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{

			double x = x_start + j * dx; // current real value
			double y = y_fin - i * dy;	 // current imaginary value

			int value = mandelbrot(x, y, 0);

			Color c;

			if (value == 100)
			{
				c = black;
			}
			else if (value >= 90)
			{
				c = red;
			}
			else if (value >= 70)
			{
				c = l_red;
			}
			else if (value >= 50)
			{
				c = orange;
			}
			else if (value >= 30)
			{
				c = yellow;
			}
			else if (value >= 20)
			{
				c = l_green;
			}
			else if (value >= 10)
			{
				c = green;
			}
			else if (value >= 5)
			{
				c = l_cyan;
			}
			else if (value >= 4)
			{
				c = cyan;
			}
			else if (value >= 3)
			{
				c = l_blue;
			}
			else if (value >= 2)
			{
				c = blue;
			}
			else if (value >= 1)
			{
				c = magenta;
			}
			else
			{
				c = l_magenta;
			}

			offscreen_canvas->SetPixel(j, i, c.r, c.g, c.b);
		}
	}
}

void draw_deep(FrameCanvas *offscreen_canvas, double x_start, double x_fin, double y_start, double y_fin)
{
	double dx = (x_fin - x_start) / (width - 1);
	double dy = (y_fin - y_start) / (height - 1);

	Color black{0, 0, 0};
	Color red{255, 0, 0};
	Color l_red{255, 127, 127};
	Color green{0, 255, 0};
	Color l_green{127, 255, 127};
	Color orange{255, 165, 0};
	Color yellow{255, 255, 0};
	Color blue{0, 0, 255};
	Color l_blue{127, 127, 255};
	Color magenta{255, 0, 255};
	Color l_magenta{255, 127, 255};
	Color cyan{0, 255, 255};
	Color l_cyan{127, 255, 255};
	Color gray{220, 220, 220};
	Color white{255, 255, 255};

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			double x = x_start + j * dx; // current real value
			double y = y_fin - i * dy;	 // current imaginary value

			int value = mandelbrot(x, y, 0);
			Color c;

			if (value == 100)
			{
				c = black;
			}
			else if (value >= 99)
			{
				c = red;
			}
			else if (value >= 98)
			{
				c = l_red;
			}
			else if (value >= 96)
			{
				c = orange;
			}
			else if (value >= 94)
			{
				c = yellow;
			}
			else if (value >= 92)
			{
				c = l_green;
			}
			else if (value >= 90)
			{
				c = green;
			}
			else if (value >= 85)
			{
				c = l_cyan;
			}
			else if (value >= 80)
			{
				c = cyan;
			}
			else if (value >= 75)
			{
				c = l_blue;
			}
			else if (value >= 70)
			{
				c = blue;
			}
			else if (value >= 60)
			{
				c = magenta;
			}
			else
			{
				c = l_magenta;
			}

			offscreen_canvas->SetPixel(j, i, c.r, c.g, c.b);
		}
	}
}

int main(int argc, char **argv)
{
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

	int delay = 16;
	int iter = 250;
	int color_threshold = 169;
	double zoom_speed = 1.11;

	// Try -e/7 -e/20i
	double center_x = -1.04082816210546;
	double center_y = 0.346341718848392;

	int opt;
	while ((opt = getopt(argc, argv, "d:i:t:z:x:y:")) != -1)
	{
		switch (opt)
		{
		case 'd':
			delay = std::max(0, atoi(optarg));
			break;
		case 'i':
			iter = std::max(0, atoi(optarg));
			break;
		case 't':
			color_threshold = std::max(0, atoi(optarg));
			break;
		case 'z':
			zoom_speed = atof(optarg);
			break;
		case 'x':
			center_x = atof(optarg);
			break;
		case 'y':
			center_y = atof(optarg);
			break;
		default:
			return usage(argv[0]);
		}
	}

	RGBMatrix *canvas = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
	if (canvas == nullptr)
		return -1;

	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	// Create array new canvas to be used with led_matrix_swap_on_vsync
	FrameCanvas *offscreen_canvas = canvas->CreateFrameCanvas();

	width = BOSS_MATRIX_WIDTH;
	height = BOSS_MATRIX_HEIGHT;

	double factor = 1.0;

	#ifdef DEBUG
	std::chrono::nanoseconds ms_log[iter];
	#endif

	auto increment = std::chrono::milliseconds(delay);
	auto delay_until = std::chrono::steady_clock::now() + increment;
	for (int i = 0; i < iter; i++)
	{
		if (interrupt_received)
		{
			break;
		}

		// adjust aspect ratio?, slow down zoom in?
		double x_start = center_x - 1.5 * factor;
		double x_fin = center_x + 1.5 * factor;
		double y_start = center_y - factor;
		double y_fin = center_y + factor;
		factor /= zoom_speed;

		if (i < color_threshold)
		{
			draw(offscreen_canvas, x_start, x_fin, y_start, y_fin);
		}
		else
		{
			draw_deep(offscreen_canvas, x_start, x_fin, y_start, y_fin);
		}
		offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas, 30);
		//sleep remaining time
		#ifdef DEBUG
		std::chrono::nanoseconds ms_diff = delay_until - std::chrono::steady_clock::now();
		ms_log[i] = ms_diff;
		#endif
		//std::this_thread::sleep_until(delay_until);
		delay_until += increment;
	}

	delete canvas;

	#ifdef DEBUG
	for (int i = 0; i < iter; i++){
		std::cout << i << ": " << ms_log[i].count() << std::endl;
	}
	#endif

	return 0;
}
