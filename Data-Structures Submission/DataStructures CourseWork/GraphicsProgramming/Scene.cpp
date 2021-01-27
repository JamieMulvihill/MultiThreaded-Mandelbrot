#include "Scene.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <complex>


using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::ofstream;
typedef std::chrono::steady_clock the_clock;
using std::ofstream;
std::ofstream file_; // create a file to store results of test to a csv file

the_clock::time_point computedMandelbrotStart;

Scene::Scene(Input *in)
{
	// Store pointer for input class
	input = in;

	glEnable(GL_TEXTURE_2D);
	
	isExploring = false;
	isTesting = false;
	testingSingleGPU = false;
	testingMultiGPU = false;

	query_AMP_support();

	CalculateValues();

	UserSelection();

}
// function to break down Mandelbrot set to a number of taks and put on farm queue
void Scene::TaskDecompostion(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_, decimal increment, decimal start_row, int rowsPerTask) {

	for (int i = 0; i < totalTasks; i++) {
		TaskData* t = new TaskData(i);
		t->startRow = start_row;
		t->endRow = t->startRow + rowsPerTask;
		t->writeExt = extent<2>(rowsPerTask, 640);
		t->left = left_;
		t->right = right_;
		t->top = top_;
		t->bottom = bottom_;
		mandel = new MandelbrotTask(t, image[t->startRow]);
		farm.add_task(mandel);
		start_row += rowsPerTask;
		top_ += increment;
		bottom_ += increment;
	}

}

// function to run the chosen mandelbrot
void Scene::PartioningMandelbrot(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_, decimal increment, decimal start_row, int rowsPerTask) {

	// if the user has chosen to explore the Mandelbrot
	//Call the task decompisiton function and run the farm single GPU function as it has the best performance
	if (isExploring) {
		TaskDecompostion(totalTasks, left_, right_, top_, bottom_, increment, start_row, rowsPerTask);
		farm.SingleGPU(rowsPerTask);
	}

	// if the user has chosen performance testing, run the selected test, and srt the clock for the results calclations
	if (isTesting) {
		if (testingSingleGPU) {
			TaskDecompostion(totalTasks, left_, right_, top_, bottom_, increment, start_row, rowsPerTask);
			computedMandelbrotStart = the_clock::now();
			farm.SingleGPU(rowsPerTask);
		}
		else if (testingMultiGPU) {
			TaskDecompostion(totalTasks, left_, right_, top_, bottom_, increment, start_row, rowsPerTask);
			computedMandelbrotStart = the_clock::now();
			farm.MultiGPULoadBalanced(rowsPerTask);
		}
		else if (testingCPUFarm) {
			PopulateCPUFarm();
			computedMandelbrotStart = the_clock::now();
			farm.CPUFarmMandelbrot();
		}
		else if (testingCPUFork) {
			CPUMandelbrotForkJoin();
		}
	}
}

// draw the Mandelbrot set to a tga file
void Scene::WriteTga(const char *filename) {

	ofstream outfile(filename, ofstream::binary);

	uint8_t header[18] = {
		0, // no image ID
		0, // no colour map
		2, // uncompressed 24-bit image
		0, 0, 0, 0, 0, // empty colour map specification
		0, 0, // X origin
		0, 0, // Y origin
		WIDTH & 0xFF, (WIDTH >> 8) & 0xFF, // width
		HEIGHT & 0xFF, (HEIGHT >> 8) & 0xFF, // height
		24, // bits per pixel
		0, // image descriptor
	};
	outfile.write((const char *)header, 18);

	for (int y = 0; y < HEIGHT; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			uint8_t pixel[3] = {
				(image[y][x] >> 16) & 0xFF, // blue channel
				(image[y][x] >> 8) & 0xFF, // green channel
				image[y][x] & 0xFF, // red channel
			};
			outfile.write((const char *)pixel, 3);
		}
	}

	outfile.close();
	if (!outfile)
	{
		// An error has occurred at some point since we opened the file.
		std::cout << "Error writing to " << filename << std::endl;
		exit(1);
	}
}

