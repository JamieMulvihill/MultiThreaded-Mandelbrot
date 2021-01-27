#include "farm.h"
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

// funciton to add task to queue
void Farm::add_CPUTask(CPUTask * task) {
	CPUQueue.push(task);
}

// funciton to add task to queue
void Farm::add_task(Task *task) {
	farmQueue.push(task);
}

// function to run mandelbrot on one GPU accelerator
void Farm::SingleGPU(int rows) {
	Task * t;
	while (farmQueue.try_pop(t)) { // while a task can be safely poped of the queue, pop it off
			farmThreads.push_back(new std::thread([&] {  // create a therad
				t->run(accls[0], rows); // run task
			}));
	}

	for (auto t : farmThreads) {
		t->join();
	}
	farmThreads.clear();
}

// function to run mandelbrot on multiple acceleratora
void Farm::MultiGPULoadBalanced(int rows) {
	
	for (concurrency::accelerator a : accls) {
		if (!a.is_emulated) {
			usableAccls.push_back(a);
			acclQueue.push(a); // add non emulated accelrator to queue
			
		}
	}

	for (int i = 0; i < usableAccls.size(); i++) {
		farmThreads.push_back(new std::thread([&] {				// create a thread
			while (true) {
				mutex_.lock();				// lock mutex so no other thread can take the same task or accelersator
				Task * t;
				accelerator a;
				if (farmQueue.try_pop(t) && acclQueue.try_pop(a)) {			// pop a task safely off the queue and check is there an available accerator
					mutex_.unlock();		// unlock mutex
					t->run(a, rows);		// run task
					acclQueue.push(a);
					delete t;

				}
				else {
					mutex_.unlock();
					return;
				}
			}
		}));
	}
	
	for (auto t : farmThreads) { // join threads
		t->join();
		delete t;
	}
	farmThreads.clear();
	usableAccls.clear();

}

void Farm::CPUFarmMandelbrot()
{
	
	farmThreads.clear();

	for (int i = 0; i < coreCount; i++) {
		farmThreads.push_back(new std::thread([&] {				// create a thread
			while (true) {
				mutex_.lock();
				if (!CPUQueue.empty()) {						// if theres a task to be done
					CPUTask *t = CPUQueue.front();
					CPUQueue.pop();								// take it from the queue
					mutex_.unlock();
					t->run();									// run task
					delete t;
				}
				else {
					mutex_.unlock();
					return;
				}
			}
		}));
	}
	for (auto t : farmThreads) {
		t->join();
	}
}
