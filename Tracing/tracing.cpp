//main.cc
#include "rtweekend.h"
#define STB_IMAGE_IMPLEMENTATION
#include "bvh.h"
#include "camera.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "stb-master\\stb_image.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <malloc.h>
using namespace std;

auto checker = make_shared<checker_texture>(
    make_shared<constant_texture>(vec3(0.2, 0.3, 0.1)),
    make_shared<constant_texture>(vec3(0.9, 0.9, 0.9)));

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
    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
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

hittable_list two_perlin_spheres()
{
    hittable_list objects;

    auto pertext = make_shared<noise_texture>();
    objects.add(make_shared<sphere>(vec3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
    objects.add(make_shared<sphere>(vec3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    return objects;
}

hittable_list random_scene()
{
    hittable_list world;

    // world.add(make_shared<sphere>(
    //     vec3(0, -1000, 0), 1000, make_shared<lambertian>(vec3(0.5, 0.5, 0.5))));
    world.add(make_shared<sphere>(vec3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

    for (int a = -10; a < 10; a++)
    {
        for (int b = -10; b < 10; b++)
        {
            auto choose_mat = random_double();
            vec3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - vec3(4, .2, 0)).length() > 0.9)
            {
                if (choose_mat < 0.8)
                {
                    // diffuse
                    auto albedo = vec3::random() * vec3::random();
                    world.add(make_shared<moving_sphere>(
                        center, center + vec3(0, random_double(0, .5), 0), 0.0, 1.0, 0.2,
                        make_shared<lambertian>(make_shared<constant_texture>(albedo))));
                }
                else if (choose_mat < 0.95)
                {
                    // metal
                    auto albedo = vec3::random(.5, 1);
                    auto fuzz = random_double(0, .5);
                    world.add(
                        make_shared<sphere>(center, 0.2, make_shared<metal>(albedo, fuzz)));
                }
                else
                {
                    // glass
                    world.add(make_shared<sphere>(center, 0.2, make_shared<dielectric>(1.5)));
                }
            }
        }
    }

    world.add(make_shared<sphere>(vec3(0, 1, 0), 1.0, make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(
        vec3(-4, 1, 0), 1.0, make_shared<lambertian>(make_shared<constant_texture>(vec3(0.4, 0.2, 0.1)))));
    world.add(make_shared<sphere>(
        vec3(4, 1, 0), 1.0, make_shared<metal>(vec3(0.7, 0.6, 0.5), 0.0)));

    //return world;
    return static_cast<hittable_list>(make_shared<bvh_node>(world, 0, 1));
}

hittable_list simple_light()
{
    hittable_list objects;

    auto pertext = make_shared<noise_texture>(4);
    objects.add(make_shared<sphere>(vec3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
    objects.add(make_shared<sphere>(vec3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(make_shared<constant_texture>(vec3(4, 4, 4)));
    objects.add(make_shared<sphere>(vec3(0, 7, 0), 2, difflight));
    objects.add(make_shared<xy_rect>(3, 5, 1, 3, -2, difflight));

    return objects;
}

int main()
{
    time_t nowtim = time(0);

    ofstream ou;
    ou.open(strtx);
    //ou.open(strho);
    const int image_width = 400;
    const int image_height = 200;
    const int samples_per_pixel = 100;
    const int max_depth = 20;
    const vec3 background(0, 0, 0);

    ou << "P3\n"
       << image_width << " " << image_height << "\n255\n";

    //auto world = random_scene();
    auto world = simple_light();
    //auto world = two_perlin_spheres();

    const auto aspect_ratio = double(image_width) / image_height;

    vec3 lookfrom(13, 2, 3);
    vec3 lookat(0, 0, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

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