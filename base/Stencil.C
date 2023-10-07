#include "Stencil.h"

using namespace image;

Stencil::Stencil(int halfwidth)
: half_width(halfwidth)
{
	// Compute the full size of the stencil
	int const fullWidth = (2 * half_width + 1);
	int const fullSize = fullWidth * fullWidth;

	float random[fullSize];

	// Initialize random number generator
	std::random_device seed;
	std::mt19937 gen(static_cast<unsigned int>(seed()));
	std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

	// Generate random numbers between -0.1 and 0.1 for all but the center element
	float sum = 0.0f;
	for(int i = 0; i < fullSize; ++i)
	{
		if(i == 4) continue;
		random[i] = dist(gen);
		sum += random[i];
	}

	// Compute the value of the center element so that the sum of all elements is 1
	int const centerIndex = floor(fullSize / 2);
	random[centerIndex] = 1.0f - sum;

	// Initialize stencil values to above choice
	stencil_values = new float[fullSize];
	for(int i = 0; i < fullSize; i++) {
		stencil_values[i] = random[i];
	}
}

Stencil::~Stencil()
{
	delete[] stencil_values;
}

int Stencil::getFullWidth() const
{
	return (2 * half_width + 1);
}

int Stencil::getSize() const
{
	return getFullWidth() * getFullWidth();
}

int Stencil::getIndex(int relativeCol, int relativeRow) const
{
	int fullWidth = this->getFullWidth();

	// converting from (0,0)-centered to (0,0)-cornered
	int shiftedCol = relativeCol + half_width;
	int shiftedRow = relativeRow + half_width;

	// deriving index to row containing requested element index
	int rowIndex = shiftedRow * fullWidth;

	// deriving index to requested element
	int index = rowIndex + shiftedCol;

	return index;
}

float const & Stencil::operator() (int iCol, int jRow) const
{
	return stencil_values[getIndex(iCol, jRow)];
}

// void Stencil::printStencil(void) const
// {
// 	for(int i = 0; i < getFullWidth(); i++) {
// 		for(int j = 0; j < getFullWidth(); j++) {
// 			int index = i * getFullWidth() + j;
// 			std::cout << std::setw(6) << std::setprecision(2) << stencil_values[index] << " ";
// 		}
// 		std::cout << "\n";
// 	}
// }

// float & Stencil::operator() (int iCol, int jRow)
// {
//     return; // for setting something
// }
