#pragma once
#include <iostream>
#include <windows.h>
#include <ppl.h>
#include <queue>
#include <mutex>
#include <thread>
#include "Task.h"
#include "CPUTask.h"
#include "MandelbrotTask.h"
#include "CPUMAndelbrotTask.h"
#include <concurrent_queue.h>
#include <condition_variable>


using decimal = double;

using namespace concurrency;


/** A collection of tasks that should be performed in parallel. */
class Farm {
public:
	
	// functions to add tasks to the specific queue
	void add_task(Task *task);
	void add_CPUTask(CPUTask * task);
	
	// functions to run the specified farm
	void SingleGPU(int rows);
	void MultiGPULoadBalanced(int rows);
	void CPUFarmMandelbrot();

private:
	// vectors fro the acclerators, first vector gets all accelerators available, second vector is populated with the non emulated acclerators
	std::vector<accelerator> accls = accelerator::get_all();
	std::vector<accelerator> usableAccls;


	std::queue<CPUTask * > CPUQueue; 
	concurrent_queue<Task *> farmQueue; // concurrent queue used to ensure thread saftey
	concurrent_queue<accelerator > acclQueue;
	std::mutex mutex_;
	std::vector<std::thread *> farmThreads;
	int coreCount = std::thread::hardware_concurrency(); // gets the number of cores and is used to create a number of threads 
};