// handles the user input to explore the mandelbrot set
void Scene::handleInput(float dt)
{
	// Handle user input
	if (input->isKeyDown('w')) {
		panY += (panChangeY * (scaler * iteration / 4));
		input->SetKeyUp('w');
	}
	if (input->isKeyDown('s')) {
		panY -= (panChangeY * (scaler * iteration / 4));
		input->SetKeyUp('s');
	}
	if (input->isKeyDown('a')) {
		panX -= (panChangeX * (scaler * iteration / 4));
		input->SetKeyUp('a');
	}
	if (input->isKeyDown('d')) {
		panX += (panChangeX * (scaler * iteration / 4));
		input->SetKeyUp('d');
	}

	if (input->isKeyDown('b')) {
		scaleAmount += 0.005;
		scaler /= scaleAmount;
		iteration++;
		input->SetKeyUp('b');
	}

	if (input->isKeyDown('v')) {
		scaleAmount -= 0.005;
		scaler *= scaleAmount;
		iteration--;
		input->SetKeyUp('v');
	}

	if (input->isKeyDown('c')) {
		WriteTga("output.tga");
	}
}

void Scene::update(float dt)
{

	handleInput(dt);

	// Calculate FPS for output
	calculateFPS();

	
	if (isTesting) {
		PerformanceTest(totalTasks, (left_ * scaler) + panX, (right_ * scaler) + panX, (top_ * scaler) + panY, (bottom_ * scaler) + panY, (increment* scaler), start_row, rowsPerTask);
	}

	else {
		PartioningMandelbrot(totalTasks, (left_ * scaler) + panX, (right_ * scaler) + panX, (top_ * scaler) + panY, (bottom_ * scaler) + panY, (increment* scaler), start_row, rowsPerTask);
	}
	
}

void Scene::render() {
	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset transformations
	glLoadIdentity();
	// Set the camera
	gluLookAt(0.0f, 0.0f, 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	// Render geometry/scene here -------------------------------------
	glPushMatrix();

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(-1.f, -0.75f, 1.f); //v0
		glTexCoord2f(0, 1);
		glVertex3f(-1.f, .75f, 1.f); //v1
		glTexCoord2f(1, 1);
		glVertex3f(1.f, .75f, 1.f); //v2
		glTexCoord2f(1, 0);
		glVertex3f(1.f, -.75f, 1.f); //v3
		glEnd();
	glPopMatrix();


	// End render geometry --------------------------------------

	// Render text, should be last object rendered.
	renderTextOutput();
	
	// Swap buffers, after all objects are rendered.
	glutSwapBuffers();
}

// function to initialize the values for the calculation of the first mandelbrot set
void Scene::CalculateValues( ){

	scaler = 1.0;
	scaleAmount = 1.01;

	panX = .2;
	panY = .2;

	panChangeX = panX / 60;
	panChangeY = panY / 60;

	iteration = 1;

	totalTop = -1.25;
	totalBottom = 1.25;
	range = abs(totalBottom) + abs(totalTop);

	totalTasks = 2; // varied the number for total tasks to carry out different test 2, 4, 8, 16, 32
	rows = 480;
	start_row = 0;
	rowsPerTask = int(rows / totalTasks);

	increment = range / totalTasks;
	top_ = -1.25;
	bottom_ = top_ + increment;

	left_ = -2.0;
	right_ = 1.0;
}

// Handles the resize of the window. If the window changes size the perspective matrix requires re-calculation to match new window size.
void Scene::resize(int w, int h) 
{
	width = w;
	height = h;
	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (h == 0)
		h = 1;

	float ratio = (float)w / (float)h;
	fov = 45.0f;
	nearPlane = 0.1f;
	farPlane = 100.0f;

	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);

	// Reset Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	// Set the correct perspective.
	gluPerspective(fov, ratio, nearPlane, farPlane);

	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}
// function to carry out the perfamnce tests, run s the desired test 100 times
void Scene::PerformanceTest(int totalTasks, decimal left_, decimal right_, decimal top_, decimal bottom_, decimal increment, decimal start_row, int rowsPerTask)
{
	ofstream my_file("MandelbrotPerformanceTest.csv");
	int  totalTests = 101;
		for (int j = 0; j < totalTests; j++) {
			PartioningMandelbrot(totalTasks, left_, right_, top_, bottom_, increment, start_row, rowsPerTask);
			the_clock::time_point computedMandelbrotEnd = the_clock::now();
			auto computeTime = duration_cast<microseconds>(computedMandelbrotEnd - computedMandelbrotStart).count();
			my_file << computeTime << "," << 0 << std::endl;
		}
	std::cout << "Tests Complete" << std::endl;
	isTesting = false;
}

