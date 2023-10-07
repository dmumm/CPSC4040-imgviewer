#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "Image.h"
#include "ImageProcessor.h"
#include "Stencil.h"

#include <cmath> // for std::pow
#include <algorithm>  // for std::fill

namespace image {

class ImageProcessor {
  public:

	// Adjusts the gamma level of the given image
	static void applyGamma(float gamma, Image & inputImage);

	// Applies linear convolution on the given image
	static void doCircularLinearConvolution(Stencil const & stencil, const Image & imageToReadFrom, Image & imageToWriteTo);
	static void doBoundedLinearConvolution(Stencil const & stencil, const Image & imageToReadFrom, Image & imageToWriteTo);


	// Add more image processing methods as needed


  private:
}; // class ImageProcessor

} // namespace image

#endif // IMAGE_PROCESSOR_H
