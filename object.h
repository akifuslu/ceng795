#pragma once
#include "material.h"
#include "vector.h"
#include "pugixml.hpp"
#include "ray.h"
#include <vector>

namespace raytracer
{
    class Face
    {
        public:
            Face();
            Face(Vector3f* v0, Vector3f* v1, Vector3f* v2);
            Vector3f* V0;
            Vector3f* V1;
            Vector3f* V2;
            Vector3f Normal;
            Vector3f V0V1;
            Vector3f V0V2;
            bool Hit(const Ray& ray, RayHit& hit);
    };

    class Object
    {
        public:
            Object(pugi::xml_node node);
            int MaterialId;
            friend std::ostream& operator<<(std::ostream& os, const Object& mesh);
            virtual std::ostream& Print(std::ostream& os) const;
            virtual bool Hit(const Ray& ray, RayHit& hit);
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials);
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
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
        private:
            std::vector<Face> _faces;
    };

    class Triangle : public Object
    {
        public:
            Triangle(pugi::xml_node node);
            Vector3i Indices;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
        private:
            Face _face;            
    };

    class Sphere : public Object
    {
        public:
            Sphere(pugi::xml_node node);
            int Center;
            float Radius;
            virtual std::ostream& Print(std::ostream& os) const override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            virtual void Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials) override;
        private:
            Vector3f _center;            
    };
}