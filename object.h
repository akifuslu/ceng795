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

    class RayHit
    {
        public:
            Object* Object;
            Material Material;
            float T;
            Vector3f Point;
            Vector3f Normal;
    };
    
    class IHittable
    {
        public:
            virtual bool Hit(const Ray& ray, RayHit& hit) {return false;}
            Vector3f Center;
            Vector3f Bounds[2];
            virtual void Print()
            {
                std::cout << Bounds[0] << std::endl;
                std::cout << Bounds[1] << std::endl;
            }
    };

    class AABB : public IHittable
    {
        public:
            AABB(){}
            AABB(IHittable** hs, int count);
            AABB(IHittable* right, IHittable* left);
            IHittable* Left;
            IHittable* Right;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Print() override
            {
                std::cout << Bounds[0] << std::endl;
                std::cout << Bounds[1] << std::endl;
                Left->Print();
                Right->Print();
            }
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

    class Object
    {
        public:
            Object(pugi::xml_node node);
            int MaterialId;
            friend std::ostream& operator<<(std::ostream& os, const Object& mesh);
            virtual std::ostream& Print(std::ostream& os) const;
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials);
            virtual std::vector<IHittable*> GetHittables() 
            {
                return std::vector<IHittable*>();
            }
        protected:
            Material _material;
    };

    class Mesh : public Object
    {
        public:
            Mesh(pugi::xml_node node);
            std::vector<Vector3i> Faces;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
            virtual std::vector<IHittable*> GetHittables() override;
        private:
            std::vector<Face> _faces;
            bool _ply = false;
    };

    class Triangle : public Object
    {
        public:
            Triangle(pugi::xml_node node);
            Vector3i Indices;
            virtual std::ostream& Print(std::ostream& os) const override;            
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
            virtual std::vector<IHittable*> GetHittables() override;
        private:
            Face _face;            
    };

    class Sphere : public Object, public IHittable
    {
        public:
            Sphere(pugi::xml_node node);
            int CenterId;
            float Radius;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
            virtual std::vector<IHittable*> GetHittables() override;
        private:
            Vector3f _center;            
    };

}