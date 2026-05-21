#ifndef MAIN_HEADER_H
#define MAIN_HEADER_H

#include <cmath>
#include <iostream>
#include <limits>
#include <cstdlib>
#include <memory>
#include <utility>

// C++ Std Usings

using std::make_shared;
using std::shared_ptr;

// Constants
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline constexpr double maxdouble = std::numeric_limits<double>::max();
inline constexpr double mindouble =  std::numeric_limits<double>::lowest();  // = -1.79769e+308

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}
inline double random_double(){
    return std::rand() / (RAND_MAX + 1.0);
}
inline double random_double(double min, double max){
    return min + (max-min)*random_double();
}

// Common Headers

#include "interval.h"
#include "color.h"
#include "ray.h"
#include "vec3.h"

#endif