#ifndef WARP_H
#define WARP_H

#pragma once
#include <complex>
#include <cmath>
#include <vector>

#include "Image.h"

namespace image {

 typedef double fractal_t;

 struct Point
 {
    fractal_t x;
    fractal_t y;
 };


class Warp {

      public:

    Warp(){}
    virtual ~Warp(){}
    virtual Point operator()(const Point& P) const = 0;  // Pure virtual function to ensure that any derived class will implement it.

};



} // namespace image

#endif // WARP_H
