#ifndef STENCIL_H
#define STENCIL_H

#include <ctime>
#include <random>
#include <iostream> //TODO: debug
#include <iomanip> //TODO: debug

namespace image {

class Stencil {

      public:

	static int const DEFAULT_HALF_WIDTH = 5;

	Stencil(int halfwidth = DEFAULT_HALF_WIDTH);
	~Stencil();

	int getHalfwidth() const
	{
		return half_width;
	}

	int getFullWidth() const;

	int getSize() const;

	int getIndex(int iCol, int jRow) const;

	void printStencil(void) const;

	// float & operator()(int iCol, int jRow);
	float const & operator() (int iCol, int jRow) const;

      private:

	int half_width;
	float * stencil_values;

}; // class Stencil

} // namespace image

#endif // STENCIL_H
