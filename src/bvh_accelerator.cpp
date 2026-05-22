#include "bvh_accelerator.h"

BVHAccelerator::BVHAccelerator(std::vector<std::shared_ptr<hittable>> &objects, int maxprims) : hittables(objects), maxPrimsPerNode(std::min(maxprims, 255))
    {
        if (hittables.size() == 0)
        {
            return;
        }
        std::vector<BVHhittableInfo> hittableinfo(hittables.size());
        for (size_t i = 0; i < hittables.size(); i++)
        {
            hittableinfo[i] = BVHhittableInfo(i, hittables[i]->calculateBounds());
        }
        // std::cout<<"a"<<std::endl;
        MemoryArena arena(1024 * 1024);
        // std::cout<<"b"<<std::endl;
        int totalNodes = 0;
        std::vector<std::shared_ptr<hittable>> orderedprims;
        BVHbuildnode *root;
        
        root = recursiveBuild(arena, hittableinfo, 0, hittables.size(), &totalNodes, orderedprims);                                                 
        // std::cout<<"c"<<std::endl;

        hittables.swap(orderedprims);

        // std::cout<<"d"<<std::endl;
        nodes = new LinearBVHNode[totalNodes];
        // std::cout<<"e"<<std::endl;
        int offset = 0;
        flattenBVHTree(root, &offset);
        // std::cout<<"f"<<std::endl;
    }