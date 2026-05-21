#ifndef HITTABLE_LIST
#define HITTABLE_LIST

#include "hittable.h"
#include "bvh_commons.h"
#include "sphere.h"
#include <vector>

class hittable_list : public hittable
{
public:
    std::vector<shared_ptr<hittable>> objects;
    LinearBVHNode *linearnode = nullptr;
    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void add(shared_ptr<hittable> object)
    {
        objects.push_back(object);
    }

    bool hit(const ray &r, interval ray_t, hit_record &rec) const override
    {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest_t = ray_t.maxt;
        for (const auto &object : objects)
        {
            if (object->hit(r, interval(ray_t.mint, closest_t), temp_rec))
            {
                hit_anything = true;
                closest_t = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
    bool hitBVH(const ray &r, interval ray_t, hit_record &rec) const override
    {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest_t = ray_t.maxt;

        int toVisitOffset = 0, currentNodeIndex = 0;
        int nodesToVisit[64];

        while(true){
            const LinearBVHNode *node = &linearnode[currentNodeIndex];
            if(r.intersectBounds(node->bounds)){
                if(node->nhittables > 0){
                    int start = node->hittablesOffset;
                    int end =  start + node->nhittables;
                    // std::cout<<"start offset :"<<start<<std::endl;
                    // node->bounds.printbounds();
                    for(int i=start; i < end; i++){
                        std::shared_ptr<sphere> obj = std::dynamic_pointer_cast<sphere>(objects[i]);
                        // std::cout<<"index :"<<i<<" radius :"<<obj->radius<<std::endl;
                        if(objects[i]->hit(r, interval(ray_t.mint, closest_t), temp_rec)){
                            hit_anything = true;
                            closest_t = temp_rec.t;
                            rec = temp_rec;
                        }
                    }
                    if (toVisitOffset == 0) break;
                    currentNodeIndex = nodesToVisit[--toVisitOffset];
                }
                else{
                    // std::cout<<"interior node"<<std::endl;
                    if(r.direction()[node->axis] < 0){
                        // std::cout<<"right taken"<<std::endl;
                        nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
                        currentNodeIndex = node->secondChildOffset;
                    }
                    else{
                        // std::cout<<"left taken"<<std::endl;
                        currentNodeIndex++;
                        nodesToVisit[toVisitOffset++] = node->secondChildOffset;
                    }
                }
            }
            else{
                // std::cout<<"Bounds not intersect"<<std::endl;
                if(toVisitOffset == 0) break;
                else currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
        }

        return hit_anything;
    }
    Bounds3f calculateBounds() override
    {
        Bounds3f bounds;
        for (auto &obj : objects)
            bounds = boundUnion(bounds, obj->calculateBounds());
        return bounds;
    }
};


#endif