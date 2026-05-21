#pragma once
#include "main_header.h"
#include "memoryArena.h"
#include "hittable_list.h"
#include "bvh_commons.h"


class BVHAccelerator
{
private:
    const int maxPrimsPerNode;
    size_t builnodesize = sizeof(BVHbuildnode);
    

public:
    LinearBVHNode *nodes = nullptr;
    std::vector<std::shared_ptr<hittable>> hittables;
    BVHAccelerator(std::vector<std::shared_ptr<hittable>> &objects, int maxprims);

    BVHbuildnode *recursiveBuild(MemoryArena &arena, std::vector<BVHhittableInfo> &hittableinfo, int start, int end, int *totalNodes, std::vector<std::shared_ptr<hittable>> &orderedprims);
    
    bool SAH(int &mid, std::vector<BVHhittableInfo> &hittableinfo, int nbuckets, int dim, int start, int end, Bounds3f centroidbounds, Bounds3f bounds);

    BVHbuildnode *makeLeaf(BVHbuildnode *node, std::vector<std::shared_ptr<hittable>> &orderedprims, std::vector<BVHhittableInfo> &hittableinfo, int nhittables, Bounds3f bounds, int start, int end)
    {
        int firstoffset = orderedprims.size();
        for (int i = start; i < end; i++)
        {
            int hitnum = hittableinfo[i].hittableno;
            orderedprims.push_back(hittables[hitnum]);
            // std::cout<<"hitnum : "<<hitnum<<" "<<std::endl;
        }
        node->InitLeaf(firstoffset, nhittables, bounds);
        // std::cout<<"firstoffset : "<<firstoffset<<std::endl;
        // node->bounds.printbounds();
        return node;
    }

    int flattenBVHTree(BVHbuildnode *node, int *offset);
};