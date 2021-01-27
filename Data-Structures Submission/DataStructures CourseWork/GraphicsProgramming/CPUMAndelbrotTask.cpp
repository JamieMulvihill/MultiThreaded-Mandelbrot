#include "CPUMandelbrotTask.h"

void CPUMandelBrotTask::run() {

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
				imageTD->array[y][x] = 0x0ffff0; // black
			}
			else
			{
				// z escaped within less than MAX_ITERATIONS
				// iterations. This point isn't in the set.
				imageTD->array[y][x] = (iterations * 255 / 500) | (iterations * 75 / 255) | iterations * 125; // SHIFT
			}
		}
	}
}
