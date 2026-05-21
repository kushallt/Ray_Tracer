#pragma once
#include "vec3.h"
#include "main_header.h"

class Bounds3f{
public:
    vec3 bmin = maxdouble*vec3(1, 1, 1);
    vec3 bmax = mindouble*vec3(1, 1, 1);
    Bounds3f() = default;
    Bounds3f(vec3 v1, vec3 v2) : bmin(v1), bmax(v2) {}

    double surfaceArea(){
        if(checkdefault()) return 0;
        double dx = bmax.x() - bmin.x();
        double dy = bmax.y() - bmin.y();
        double dz = bmax.z() - bmin.z();
        return 2*(dx*dy + dy*dz + dx*dz);
    }
    void printbounds() const {
        std::cout<<"min :"<<bmin.x()<<" "<<bmin.y()<<" "<<bmin.z()<<std::endl;
        std::cout<<"max :"<<bmax.x()<<" "<<bmax.y()<<" "<<bmax.z()<<std::endl;
    }
    bool checkdefault(){
        return (bmin == maxdouble*vec3(1, 1, 1)) && (bmax == mindouble*vec3(1, 1, 1)) ;
    }
};

inline Bounds3f boundUnion(Bounds3f b1, Bounds3f b2) {
    Bounds3f b;

    b.bmin.e[0] = std::min(b1.bmin.x(), b2.bmin.x());
    b.bmin.e[1] = std::min(b1.bmin.y(), b2.bmin.y());
    b.bmin.e[2] = std::min(b1.bmin.z(), b2.bmin.z());

    b.bmax.e[0] = std::max(b1.bmax.x(), b2.bmax.x());
    b.bmax.e[1] = std::max(b1.bmax.y(), b2.bmax.y());
    b.bmax.e[2] = std::max(b1.bmax.z(), b2.bmax.z());

    return b;
}