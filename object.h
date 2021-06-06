#pragma once
#include "Eigen/Dense"
#include "material.h"
#include "vecfrom.h"
#include "pugixml.hpp"
#include "ray.h"
#include <vector>
#include <iostream>
#include "texture.h"

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
            DiffuseTexture* Texture;
            float u, v;
            Matrix<float, 3, 3> TBN;
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
            void Extend(Vector3f offset)
            {
                Vector3f b0 = Bounds[0] + offset;
                Vector3f b1 = Bounds[1] + offset;
                Bounds[0].x() = std::min(Bounds[0].x(), b0.x());
                Bounds[0].y() = std::min(Bounds[0].y(), b0.y());
                Bounds[0].z() = std::min(Bounds[0].z(), b0.z());
                Bounds[1].x() = std::min(Bounds[1].x(), b1.x());
                Bounds[1].y() = std::min(Bounds[1].y(), b1.y());
                Bounds[1].z() = std::min(Bounds[1].z(), b1.z());
            }
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
            Face(Vector3f* v0, Vector3f* v1, Vector3f* v2, Material* material,
                    Vector2f* uv0, Vector2f* uv1, Vector2f* uv2);
            Vector3f* V0;
            Vector3f* V1;
            Vector3f* V2;
            Vector2f* UV0;
            Vector2f* UV1;
            Vector2f* UV2;
            Vector3f Normal;
            Vector3f V0V1;
            Vector3f V0V2;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
//        private:
            Material* _material;
            Matrix<float, 3, 3> TBN;
    };

    class Object : public IHittable
    {
        public:
            Object(pugi::xml_node node);
            int Id;
            int MaterialId;
            friend std::ostream& operator<<(std::ostream& os, const Object& mesh);
            virtual std::ostream& Print(std::ostream& os) const;
            virtual void Load(Scene& scene);
            std::string Transformations;
            Transform<float, 3, Affine> LocalToWorld;
            Transform<float, 3, Affine> WorldToLocal;
            Vector3f MotionBlur;
            DiffuseTexture* DiffuseMap = NULL;
            NormalTexture* NormalMap = NULL;
            BumpTexture* BumpMap = NULL; 
        protected:
            Material _material;
            int _texIds[2];
    };

    class Mesh : public Object
    {
        public:
            Mesh(pugi::xml_node node);
            std::vector<Vector3i> Faces;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(Scene& scene) override;
            BVH* bvh;
        private:
            Face** _faces;
            int _fCount;
            bool _ply = false;
            int _offset;
            int _tOffset;
    };

    class MeshInstance : public Object
    {
        public:
            MeshInstance(pugi::xml_node node);
            int BaseMeshId;
            bool ResetTransform;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(Scene& scene) override;
            BVH* bvh;            
    };

    class Triangle : public Object
    {
        public:
            Triangle(pugi::xml_node node);
            Vector3i Indices;
            virtual std::ostream& Print(std::ostream& os) const override;            
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(Scene& scene) override;
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
            virtual void Load(Scene& scene) override;
        private:
            Vector3f _center;            
    };

}