#pragma once
#include "CPUTask.h"
#include <string>
#include <fstream>
#include <iostream>
#include <complex>

using decimalValues = float;
using std::ofstream;

// strtuct used to pass a two demensional array as a parameter
struct TwoDemensional {
	static const int x = 640;
	static const int y = 480;
	int array[y][x];
};

class CPUMandelBrotTask : public CPUTask
{
public:
	CPUMandelBrotTask(int id_, double left_, double right_, double top_, double bottom_, double yStart_, double yEnd_, TwoDemensional* image_)
		: id(id_), left(left_), right(right_), top(top_), bottom(bottom_), yStart(yStart_), yEnd(yEnd_), imageTD(image_)
	{
	}

	void run();
private:
	// variable used fro passing values in the constructor
	int id;
	double left, right;
	double top, bottom;
	double yStart, yEnd;
	TwoDemensional* imageTD;

	// variables used for calculating mandelbrot
	int MAX_ITERATIONS = 500;
	const int WIDTH = 640;
	const int HEIGHT = 480;
};