#include "choochoo.h"

#include "common.h"
#include "led-matrix.h"
#include "graphics.h"

#include <unistd.h>
#include <string.h>

constexpr int LED_MATRIX_HEIGHT = 32;
constexpr int LED_MATRIX_WIDTH = 64;
constexpr int BOSS_WIDTH = 3;
constexpr int BOSS_MATRIX_WIDTH = BOSS_WIDTH * LED_MATRIX_WIDTH;

constexpr int ROWS = LED_MATRIX_HEIGHT;
constexpr int COLS = BOSS_MATRIX_WIDTH * 2;

constexpr int TRAIN_OFFSET = BOSS_MATRIX_WIDTH;
constexpr int TRAIN_HEIGHT = 19;

constexpr int SMOKE_OFFSET = 8;
constexpr int SMOKE_HEIGHT = 5;

constexpr int WHEEL_OFFSET = 1;
constexpr int WHEEL_HEIGHT = 5;

constexpr int ANIM_LENGTH = COLS;

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
		memset(matrix[i], 0, sizeof(int) * COLS);
	}

	std::vector<const char*> il {
			LOK18,
			"17", "16", "15", "14", "13", // padding for wheel section
			LOK12, LOK11, LOK10, LOK9,
			LOK8, LOK7, LOK6, LOK5,
			LOK4, LOK3, LOK2, LOK1, LOK0};

	int train_len = strlen(LOK0);

	for (int i = 0; i < TRAIN_HEIGHT; i++)
	{
		for (int j = 0; j < train_len; j++)
		{
			matrix[ROWS - i - 1][TRAIN_OFFSET + j] = get_matrix_value(il[i][j]);
		}
		if (i == 0)
		{
			i += WHEEL_HEIGHT; // skip wheel section
		}
	}

	return matrix;
}

const char* get_wheel_17(int frame)
{
	return std::vector<const char*>{LOK17_0, LOK17_1, LOK17_2, LOK17_3}[frame];
}

const char* get_wheel_16(int frame)
{
	return std::vector<const char*>{LOK16_0, LOK16_1, LOK16_2, LOK16_3}[frame];
}

const char* get_wheel_15(int frame)
{
	return std::vector<const char*>{LOK15_0, LOK15_1, LOK15_2, LOK15_3}[frame];
}

const char* get_wheel_14(int frame)
{
	return std::vector<const char*>{LOK14_0, LOK14_1, LOK14_2, LOK14_3}[frame];
}

const char* get_wheel_13(int frame)
{
	return std::vector<const char*>{LOK13_0, LOK13_1, LOK13_2, LOK13_3}[frame];
}

int** calc_wheel_matrix(int frame)
{
	int** matrix = new int*[ROWS];
	for (size_t i = 0; i < ROWS; i++)
	{
		matrix[i] = new int[COLS];
		memset(matrix[i], 0, sizeof(int) * COLS);
	}

	std::vector<const char*> il {
		get_wheel_17(frame),
		get_wheel_16(frame),
		get_wheel_15(frame),
		get_wheel_14(frame),
		get_wheel_13(frame)
	};

	int train_len = strlen(LOK0);

	for (int i = WHEEL_OFFSET; i <= WHEEL_HEIGHT; i++)
	{
		for (int j = 0; j < train_len; j++)
		{
			matrix[ROWS - i - 1][TRAIN_OFFSET + j] = get_matrix_value(il[i - WHEEL_OFFSET][j]);
		}
	}

	return matrix;
}

const char* get_smoke_0(int frame)
{
	return std::vector<const char*>{SMOKE0_0, SMOKE0_1}[frame];
}

const char* get_smoke_1(int frame)
{
	return std::vector<const char*>{SMOKE1_0, SMOKE1_1}[frame];
}

const char* get_smoke_2(int frame)
{
	return std::vector<const char*>{SMOKE2_0, SMOKE2_1}[frame];
}

const char* get_smoke_3(int frame)
{
	return std::vector<const char*>{SMOKE3_0, SMOKE3_1}[frame];
}

const char* get_smoke_4(int frame)
{
	return std::vector<const char*>{SMOKE4_0, SMOKE4_1}[frame];
}

int** calc_smoke_matrix(int frame)
{
	int** matrix = new int*[ROWS];
	for (size_t i = 0; i < ROWS; i++)
	{
		matrix[i] = new int[COLS];
		memset(matrix[i], 0, sizeof(int) * COLS);
	}

	std::vector<const char*> il {
		get_smoke_0(frame),
		get_smoke_1(frame),
		get_smoke_2(frame),
		get_smoke_3(frame),
		get_smoke_4(frame)
	};

	int smoke_len = strlen(SMOKE0_0);

	for (int i = 0; i < SMOKE_HEIGHT; i++)
	{
		for (int j = 0; j < smoke_len; j++)
		{
			matrix[i + SMOKE_OFFSET][TRAIN_OFFSET + j] = get_matrix_value(il[i][j]);
		}
	}

	return matrix;
}

rgb_matrix::FrameCanvas* draw_frame(rgb_matrix::RGBMatrix* rgb_mat, rgb_matrix::FrameCanvas *offscreen_canvas, int** lok_static,  int** wheels, int** smoke)
{
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < BOSS_MATRIX_WIDTH; j++)
		{
			if (lok_static[i][j] == 1 || wheels[i][j] == 1 || smoke[i][j] == 1)
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
	offscreen_canvas = draw_frame(rgb_mat, offscreen_canvas, lok_static, wheels, smoke);
	return rgb_mat->SwapOnVSync(offscreen_canvas);
}


int main(int argc, char** argv)
{
    rgb_matrix::RGBMatrix::Options matrix_options;
	rgb_matrix::RuntimeOptions runtime_opt;
	matrix_options.hardware_mapping = HW_ID;
	matrix_options.rows = LED_MATRIX_HEIGHT;
	matrix_options.cols = LED_MATRIX_WIDTH;
	matrix_options.chain_length = BOSS_WIDTH;
	if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
										   &matrix_options, &runtime_opt))
	{
		//return usage(argv[0]);
		return -1;
	}

	auto* rgb_mat = rgb_matrix::RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
	if (rgb_mat == nullptr)
	{
		return -1;
	}

	auto* offscreen_canvas = rgb_mat->CreateFrameCanvas();

	while(1);

	int wheel_frame = 0;
	int smoke_frame = 0;

	auto* lok_static = calc_lok_matrix();
	auto* smoke_0 = calc_smoke_matrix(0);
	auto* smoke_1 = calc_smoke_matrix(1);
	auto* wheels_0 = calc_wheel_matrix(0);
	auto* wheels_1 = calc_wheel_matrix(1);
	auto* wheels_2 = calc_wheel_matrix(2);
	auto* wheels_3 = calc_wheel_matrix(3);

	auto* smoke = &smoke_0;
	auto* wheels = &wheels_0;

	std::vector<int**> iw {wheels_0, wheels_1, wheels_2, wheels_3};

	for (size_t i = 0; i < ANIM_LENGTH - 70; i++) // magic offset
	{
		// animate
		smoke_frame = i % 6 > 2 ? 1 : 0; // animate smoke every third frame
		wheel_frame = i % 4;

		smoke = smoke_frame == 1 ? &smoke_1 : &smoke_0;
		wheels = &iw[wheel_frame];

		// draw train
		offscreen_canvas->Fill(0, 0, 0);
		offscreen_canvas = draw_lok(rgb_mat, offscreen_canvas, lok_static, *wheels, *smoke);

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
		usleep(1000 * 40); // Magic number go brrr
	}

    return 0;
}