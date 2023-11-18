#include "FractalSet.h"

#include <algorithm> // for std::clamp
#include <cmath> // for std::pow
#include <iostream>
#include <vector>

using namespace image;

ColorLUT::ColorLUT(double gamma)
: gamma(gamma)
{
	black = { 0.0f, 0.0f, 0.0f }; // black color

	bands = {
		{1.0f,	0.0f, 0.0f}, // red
		{ 0.0f, 1.0f, 0.0f}, // green
		{ 0.0f, 0.0f, 1.0f}  // blue
	};
}

void image::ColorLUT::operator() (double const & v, std::vector<float> & C) const
{
	using std::cout;
	using std::endl;

	// Clamping the input value 'v' to be within [0, 1]
	double clamped_v = v;
	if(v < 0.0) clamped_v = 0.0;
	if(v > 1.0) clamped_v = 1.0;

	if(clamped_v >= 1.0) {
		// If clamped_v > 1, set color to black
		C = black;
	} else {
		// Linearly interpolate a color value from the color LUT
		double x = clamped_v * bands.size();
		int m = static_cast<int>(x);
		if(m < 0) m = 0;
		if(m >= (int)bands.size()) m = bands.size() - 1;

		int m_prime = m + 1;
		double w = x - m;

		// cout << "Original v value: " << v << endl;
		// cout << "Clamped v value: " << clamped_v << endl;
		// cout << "Interpolation Indexes: " << m << ", " << m_prime << endl;

		// Ensuring m_prime doesn't exceed the bounds of 'bands'
		if(m_prime >= (int)bands.size()) {
			m_prime = bands.size() - 1;
		}

		for(size_t i = 0; i < 3; ++i) {
			if(m < bands.size() && m_prime < bands.size()) {
				C[i] = bands[m][i] * (1 - w) + bands[m_prime][i] * w;
			} else {
				cout << "Warning: Invalid interpolation indices. Setting color to black." << endl;
				C = black;
				break;
			}
		}
	}
}

void image::ApplyFractalWarpLUT(Point const & center, double const range, Warp const & warp, ColorLUT const & lut, Image & output)
{
	using std::cout;
	using std::endl;
	cout << "Inside ApplyFractalWarpLUT:" << endl;
	cout << "Image Dimensions: (" << output.getWidth() << ", " << output.getHeight() << ")" << endl;

	float const normalizationRadius = 2.0; // Normalization radius for computing the rate of a point

	// Loop over all pixels in the output image
	for(int jRow = 0; jRow < output.getHeight(); jRow++)
	{
#pragma omp parallel for
		for(int iCol = 0; iCol < output.getWidth(); iCol++)
		{
			// Calculate the original point's position based on pixel position, range, and center
			Point original;
			original.x = 2.0 * (double)iCol / (double)output.getWidth() - 1.0;
			original.y = 2.0 * (double)jRow / (double)output.getHeight() - 1.0;
			original.x *= range;
			original.y *= range;
			original.x += center.x;
			original.y += center.y;

			// // Transform the original point using the provided warp
			// if(iCol % 100 == 0 && jRow % 100 == 0) { // Only print occasionally, for example every 100 pixels.
			// 	cout << "Original Point: (" << original.x << ", " << original.y << ")" << endl;
			// }

			Point warped = warp(original);

			// // if(iCol % 100 == 0 && jRow % 100 == 0) {
			// 	cout << "Warped Point: (" << warped.x << ", " << warped.y << ")" << endl;
			// }

			// Compute the distance of the transformed point from the origin
			double distanceFromOrigin = std::sqrt(warped.x * warped.x + warped.y * warped.y);

			// Normalize the distance to obtain a rate
			double rate = distanceFromOrigin / normalizationRadius;

			// Use the lookup table to obtain the color corresponding to the rate
			std::vector<float> color(3, 0.0);
			lut(rate, color);

			// Set the pixel value in the output image
			output.setValue(iCol, jRow, color);
		}
	}
}

JuliaSet::JuliaSet(Point const & juliaConstant, int const iterationCount, int const numCycles)
: center(juliaConstant)
, iterations(iterationCount)
, cycles(numCycles)
{
}

Point JuliaSet::operator() (Point const & input) const
{
	// Initialize the current complex point to the input point
	std::complex<double> current(input.x, input.y);

	// The constant complex value for the Julia Set
	std::complex<double> juliaConsant(center.x, center.y);

	// Iteratively refine the value of the fractal for the given point
	for(int iteration = 0; iteration < iterations; iteration++)
	{
		using std::cout;
		using std::endl;
		// if(iteration % 50 == 0) { // Print every 50 iterations to reduce output
		// 	cout << "Iteration: " << iteration << ", Current: " << current.real() << " + " << current.imag() << "i" << endl;
		// }

		std::complex<double> temp = current;
		for(int cycle = 1; cycle < cycles; cycle++)
		{
			temp *= current;
		}
		current = temp + juliaConsant;
	}

	// Extract the real and imaginary parts to form the output point
	Point output;
	output.x = current.real();
	output.y = current.imag();

	return output;
}
