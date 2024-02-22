#include "choochoo.h"

#include "led-matrix.h"
#include "graphics.h"

#include <string>
#include <algorithm>

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>



#define ROWS 32
#define COLS 3 * 64 * 2

#define ANIM_LENGTH COLS + 20

char* get_wheel_frame_17(int i)
{
	switch (i)
	{
	case 0:
		return LOK17_0;
	case 1:
		return LOK17_1;
	case 2:
		return LOK17_2;
	case 3:
		return LOK17_3;
	default:
		return "wtf";
	}
}

char* get_wheel_frame_16(int i)
{
	switch (i)
	{
	case 0:
		return LOK16_0;
	case 1:
		return LOK16_1;
	case 2:
		return LOK16_2;
	case 3:
		return LOK16_3;
	default:
		return "wtf";
	}
}

char* get_wheel_frame_15(int i)
{
	switch (i)
	{
	case 0:
		return LOK15_0;
	case 1:
		return LOK15_1;
	case 2:
		return LOK15_2;
	case 3:
		return LOK15_3;
	default:
		return "wtf";
	}
}

char* get_wheel_frame_14(int i)
{
	switch (i)
	{
	case 0:
		return LOK14_0;
	case 1:
		return LOK14_1;
	case 2:
		return LOK14_2;
	case 3:
		return LOK14_3;
	default:
		return "wtf";
	}
}

char* get_wheel_frame_13(int i)
{
	switch (i)
	{
	case 0:
		return LOK13_0;
	case 1:
		return LOK13_1;
	case 2:
		return LOK13_2;
	case 3:
		return LOK13_3;
	default:
		return "wtf";
	}
}

int get_matrix_value(char c)
{
	return c == '@' ? 1 : 0;
}

int** calc_lok_matrix()
{
	int** matrix = new int*[ROWS];
	for (size_t i = 0; i < ROWS; i++)
	{
		matrix[i] = new int[COLS];
		memset(matrix[i], 0, sizeof(bool) * COLS);
	}

	std::vector<char*> il {
			LOK18,"17","16","15","14","13", LOK12, LOK11, LOK10,
			LOK9, LOK8, LOK7, LOK6, LOK5,
			LOK4, LOK3, LOK2, LOK1, LOK0};

	int offset = 64 * 3;

	int train_len = strlen(LOK0);
	int train_hei = 19;

	for (size_t i = 0; i < train_hei; i++)
	{
		for (size_t j = 0; j < train_len; j++)
		{
			matrix[ROWS - i - 1][offset + j] = get_matrix_value(il[i][j]);
		}
		if (i == 0)
		{
			i += 5; // skip wheel section
		}
	}

	return matrix;
}

char* get_wheel_17(int frame)
{
	return std::vector<char*>{LOK17_0, LOK17_1, LOK17_2, LOK17_3}[frame];
}

char* get_wheel_16(int frame)
{
	return std::vector<char*>{LOK16_0, LOK16_1, LOK16_2, LOK16_3}[frame];
}

char* get_wheel_15(int frame)
{
	return std::vector<char*>{LOK15_0, LOK15_1, LOK15_2, LOK15_3}[frame];
}

char* get_wheel_14(int frame)
{
	return std::vector<char*>{LOK14_0, LOK14_1, LOK14_2, LOK14_3}[frame];
}

char* get_wheel_13(int frame)
{
	return std::vector<char*>{LOK13_0, LOK13_1, LOK13_2, LOK13_3}[frame];
}

int** calc_wheel_matrix(int frame)
{
	int** matrix = new int*[ROWS];
	for (size_t i = 0; i < ROWS; i++)
	{
		matrix[i] = new int[COLS];
		memset(matrix[i], 0, sizeof(bool) * COLS);
	}

	int offset = 64 * 3;

	int train_len = strlen(LOK0);
	int wheel_hei = 5;

	std::vector<char*> il {
		get_wheel_17(frame),
		get_wheel_16(frame),
		get_wheel_15(frame),
		get_wheel_14(frame),
		get_wheel_13(frame)
	};

	for (size_t i = 1; i <= wheel_hei; i++)
	{
		for (size_t j = 0; j < train_len; j++)
		{
			matrix[ROWS - i - 1][offset + j] = get_matrix_value(il[i - 1][j]);
		}
	}

	return matrix;
}

char* get_smoke_0(int frame)
{
	return std::vector<char*>{SMOKE0_0, SMOKE0_1}[frame];
}

char* get_smoke_1(int frame)
{
	return std::vector<char*>{SMOKE1_0, SMOKE1_1}[frame];
}

char* get_smoke_2(int frame)
{
	return std::vector<char*>{SMOKE2_0, SMOKE2_1}[frame];
}

