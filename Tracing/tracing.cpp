//main.cc
#include "rtweekend.h"
#define STB_IMAGE_IMPLEMENTATION
#include "bvh.h"
#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "stb-master\\stb_image.h"
#include "trans.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <malloc.h>
using namespace std;

vec3 ray_color(const ray &r, const vec3 &background, const hittable &world, int depth)
{
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return vec3(0, 0, 0);

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    ray scattered;
    vec3 attenuation;
    vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered)) //如果返回false认为被吸收
        return emitted;

    return emitted + attenuation * ray_color(scattered, background, world, depth - 1);
}

hittable_list earth()
{
    int nx, ny, nn;
    unsigned char *texture_data = stbi_load("earthmap.jpg", &nx, &ny, &nn, 0);

    auto earth_surface =
        make_shared<lambertian>(make_shared<image_texture>(texture_data, nx, ny));
    auto globe = make_shared<sphere>(vec3(0, 0, 0), 2, earth_surface);

    return hittable_list(globe);
}

hittable_list final_scene()
{
    hittable_list boxes1;
    auto ground =
        make_shared<lambertian>(make_shared<constant_texture>(vec3(0.48, 0.83, 0.53)));

    const int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++)
    {
        for (int j = 0; j < boxes_per_side; j++)
        {
            auto w = 100.0;
            auto x0 = -1000.0 + i * w;
            auto z0 = -1000.0 + j * w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1, 101);
            auto z1 = z0 + w;

            boxes1.add(make_shared<box>(vec3(x0, y0, z0), vec3(x1, y1, z1), ground));
        }
    }

    hittable_list objects;

    objects.add(make_shared<bvh_node>(boxes1, 0, 1));

    auto light = make_shared<diffuse_light>(make_shared<constant_texture>(vec3(7, 7, 7)));
    objects.add(make_shared<xz_rect>(123, 423, 147, 412, 554, light));

    auto center1 = vec3(400, 400, 200);
    auto center2 = center1 + vec3(30, 0, 0);
    auto moving_sphere_material =
        make_shared<lambertian>(make_shared<constant_texture>(vec3(0.7, 0.3, 0.1)));
    objects.add(make_shared<moving_sphere>(center1, center2, 0, 1, 50, moving_sphere_material));

    objects.add(make_shared<sphere>(vec3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    objects.add(make_shared<sphere>(
        vec3(0, 150, 145), 50, make_shared<metal>(vec3(0.8, 0.8, 0.9), 10.0)));

    auto boundary = make_shared<sphere>(vec3(360, 150, 145), 70, make_shared<dielectric>(1.5));
    objects.add(boundary);
    objects.add(make_shared<constant_medium>(
        boundary, 0.2, make_shared<constant_texture>(vec3(0.2, 0.4, 0.9))));
    boundary = make_shared<sphere>(vec3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
    objects.add(make_shared<constant_medium>(
        boundary, .0001, make_shared<constant_texture>(vec3(1, 1, 1))));

    int nx, ny, nn;
    auto tex_data = stbi_load("earthmap.jpg", &nx, &ny, &nn, 0);

    auto emat = make_shared<lambertian>(make_shared<image_texture>(tex_data, nx, ny));
    objects.add(make_shared<sphere>(vec3(400, 200, 400), 100, emat));
    auto pertext = make_shared<noise_texture>(0.1);
    objects.add(make_shared<sphere>(vec3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(make_shared<constant_texture>(vec3(0.73, 0.73, 0.73)));
    int ns = 1000;
    for (int j = 0; j < ns; j++)
    {
        boxes2.add(make_shared<sphere>(vec3::random(0, 165), 10, white));
    }

    objects.add(make_shared<translate>(
        make_shared<rotate_y>(
            make_shared<bvh_node>(boxes2, 0.0, 1.0), 15),
        vec3(-100, 270, 395)));

    return objects;
}

int main()
{
    time_t nowtim = time(0);

    ofstream ou;
    ou.open(strtx);
    //ou.open(strho);
    const int image_width = 200;
    const int image_height = 200;
    const int samples_per_pixel = 100;
    const int max_depth = 20;
    const vec3 background(0, 0, 0);

    ou << "P3\n"
       << image_width << " " << image_height << "\n255\n";

    auto world = final_scene();

    const auto aspect_ratio = double(image_width) / image_height;

    vec3 lookfrom(278, 278, -800);
    vec3 lookat(278, 278, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;
    auto vfov = 40.0;

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

    for (int j = image_height - 1; j >= 0; --j)
    {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i)
        {
            vec3 color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s)
            {
                auto u = (i + random_double()) / image_width;
                auto v = (j + random_double()) / image_height;
                ray r = cam.get_ray(u, v);
                color += ray_color(r, background, world, max_depth);
            }
            color.write_color(ou, samples_per_pixel);
        }
    }

    std::cerr << "\nDone.\n";
    cout << time(0) - nowtim << endl;
}