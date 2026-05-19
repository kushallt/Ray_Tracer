#ifndef CAMERA_H
#define CAMERA_H

#include "main_header.h"
#include "hittable.h"

class camera{
    public:
        double aspect_ratio = 1.0;
        int image_width = 100;
        int samples_per_pixel = 10;
        int max_depth = 10;
        double vfov = 90;
        point3 lookfrom = point3(0,0,0);   // Point camera is looking from
        point3 lookat   = point3(0,0,-1);  // Point camera is looking at
        vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction
        void render(const hittable& world){
            initialize();
            std::cout<<"P3\n"<<image_width<<' '<<image_height<<"\n255\n";
            for (int j = 0; j < image_height; j++)
            {
                for (int i = 0; i < image_width; i++)
                {
                   color pixel_color(0,0,0);
                    for (int sample = 0; sample < samples_per_pixel; sample++) {
                        ray r = get_ray(i, j);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                    write_color(std::cout, pixel_samples_scale * pixel_color);
                    }
                
            }
        }
    private:
        int image_height;
        double pixel_samples_scale;
        point3 center;
        point3 pixel100_loc;
        vec3 pixel_delta_u;
        vec3 pixel_delta_v;
        void initialize(){
            image_height = int(image_width/aspect_ratio);
            image_height = (image_height < 1)? 1:image_height;

            vec3   u, v, w;        

            pixel_samples_scale = 1.0/samples_per_pixel;

            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            auto focal_length = (lookfrom - lookat).length();
            auto theta = degrees_to_radians(vfov);
            auto h = std::tan(theta/2);
            auto viewport_height = 2 * h * focal_length;
            auto viewport_width = viewport_height*(double(image_width)/image_height);
            center = lookfrom;

            auto viewport_u = viewport_width*u;
            auto viewport_v = viewport_height* -v;

            pixel_delta_u = viewport_u / image_width;
            pixel_delta_v = viewport_v / image_height;

            auto  viewport_upper_left = center - focal_length*w - viewport_u/2 - viewport_v/2; 
            pixel100_loc = viewport_upper_left + 0.5*(pixel_delta_u + pixel_delta_v);

        }
        color ray_color(const ray& r, int depth, const hittable& world) const{
            hit_record rec;
            if(depth <= 0){
                return color(0, 0, 0);
            }
            if(world.hit(r, interval(0.001, infinity), rec)){
                ray scattered;
                color attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered))
                    return attenuation * ray_color(scattered, depth-1, world);
                return color(0,0,0);
                }
            vec3 unit_direction = unit_vector(r.direction());
            auto a = 0.5 * (unit_direction.y() + 1.0);
            return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
        }
        ray get_ray(int i, int j) const {

            auto offset = sample_square();
            auto pixel_sample = pixel100_loc
                            + ((i + offset.x()) * pixel_delta_u)
                            + ((j + offset.y()) * pixel_delta_v);

            auto ray_origin = center;
            auto ray_direction = pixel_sample - ray_origin;

            return ray(ray_origin, ray_direction);
        }

        vec3 sample_square() const {
            return vec3(random_double() - 0.5, random_double() - 0.5, 0);
        }
};

#endif