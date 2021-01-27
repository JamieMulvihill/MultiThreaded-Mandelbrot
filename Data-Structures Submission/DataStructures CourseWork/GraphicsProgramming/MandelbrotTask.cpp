#include "MandelbrotTask.h"

// struct complex used to do calculations in amp restricted code
struct Complex1 {
	decimal x;
	decimal y;
};

Complex1 c_add(Complex1 c1, Complex1 c2) restrict(cpu, amp) // restrict keyword - able to execute this function on the GPU and CPU
{
	Complex1 tmp;
	decimal a = c1.x;
	decimal b = c1.y;
	decimal c = c2.x;
	decimal d = c2.y;
	tmp.x = a + c;
	tmp.y = b + d;
	return tmp;
} // c_add

float c_abs(Complex1 c) restrict(cpu, amp)
{
	return (c.x*c.x + c.y*c.y);
} // c_abs

Complex1 c_mul(Complex1 c1, Complex1 c2) restrict(cpu, amp)
{
	Complex1 tmp;
	decimal a = c1.x;
	decimal b = c1.y;
	decimal c = c2.x;
	decimal d = c2.y;
	tmp.x = a * c - b * d;
	tmp.y = b * c + a * d;
	return tmp;
} // c_mul

void MandelbrotTask::run(concurrency::accelerator &a,int rows)
{
	concurrency::accelerator_view view = a.default_view; // create an accelorator view for the kernal to be done on
	concurrency::array_view<uint32_t, 2> imageT(data->writeExt, imageTD); // create an array view to transfer data from cpu to gpu 

	double left_ = data->left;
	double right_ = data->right;
	double top_ = data->top;
	double bottom_ = data->bottom;


	parallel_for_each(view, data->writeExt, [=](concurrency::index<2>idx) restrict(amp)  // runs a kernal for each element within the extent
	{
		int x = idx[1]; // x value is set to second value of the index position
		int y = idx[0]; // y value is set tothe firstvalue of the index position
		
		double  aLeft = left_;
		double aRight = right_;
		double aTop = top_;
		double aBottom = bottom_;
		
		Complex1 c =
		{
			aLeft + (x * (aRight - aLeft) / 640),
			aTop + (y * (aBottom - aTop) / rows)

		};

		Complex1 z =
		{
			0,
			0
		};
		int iterations = 0;

		while (c_abs(z) < 4.0 && iterations < 500)
		{
			z = c_add(c_mul(z, z), c);

			++iterations;
		}

		if (iterations == 500)
		{
			imageT[y][x] = 0xff0000ff; // stores calculation back into he array view at index location

		}
		else
		{
			imageT[y][x] = (((iterations / 4) % 255) << 16) | ((iterations % 255) << 8) | (iterations / 4) % 255;   // stores calculation back into he array view at index location
		}
	});
	imageT.synchronize(); // synchronize values and copy back to cpu
}


