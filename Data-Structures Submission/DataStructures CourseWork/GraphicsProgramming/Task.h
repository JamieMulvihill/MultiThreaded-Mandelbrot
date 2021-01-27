#ifndef TASK_H
#define TASK_H


#include <AMP.h>
#include <AMP_math.h>
#include "amp_short_vectors.h"
#include <amp.h>



/** Abstract base class: a task to be executed. */
class Task
{
public:
	virtual ~Task()
	{
	}

	/** Perform the task. Subclasses must override this. */
	virtual void run(concurrency::accelerator &a, int rows) = 0;
};

#endif
