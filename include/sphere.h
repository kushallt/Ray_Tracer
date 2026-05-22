#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "bounds.h"
class material;

class sphere : public hittable
{
public:
    sphere(const point3 &center, double radius, shared_ptr<material> mat) : center(center), radius(std::fmax(0, radius)), mat(mat)
    {
    }

    bool hit(const ray &r, interval ray_t, hit_record &rec) const override
    {
        auto oc = center - r.origin();
        auto h = dot(r.direction(), oc);
        auto a = dot(r.direction(), r.direction());
        auto c = dot(oc, oc) - radius * radius;
        auto disc = h * h - a * c;
        auto sqrtd = std::sqrt(disc);

        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root))
        {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        rec.mat = mat;
        auto outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        return true;
    }
    bool hitBVH(const ray &r, interval ray_t, hit_record &rec) const override
    {
        return hit(r, ray_t, rec);
    }

    Bounds3f calculateBounds() override
    {
        Bounds3f bound;
        bound.bmax = center + radius * (vec3(1, 1, 1));
        bound.bmin = center - radius * (vec3(1, 1, 1));
        return bound;
    }
    double radius;

private:
    point3 center;

    shared_ptr<material> mat;
};

#endif