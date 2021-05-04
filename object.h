#pragma once
#include "Eigen/Dense"
#include "material.h"
#include "vecfrom.h"
#include "pugixml.hpp"
#include "ray.h"
#include <vector>
#include <iostream>

using namespace Eigen;

namespace raytracer
{
    class Object;
    class Scene;

    class RayHit
    {
        public:
            Object* Object;
            Material Material;
            float T;
            Vector3f Point;
            Vector3f Normal;
    };
    
    class AABB
    {
        public:
            AABB(){}
            AABB(const AABB& c)
            {
                Bounds[0] = c.Bounds[0];
                Bounds[1] = c.Bounds[1];
                Center = c.Center;
            }
            Vector3f Bounds[2];
            Vector3f Center;
            bool Intersect(const Ray& ray);
            void ApplyTransform(const Transform<float, 3, Affine>& transform);
    };

    class IHittable
    {
        public:
            virtual bool Hit(const Ray& ray, RayHit& hit) {return false;}
            AABB aabb;
    };

    class BVH : public IHittable
    {
        public:
            BVH(){}
            BVH(IHittable** hs, int count);
            BVH(IHittable* right, IHittable* left);
            IHittable* Left;
            IHittable* Right;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
    };

    class Face : public IHittable
    {
        public:
            Face();
            Face(Vector3f* v0, Vector3f* v1, Vector3f* v2, Material* material);
            Vector3f* V0;
            Vector3f* V1;
            Vector3f* V2;
            Vector3f Normal;
            Vector3f V0V1;
            Vector3f V0V2;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
//        private:
            Material* _material;
    };

    class Object : public IHittable
    {
        public:
            Object(pugi::xml_node node);
            int Id;
            int MaterialId;
            friend std::ostream& operator<<(std::ostream& os, const Object& mesh);
            virtual std::ostream& Print(std::ostream& os) const;
            virtual void Load(const Scene& scene);
            std::string Transformations;
            Transform<float, 3, Affine> LocalToWorld;
            Transform<float, 3, Affine> WorldToLocal;
        protected:
            Material _material;
    };

    class Mesh : public Object
    {
        public:
            Mesh(pugi::xml_node node);
            std::vector<Vector3i> Faces;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(const Scene& scene) override;
            BVH* bvh;
        private:
            Face** _faces;
            int _fCount;
            bool _ply = false;
    };

    class MeshInstance : public Object
    {
        public:
            MeshInstance(pugi::xml_node node);
            int BaseMeshId;
            bool ResetTransform;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(const Scene& scene) override;
            BVH* bvh;            
    };

    class Triangle : public Object
    {
        public:
            Triangle(pugi::xml_node node);
            Vector3i Indices;
            virtual std::ostream& Print(std::ostream& os) const override;            
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(const Scene& scene) override;
        private:
            Face _face;            
    };

    class Sphere : public Object
    {
        public:
            Sphere(pugi::xml_node node);
            int CenterId;
            float Radius;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(const Scene& scene) override;
        private:
            Vector3f _center;            
    };

}