// Calculates FPS
void Scene::calculateFPS()
{

	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);

	if (time - timebase > 1000) {
		sprintf_s(fps, "FPS: %4.2f", frame*1000.0 / (time - timebase));
		timebase = time;
		frame = 0;
	}
}

// function to print out a welcome message to the user and ask what task they want the application to run,
// if the user selects testing, asks which test they would like to carry out
void Scene::UserSelection()
{

	std::cout << "					*Welcome to Mandelbrot Explorer!*	" << std::endl;
	std::cout << "			Would you like to Perfromance test or Explore Mandelbrot?	" << std::endl << std::endl;
	std::cout << " Press 'p' for Performance Test! " << std::endl;
	std::cout << " Press 'e' to Explore! " << std::endl;
	std::cin >> userInput;
	std::cout << "You pressed '" << userInput << "'" << std::endl;
	if (userInput == 'e') {
		std::cout << "Happy Exploring" << std::endl;
		isExploring = true;
	}
	else if (userInput == 'p') {
		
		char testChoice;
		std::cout << " Which Test would you like to run? " << std::endl;
		std::cout << " Press 's' for Single GPU Partitioned Mandelbrot Test " << std::endl;
		std::cout << " Press 'l' for Multi GPU load balanced Mandelbrot Test " << std::endl;
		std::cout << " Press 'f' for CPU Farm Mandelbrot Test " << std::endl;
		std::cout << " Press 'j' for CPU Fork-Join Mandelbrot Test " << std::endl;
		std::cin >> testChoice;

		if (testChoice == 's') {
			std::cout << " Single GPU Partitioned Mandelbrot Test " << std::endl;
			std::cout << "Happy Testing" << std::endl;
			testingSingleGPU = true;
		}
		else if (testChoice == 'l') {
			std::cout << " Multi GPU load balanced Mandelbrot Test" << std::endl;
			std::cout << "Happy Testing" << std::endl;
			testingMultiGPU = true;
		}
		else if (testChoice == 'f') {
			std::cout << " CPU Farm Mandelbrot Test" << std::endl;
			std::cout << "Happy Testing" << std::endl;
			testingCPUFarm = true;
		}
		else if (testChoice == 'j') {
			std::cout << " CPU Fork-Join Mandelbrot Test" << std::endl;
			std::cout << "Happy Testing" << std::endl;
			testingCPUFork = true;
		}
		isTesting = true;
	}
	else {
		std::cout << "*****INVALID INPUT******" << std::endl;
		userInput = NULL;
		UserSelection();
	}

}

// Compiles standard output text including FPS and current mouse position.
void Scene::renderTextOutput()
{
	// Render current mouse position and frames per second.
	sprintf_s(mouseText, "Mouse: %i, %i", input->getMouseX(), input->getMouseY());
	displayText(-1.f, 0.96f, 1.f, 0.f, 0.f, mouseText);
	displayText(-1.f, 0.90f, 1.f, 0.f, 0.f, fps);
}

//function to add task to the farms queue
void Scene::PopulateCPUFarm()
{ 
	int slices = 16;
	int sliceSize = HEIGHT / slices;

	// add tasks to farm queue
	for (int j = 0; j < slices; ++j) {
		farm.add_CPUTask(new CPUMandelBrotTask(j, -2.0, 1.0, 1.125, -1.125, j * sliceSize, (j * sliceSize) + sliceSize, (TwoDemensional*)image));
	}
}

// function to compute the mandelbrot set
void Scene::ComputeMandelbrot(double left, double right, double top, double bottom, double yStart, double yEnd)
{
	for (int y = yStart; y < yEnd; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			// Work out the point in the complex plane that
			// corresponds to this pixel in the output image.

			std::complex<double> c(left + (x * (right - left) / WIDTH),
				top + (y * (bottom - top) / HEIGHT));

			// Start off z at (0, 0).
			std::complex<double> z(0.0, 0.0);

			// Iterate z = z^2 + c until z moves more than 2 units
			// away from (0, 0), or we've iterated too many times.
			int iterations = 0;
			while (abs(z) < 2.0 && iterations < 500)
			{
				z = (z * z) + c;

				++iterations;
			}

			if (iterations == 500)
			{
				// z didn't escape from the circle.
				// This point is in the Mandelbrot set.
				image[y][x] = 0x0ffff0; // black
			}
			else
			{
				// z escaped within less than MAX_ITERATIONS
				// iterations. This point isn't in the set.
				image[y][x] = (iterations * 255 / 500) | (iterations * 75 / 255) | iterations * 125; // SHIFT
			}
		}
	}
}

