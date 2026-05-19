#ifndef HITTABLE_LIST
#define HITTABLE_LIST

#include "hittable.h"

#include<vector>


class hittable_list : public hittable{
    public:
        std::vector<shared_ptr<hittable>> objects;
        hittable_list() {}
        hittable_list(shared_ptr<hittable> object) {add(object);}

        void add(shared_ptr<hittable> object){
            objects.push_back(object);
        }

        bool hit(const ray& r,  interval ray_t, hit_record& rec) const override{
            hit_record temp_rec;
            bool hit_anything = false;
            double closest_t = ray_t.maxt;
            for(const auto& object:objects){
                if(object->hit(r, interval(ray_t.mint, closest_t), temp_rec)){
                    hit_anything = true;
                    closest_t = temp_rec.t;
                    rec = temp_rec;
                }
            }

            return hit_anything;

        }
};

#endif