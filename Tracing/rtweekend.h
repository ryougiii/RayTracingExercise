//rtweekend.h
#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include "ray.h"
#include "vec3.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

// Usings

using std::make_shared;
using std::shared_ptr;

// Utility Functions

inline double degrees_to_radians(double degrees)
{
    return degrees * pi / 180;
}

class aabb
{
public:
    aabb() {}
    aabb(const vec3 &a, const vec3 &b)
    {
        _min = a;
        _max = b;
    }

    inline bool hit(const ray &r, double tmin, double tmax) const
    {
        for (int a = 0; a < 3; a++)
        {
            auto invD = 1.0f / r.direction()[a];
            auto t0 = (min()[a] - r.origin()[a]) * invD;
            auto t1 = (max()[a] - r.origin()[a]) * invD;
            if (invD < 0.0f)
                std::swap(t0, t1);
            tmin = t0 > tmin ? t0 : tmin;
            tmax = t1 < tmax ? t1 : tmax;
            if (tmax <= tmin)
                return false;
        }
        return true;
    }
    vec3 min() const { return _min; }
    vec3 max() const { return _max; }

    vec3 _min;
    vec3 _max;
};

#endif