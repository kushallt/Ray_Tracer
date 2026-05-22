#include "bvh_accelerator.h"

BVHbuildnode *BVHAccelerator::recursiveBuild(MemoryArena &arena, std::vector<BVHhittableInfo> &hittableinfo, int start, int end, int *totalNodes, std::vector<std::shared_ptr<hittable>> &orderedprims)
    {

        BVHbuildnode *node = (BVHbuildnode *)arena.allocate(builnodesize, builnodesize);
        (*totalNodes)++;
        Bounds3f bounds;
        for (int i = start; i < end; ++i)
            bounds = boundUnion(bounds, hittableinfo[i].bounds);

        int nhittables = end - start;
        if (nhittables == 1)
        {
            // std::cout<<"start :"<<start<<" end:"<<end<<std::endl;
            return makeLeaf(node, orderedprims, hittableinfo, nhittables, bounds, start, end);
        }
        else
        {
            double x = hittableinfo[start].centroid.x();
            double y = hittableinfo[start].centroid.y();
            double z = hittableinfo[start].centroid.z();
            double xmax = x, ymax = y, zmax = z;
            for (int i = start + 1; i < end; i++)
            {
                x = std::min(hittableinfo[i].centroid.x(), x);
                y = std::min(hittableinfo[i].centroid.y(), y);
                z = std::min(hittableinfo[i].centroid.z(), z);
                xmax = std::max(hittableinfo[i].centroid.x(), xmax);
                ymax = std::max(hittableinfo[i].centroid.y(), ymax);
                zmax = std::max(hittableinfo[i].centroid.z(), zmax);
            }
            double xdiff = xmax - x, ydiff = ymax - y, zdiff = zmax - z;
            int dim;
            if (xdiff >= ydiff && xdiff >= zdiff)
                dim = 0;
            else if (ydiff >= xdiff && ydiff >= zdiff)
                dim = 1;
            else
                dim = 2;
            Bounds3f centroidbounds = Bounds3f(vec3(x, y, z), vec3(xmax, ymax, zmax));
            int mid;
            if (centroidbounds.bmax[dim] == centroidbounds.bmin[dim])
            {
                // std::cout<<"mstart :"<<start<<" mend :"<<end<<std::endl;
                return makeLeaf(node, orderedprims, hittableinfo, nhittables, bounds, start, end);
            }
            else
            {
                bool returnleaf = SAH(mid, hittableinfo, 8, dim, start, end, centroidbounds, bounds);
                // for(const auto& ent:hittableinfo){
                //     std::cout<<ent.hittableno<<std::endl;
                // }
                if (returnleaf)
                    return makeLeaf(node, orderedprims, hittableinfo, nhittables, bounds, start, end);
                else{
                    BVHbuildnode *left = recursiveBuild(arena, hittableinfo, start, mid, totalNodes, orderedprims);
                    BVHbuildnode  *right = recursiveBuild(arena, hittableinfo, mid, end, totalNodes, orderedprims);
                    node->InitInterior(dim, left, right);
                }
                // node->bounds.printbounds();
                return node;
            }
        }
    }