// function to split up the mandelbrot using the fork join design pattern
void Scene::CPUMandelbrotForkJoin()
{
	int slices = 16;
	int sliceSize = HEIGHT / slices;

	ofstream my_fileTime("Fork-Join.csv");
	std::vector<std::thread*> threads;
	for (int h = 0; h < 100; h++) {
		the_clock::time_point start = the_clock::now();

		for (int i = 0; i < slices; i++)
		{

			// create a mandelbrot for each slice
			threads.push_back(new std::thread([=] { ComputeMandelbrot(-2.0, 1, -1.25, 1.25, i * sliceSize, (i * sliceSize) + sliceSize);
			}));

		}

		for (int j = 0; j < 16; j++)
		{
			threads[j]->join();
		}

		the_clock::time_point end = the_clock::now();
		// Compute the difference between the two times in milliseconds
		auto time_taken = duration_cast<microseconds>(end - start).count();
		my_fileTime << time_taken << "," << 0 << std::endl;
		threads.clear();
	}
	
	std::cout << "Tests Complete" << std::endl;
	isTesting = false;
}

// Renders text to screen. Must be called last in render function (before swap buffers)
void Scene::displayText(float x, float y, float r, float g, float b, char* string) {
	// Get Lenth of string
	int j = strlen(string);

	// Swap to 2D rendering
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, 5, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Orthographic lookAt (along the z-axis).
	gluLookAt(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Set text colour and position.
	glColor3f(r, g, b);
	glRasterPos2f(x, y);
	// Render text.
	for (int i = 0; i < j; i++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
	}
	// Reset colour to white.
	glColor3f(1.f, 1.f, 1.f);

	// Swap back to 3D rendering.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, ((float)width/(float)height), nearPlane, farPlane);
	glMatrixMode(GL_MODELVIEW);
}

// functions to provide information on hardware being used
void Scene::report_accelerator(const accelerator a)
{
	const std::wstring bs[2] = { L"false", L"true" };
	std::wcout << ": " << a.description << " "
		<< std::endl << "       device_path                       = " << a.device_path
		<< std::endl << "       has_display                       = " << bs[a.has_display]
		<< std::endl << "       is_debug                          = " << bs[a.is_debug]
		<< std::endl << "       is_emulated                       = " << bs[a.is_emulated]
		<< std::endl << "       supports_double_precision         = " << bs[a.supports_double_precision]
		<< std::endl << "       supports_limited_double_precision = " << bs[a.supports_limited_double_precision]
		<< std::endl;
}
// List and select the accelerator to use
void Scene::list_accelerators()
{
	//get all accelerators available to us and store in a vector so we can extract details
	std::vector<accelerator> accls = accelerator::get_all();
	// iterates over all accelerators and print characteristics
	for (unsigned i = 0; i < accls.size(); i++)
	{
		std::cout << "num of accel :" << i << std::endl;
		accelerator a = accls[i];
		report_accelerator(a);
		//if ((a.dedicated_memory > 0) & (a.dedicated_memory < 0.5*(1024.0f * 1024.0f)))
		//accelerator::set_default(a.device_path);
	}

	accelerator::set_default(accls[0].device_path);
	accelerator acc = accelerator(accelerator::default_accelerator);
	//std::wcout << " default acc = " << acc.description << std::endl;

} // list_accelerators
  // query if AMP accelerator exists on hardware
void Scene::query_AMP_support()
{
	std::vector<accelerator> accls = accelerator::get_all();
	if (accls.empty())
	{
		std::cout << "No accelerators found that are compatible with C++ AMP" << std::endl;
	}
	else
	{
		std::cout << "Accelerators found that are compatible with C++ AMP" << std::endl;
		list_accelerators();

	}
	std::cout << std::endl << std::endl << std::endl;
} // query_AMP_support