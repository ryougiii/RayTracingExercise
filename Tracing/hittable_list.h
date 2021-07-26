//hittable_list.h
#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include "sphere.h"
#include <memory>
#include <vector>

using std::make_shared;
using std::shared_ptr;

class hittable_list : public hittable
{
public:
    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const
    {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = t_max;

        for (const auto &object : objects)
        {
            if (object->hit(r, t_min, closest_so_far, temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    bool bounding_box(double t0, double t1, aabb &output_box) const
    {
        if (objects.empty())
            return false;

        aabb temp_box;
        bool first_box = true;

        for (const auto &object : objects)
        {
            if (!object->bounding_box(t0, t1, temp_box))
                return false;
            output_box = first_box ? temp_box : surrounding_box(output_box, temp_box);
            first_box = false;
        }

        return true;
    }

public:
    std::vector<shared_ptr<hittable>> objects;
};

class flip_face : public hittable
{
public:
    flip_face(shared_ptr<hittable> p) : ptr(p) {}

    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const
    {
        if (!ptr->hit(r, t_min, t_max, rec))
            return false;

        rec.front_face = !rec.front_face;
        return true;
    }

    virtual bool bounding_box(double t0, double t1, aabb &output_box) const
    {
        return ptr->bounding_box(t0, t1, output_box);
    }

public:
    shared_ptr<hittable> ptr;
};

class box : public hittable
{
public:
    box() {}
    box(const vec3 &p0, const vec3 &p1, shared_ptr<material> ptr);

    virtual bool hit(const ray &r, double t0, double t1, hit_record &rec) const;

    virtual bool bounding_box(double t0, double t1, aabb &output_box) const
    {
        output_box = aabb(box_min, box_max);
        return true;
    }

public:
    vec3 box_min;
    vec3 box_max;
    hittable_list sides;
};

box::box(const vec3 &p0, const vec3 &p1, shared_ptr<material> ptr)
{
    box_min = p0;
    box_max = p1;

    sides.add(make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p1.z(), ptr));
    sides.add(make_shared<flip_face>(
        make_shared<xy_rect>(p0.x(), p1.x(), p0.y(), p1.y(), p0.z(), ptr)));

    sides.add(make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p1.y(), ptr));
    sides.add(make_shared<flip_face>(
        make_shared<xz_rect>(p0.x(), p1.x(), p0.z(), p1.z(), p0.y(), ptr)));

    sides.add(make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p1.x(), ptr));
    sides.add(make_shared<flip_face>(
        make_shared<yz_rect>(p0.y(), p1.y(), p0.z(), p1.z(), p0.x(), ptr)));
}

bool box::hit(const ray &r, double t0, double t1, hit_record &rec) const
{
    return sides.hit(r, t0, t1, rec);
}

#endif