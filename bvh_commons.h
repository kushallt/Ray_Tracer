#pragma once
#include "main_header.h"
#include "memoryArena.h"
#include "hittable_list.h"
#include <cstdint>

struct BVHhittableInfo
{
    size_t hittableno;
    Bounds3f bounds;
    point3 centroid;
    BVHhittableInfo() = default;
    BVHhittableInfo(size_t hittableno, const Bounds3f &bounds) : hittableno(hittableno), bounds(bounds), centroid(0.5 * bounds.bmin + 0.5 * bounds.bmax) {}
};

struct BVHbuildnode
{

    Bounds3f bounds;
    BVHbuildnode *children[2];
    int splitAxis, firstoffset, nhittables;

    void InitInterior(int axis, BVHbuildnode *c0, BVHbuildnode *c1)
    {
        children[0] = c0;
        children[1] = c1;
        bounds = boundUnion(c0->bounds, c1->bounds);
        splitAxis = axis;
        nhittables = 0;
    }
    void InitLeaf(int first, int n, const Bounds3f &b)
    {
        firstoffset = first;
        nhittables = n;
        bounds = b;
        children[0] = children[1] = nullptr;
    }
};

struct LinearBVHNode {
    Bounds3f bounds;
    union {
        int hittablesOffset;  
        int secondChildOffset;   
    };
    int nhittables; 
    int axis;         
    uint8_t pad[4]; // padding to minimise cache straddles
};