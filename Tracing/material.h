#ifndef MATERIAL_H
#define MATERIAL_H
#include "hittable_list.h"
#include "ray.h"
#include "texture.h"

class material
{
public:
    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const = 0;

    virtual vec3 emitted(double u, double v, const vec3 &p) const
    {
        return vec3(0, 0, 0);
    }
};

class lambertian : public material
{
public:
    lambertian(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        vec3 scatter_direction = rec.normal + random_unit_vector();
        scattered = ray(rec.p, scatter_direction, r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }

public:
    shared_ptr<texture> albedo;
};

class metal : public material
{
public:
    metal(const vec3 &a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0); //dot<0我们认为吸收
    }

public:
    vec3 albedo;
    double fuzz;
};
double schlick(double cosine, double ref_idx)
{
    auto r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}
class dielectric : public material
{
public:
    dielectric(double ri) : ref_idx(ri) {}

    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        attenuation = vec3(1.0, 1.0, 1.0);
        double etai_over_etat = (rec.front_face) ? (1.0 / ref_idx) : (ref_idx);

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = ffmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        if (etai_over_etat * sin_theta > 1.0)
        {
            vec3 reflected = reflect(unit_direction, rec.normal);
            scattered = ray(rec.p, reflected);
            return true;
        }
        double reflect_prob = schlick(cos_theta, etai_over_etat);
        if (random_double() < reflect_prob)
        {
            vec3 reflected = reflect(unit_direction, rec.normal);
            scattered = ray(rec.p, reflected);
            return true;
        }
        vec3 refracted = refract(unit_direction, rec.normal, etai_over_etat);
        scattered = ray(rec.p, refracted);
        return true;
    }

public:
    double ref_idx;
};

class diffuse_light : public material
{
public:
    diffuse_light(shared_ptr<texture> a) : emit(a) {}

    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        return false;
    }

    virtual vec3 emitted(double u, double v, const vec3 &p) const
    {
        return emit->value(u, v, p);
    }

public:
    shared_ptr<texture> emit;
};

class isotropic : public material
{
public:
    isotropic(shared_ptr<texture> a) : albedo(a) {}

    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        scattered = ray(rec.p, random_in_unit_sphere(), r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }

public:
    shared_ptr<texture> albedo;
};

class constant_medium : public hittable
{
public:
    constant_medium(shared_ptr<hittable> b, double d, shared_ptr<texture> a)
        : boundary(b), neg_inv_density(-1 / d)
    {
        phase_function = make_shared<isotropic>(a);
    }

    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const;

    virtual bool bounding_box(double t0, double t1, aabb &output_box) const
    {
        return boundary->bounding_box(t0, t1, output_box);
    }

public:
    shared_ptr<hittable> boundary;
    shared_ptr<material> phase_function;
    double neg_inv_density;
};

bool constant_medium::hit(const ray &r, double t_min, double t_max, hit_record &rec) const
{
    // Print occasional samples when debugging. To enable, set enableDebug true.
    const bool enableDebug = false;
    const bool debugging = enableDebug && random_double() < 0.00001;

    hit_record rec1, rec2;

    if (!boundary->hit(r, -infinity, infinity, rec1))
        return false;

    if (!boundary->hit(r, rec1.t + 0.0001, infinity, rec2))
        return false;

    if (debugging)
        std::cerr << "\nt0=" << rec1.t << ", t1=" << rec2.t << '\n';

    if (rec1.t < t_min)
        rec1.t = t_min;
    if (rec2.t > t_max)
        rec2.t = t_max;

    if (rec1.t >= rec2.t)
        return false;

    if (rec1.t < 0)
        rec1.t = 0;

    const auto ray_length = r.direction().length();
    const auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    const auto hit_distance = neg_inv_density * log(random_double());

    if (hit_distance > distance_inside_boundary)
        return false;

    rec.t = rec1.t + hit_distance / ray_length;
    rec.p = r.at(rec.t);

    if (debugging)
    {
        std::cerr << "hit_distance = " << hit_distance << '\n'
                  << "rec.t = " << rec.t << '\n'
                  << "rec.p = " << rec.p << '\n';
    }

    rec.normal = vec3(1, 0, 0); // arbitrary
    rec.front_face = true;      // also arbitrary
    rec.mat_ptr = phase_function;

    return true;
}

#endif