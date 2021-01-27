
#ifndef _SCENE_H
#define _SCENE_H


#include "glut.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include "Input.h"
#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <AMP.h>
#include <AMP_math.h>
#include "amp_short_vectors.h"
#include <amp.h>
#include <queue>
#include <concurrent_queue.h>
#include "Farm.h"
#include "CPUMAndelbrotTask.h"
#include "SOIL.h"
#include <vector>

using namespace concurrency::graphics;
using namespace concurrency;
using decimal = double;


class Scene{
	
public:
	Scene(Input *in);
	// Main render function
	void render();
	// function to initialize values for first Mandelbrot set calculations
	void CalculateValues();
	// function to break down Mandelbrot set to a number of taks and put on farm queue
	void TaskDecompostion(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_,decimal increment, decimal start_row, int rowsPerTask);
	// function to run the chosen mandelbrot
	void PartioningMandelbrot(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_, decimal increment, decimal start_row, int rowsPerTask);
	// function that writes the image to a tga file
	void WriteTga(const char * filename);
	// Handle input function that receives delta time from parent.
	void handleInput(float dt);
	// Update function receives delta time from parent (used for frame independent updating).
	void update(float dt);
	// Resizes the OpenGL output based on new window size.
	void resize(int w, int h);
	// function to run the specified test 100 times, and write results to a csv file
	void PerformanceTest(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_, decimal increment, decimal start_row, int rowsPerTask);

protected:
	// function to print out welcome message to user and ask what the user would the application to do
	void UserSelection();

	// functions for carrying out the Mandelbrot set on the CPU
	void PopulateCPUFarm();
	void ComputeMandelbrot(double left, double right, double top, double bottom, double yStart, double yEnd);
	void CPUMandelbrotForkJoin();

	// functions to provide information on the harware the application is being run on
	void report_accelerator(const accelerator a);
	void list_accelerators();
	void query_AMP_support();


	// Renders text (x, y positions, RGB colour of text, string of text to be rendered)
	void displayText(float x, float y, float r, float g, float b, char* string);
	
	// A function to collate all text output in a single location
	void renderTextOutput();
	void calculateFPS();
	

	// For access to user input.
	Input* input;
		
	// For Window and frustum calculation.
	int width, height;
	float fov, nearPlane, farPlane;

	// For FPS counter and mouse coordinate output.
	int frame = 0, time, timebase = 0;
	char fps[40];
	char mouseText[40];
	int WIDTH = 640;
	int HEIGHT = 480;

	// values used for the Task decomposition
	decimal totalTop;
	decimal totalBottom;
	decimal range;

	decimal totalLeft;
	decimal totalRight;
	decimal widthRange;

	int totalTasks;
	int rows;
	int start_row;
	int rowsPerTask;

	decimal right_;
	decimal left_;
	decimal incrementWidth;

	decimal increment;
	decimal top_;
	decimal bottom_;

	// values used for calculting the movement based on input for the Mandelbrot set
	double scaler, scaleAmount, panX, panY, panChangeX, panChangeY;
	int counter;
	int iteration;
	//bools to check the state of the Mandelbrot Set
	bool isDrawing, isZooming, isZoomingOut, isTesting, isExploring;

	// bools to specify which test to carry out
	bool testingSingleGPU, testingMultiGPU, testingCPUFork, testingCPUFarm;

	// 2d array for storing the Mandebrot set
	uint32_t image[480][640];

	// char to read user input
	char userInput;

	// instances of classes to  carry out the Mandelbrot Calculations
	Farm farm;
	MandelbrotTask * mandel; // creates a Mnadelbrot on the GPU
	CPUMandelBrotTask * CPUMandelBrot; // creates a Mandebrot set on the CPU

};

#endif