#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "Image.h"
#include "FractalSet.h"
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

    // Applies contrast transformation on the given image via average and RMS
    static void applyContrastTransformation(const Image & imageToReadFrom, Image & imageToWriteTo);

    static void applyHistogramEqualization(const Image & imageToReadFrom, Image & imageToWriteTo);


  private:
}; // class ImageProcessor

} // namespace image

#endif // IMAGE_PROCESSOR_H