char* get_smoke_3(int frame)
{
	return std::vector<char*>{SMOKE3_0, SMOKE3_1}[frame];
}

char* get_smoke_4(int frame)
{
	return std::vector<char*>{SMOKE4_0, SMOKE4_1}[frame];
}


int** calc_smoke_matrix(int frame)
{
	int** matrix = new int*[ROWS];
	for (size_t i = 0; i < ROWS; i++)
	{
		matrix[i] = new int[COLS];
		memset(matrix[i], 0, sizeof(bool) * COLS);
	}

	int offset = 64 * 3;

	int smoke_len = strlen(SMOKE0_0);
	int smoke_hei = 5;

	std::vector<char*> il {
		get_smoke_0(frame),
		get_smoke_1(frame),
		get_smoke_2(frame),
		get_smoke_3(frame),
		get_smoke_4(frame)
	};

	for (size_t i = 1; i < smoke_hei; i++)
	{
		for (size_t j = 0; j < smoke_len; j++)
		{
			matrix[i + 7][offset + j] = get_matrix_value(il[i - 1][j]);
		}
	}

	return matrix;
}

rgb_matrix::FrameCanvas* draw_frame(rgb_matrix::RGBMatrix* rgb_mat, rgb_matrix::FrameCanvas *offscreen_canvas, int** frame)
{
	for (size_t i = 0; i < ROWS; i++)
	{
		for (size_t j = 0; j < 64 * 3; j++)
		{
			if (frame[i][j] == 1)
			{
				offscreen_canvas->SetPixel(j, i, 255, 255, 255);
			}
		}

	}
	return offscreen_canvas;
}

rgb_matrix::FrameCanvas* draw_lok(rgb_matrix::RGBMatrix* rgb_mat, rgb_matrix::FrameCanvas *offscreen_canvas, int** lok_static, int** wheels, int** smoke)
{
	offscreen_canvas->Fill(0, 0, 0);

	offscreen_canvas = draw_frame(rgb_mat, offscreen_canvas, lok_static);
	offscreen_canvas = draw_frame(rgb_mat, offscreen_canvas, smoke);
	offscreen_canvas = draw_frame(rgb_mat, offscreen_canvas, wheels);

	return rgb_mat->SwapOnVSync(offscreen_canvas);
}


int main(int argc, char** argv)
{
    rgb_matrix::RGBMatrix::Options matrix_options;
	matrix_options.hardware_mapping = "adafruit-hat";
	matrix_options.rows = 32;
	matrix_options.cols = 64;
	matrix_options.chain_length = 3;
	rgb_matrix::RuntimeOptions runtime_opt;
	if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
										   &matrix_options, &runtime_opt))
	{
		//return usage(argv[0]);
		return -1;
	}

	rgb_matrix::Color fg(255,255,255);
	rgb_matrix::Color bg(0,0,0);

	rgb_matrix::RGBMatrix* rgb_mat = rgb_matrix::RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
	if (rgb_mat == nullptr)
	{
		return -1;
	}

	rgb_matrix::FrameCanvas* offscreen_canvas = rgb_mat->CreateFrameCanvas();

	int wheel_frame = 0;
	int smoke_frame = 0;

	int** lok_static = calc_lok_matrix();

	int** smoke_0 = calc_smoke_matrix(0);
	int** smoke_1 = calc_smoke_matrix(1);

	int** wheels_0 = calc_wheel_matrix(0);
	int** wheels_1 = calc_wheel_matrix(1);
	int** wheels_2 = calc_wheel_matrix(2);
	int** wheels_3 = calc_wheel_matrix(3);

	//int*** smoke = &smoke_0;
	int*** wheels = &wheels_0;

	std::vector<int**> iw {wheels_0, wheels_1, wheels_2, wheels_3};

	for (size_t i = 0; i < COLS - 70; i++)
	{
		// animate
		smoke_frame = i % 6 > 2 ? 1 : 0;
		wheel_frame = i % 4;

		//smoke = smoke_frame == 1 ? smoke_1 : smoke_0;
		wheels = &iw[wheel_frame];


		// draw train
		offscreen_canvas->Fill(0, 0, 0);
		offscreen_canvas = draw_lok(rgb_mat, offscreen_canvas, lok_static, *wheels, smoke_frame == 1 ? smoke_1 : smoke_0);


		// increment pointers
		for (size_t j = 0; j < ROWS; j++)
		{
			lok_static[j]++;
			wheels_0[j]++;
			wheels_1[j]++;
			wheels_2[j]++;
			wheels_3[j]++;
			smoke_0[j]++;
			smoke_1[j]++;
		}
		usleep(1000*40);
	}

    return 0;
}