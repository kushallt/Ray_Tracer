#include "bvh_accelerator.h"

bool BVHAccelerator::SAH(int &mid, std::vector<BVHhittableInfo> &hittableinfo, int nbuckets, int dim, int start, int end, Bounds3f centroidbounds, Bounds3f bounds)
{
    int nhittables = end - start;
    if (nhittables <= 4)
    {
        for (int i = start; i < end; i++)
        {
            auto curr = hittableinfo[i];
            for (int j = i - 1; j >= start; j--)
            {
                if (curr.centroid[dim] < hittableinfo[j].centroid[dim])
                {
                    // std::cout<<i<<" "<<j<<std::endl;
                    hittableinfo[j + 1] = hittableinfo[j];
                    hittableinfo[j] = curr;
                }
                else
                    break;
            }
        }
        mid = (start + end) / 2;
        // std::cout<<"mid = "<<mid<<std::endl;
        return 0;
    }
    struct bucketinfo
    {
        int count = 0;
        Bounds3f bounds;
    };
    bucketinfo buckets[nbuckets];
    double centroidboundslength = (centroidbounds.bmax[dim] - centroidbounds.bmin[dim]);
    for (int i = start; i < end; i++)
    {
        int b = nbuckets * (hittableinfo[i].centroid[dim] - centroidbounds.bmin[dim]) / centroidboundslength;
        if (b == nbuckets)
            b--;
        buckets[b].count++;
        buckets[b].bounds = boundUnion(buckets[b].bounds, hittableinfo[i].bounds);
    }
    float cost[nbuckets - 1] = {};
    Bounds3f b0, b1;
    int count0, count1;
    for (int i = 0; i < nbuckets - 1; i++)
    {
        count0 = 0;
        count1 = 0;
        for (int j = 0; j <= i; j++)
        {
            b0 = boundUnion(b0, buckets[j].bounds);
            count0 += buckets[j].count;
        }
        for (int j = i + 1; j < nbuckets; j++)
        {
            b1 = boundUnion(b1, buckets[j].bounds);
            count1 += buckets[j].count;
        }
        cost[i] = .125f + (count0 * b0.surfaceArea() + count1 * b1.surfaceArea()) / bounds.surfaceArea();
    }
    float minCost = cost[0];
    int minCostSplitBucket = 0;
    for (int i = 1; i < nbuckets - 1; ++i)
    {
        if (cost[i] < minCost)
        {
            minCost = cost[i];
            minCostSplitBucket = i;
        }
    }
    float leafCost = nhittables;
    // std::cout<<"leafcost :"<<leafCost<<" mincost:"<<minCost<<std::endl;
    if (nhittables > maxPrimsPerNode || minCost < leafCost)
    {
        int curind = start - 1;
        for (int i = start; i < end; i++)
        {
            int b = nbuckets * (hittableinfo[i].centroid[dim] - centroidbounds.bmin[dim]) / centroidboundslength;
            if (b == nbuckets)
                b--;
            if (b <= minCostSplitBucket)
            {
                curind++;
                std::swap(hittableinfo[i], hittableinfo[curind]);
            }
        }
        mid = curind + 1;
        // std::cout<<"mid : "<<mid<<std::endl;
        return 0;
    }
    else
    {
        return 1;
    }

    return 1;
}