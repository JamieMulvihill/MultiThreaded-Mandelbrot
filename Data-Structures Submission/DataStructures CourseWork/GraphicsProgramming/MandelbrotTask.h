#pragma once
#include "Task.h"


using decimal = double;

// struct to specicy what work each Mandelbrot task will carry out
struct TaskData {
	int id;
	int startRow;
	int endRow;
	concurrency::extent<2> writeExt;
	decimal left;
	decimal right;
	decimal top;
	decimal bottom;

	TaskData(int id_) : id(id_) {}
};

// mandelbrottask inherits from task
class MandelbrotTask : public Task
{
public:
	//Constructor is given a taskData pointer and pointer to a unint32_t 
	MandelbrotTask(TaskData * data_, uint32_t * image_)
		: data(data_), imageTD(image_) {}

	// run functions requires an accelrator to know where to run the jernal and the number of rows to be done
	void run(concurrency::accelerator &a, int rows);

	TaskData * data;
	uint32_t * imageTD;

};

