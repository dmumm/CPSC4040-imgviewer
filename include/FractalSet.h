#ifndef FRACTSALSET_H
#define FRACTSALSET_H

#include <complex>
#include <cmath>
#include <vector>

#include "Warp.h"


namespace image {

class ColorLUT {
   public:

     ColorLUT(double gamma = 1.0);
    ~ColorLUT(){}

     // Generate color value from the input value
     void operator()(const double& value, std::vector<float>& C) const;

   private:

     double gamma;
     std::vector<float> black;
     std::vector< std::vector<float> > bands;
 };

 void ApplyFractalWarpLUT( const Point& center, const double range, const Warp& warp, const ColorLUT& lut, Image& output);

class JuliaSet : public Warp {
public:

    JuliaSet(const Point& juliaConstant, const int numIterations, const int numCycles);
    ~JuliaSet(){}

    Point operator()(const Point& P) const;

private:

    Point center;
    int iterations;
    int cycles;


}; // class JuliaSet

} // namespace image

#endif // FRACTSALSET_H
