#ifndef CPUTASK_H
#define CPUTASK_H


#include <AMP.h>
#include <AMP_math.h>
#include "amp_short_vectors.h"
#include <amp.h>



/** Abstract base class: a task to be executed. */
class CPUTask
{
public:
	virtual ~CPUTask()
	{
	}

	/** Perform the task. Subclasses must override this. */
	virtual void run() = 0;
};

#endif