#ifndef RAY_H
#define RAY_H

#include "vec3.h"
#include "bounds.h"

class ray
{
public:
    ray() {}
    ray(const point3 &origin, const vec3 &direction) : orig(origin), dir(direction) {}

    const point3 &origin() const { return orig; }
    const vec3 &direction() const { return dir; }
    point3 at(double t) const
    {
        return orig + t * dir;
    }
    bool intersectBounds(Bounds3f bounds) const
    {
        float tmin = -infinity;
        float tmax = infinity;
        for (int i = 0; i < 3; i++)
        {
            float invD = 1.0f / dir[i];
            float t0 = (bounds.bmin.e[i] - orig[i]) * invD;
            float t1 = (bounds.bmax.e[i] - orig[i]) * invD;

            if (invD < 0)
                std::swap(t0, t1);

            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);

            if (tmax <= tmin)
                return false;
        }
        return true;
    }
    void printRay() const {
        std::cout<<"origin :"<<orig.x()<<" "<<orig.y()<<" "<<orig.z()<<std::endl;
        std::cout<<"direction :"<<dir.x()<<" "<<dir.y()<<" "<<dir.z()<<std::endl;
    }

private:
    point3 orig;
    vec3 dir;
};

#endif