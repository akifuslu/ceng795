#include "object.h"
#include <sstream>
#include <cfloat>
#include <cmath>

namespace raytracer
{

    Object::Object(pugi::xml_node node)
    {
        MaterialId = node.child("Material").text().as_int();
    }

    std::ostream& Object::Print(std::ostream& os) const
    {
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Object& obj)
    {
        return obj.Print(os);
    }

    bool Object::Hit(const Ray& ray, RayHit& hit)
    {
        return false;
    }

    void Object::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        _material = materials[MaterialId - 1];
    }   

    Mesh::Mesh(pugi::xml_node node) : Object(node)
    {
        auto faces = node.child("Faces").text().as_string();
        std::stringstream ss;
        ss << faces;
        int x, y, z;
        while(ss >> x >> y >> z)
        {
            Faces.push_back(Vector3i(x, y, z));
        }
    }

    std::ostream& Mesh::Print(std::ostream& os) const
    {
        os << "Material " << MaterialId << std::endl;
        os << "Faces" << std::endl;
        for(auto& f: Faces)
        {
            os << f << std::endl; 
        }
        return os;
    }

    bool Mesh::Hit(const Ray& ray, RayHit& hit)
    {
        bool flag = false;
        float t = FLT_MAX;        
        for (size_t i = 0; i < _faces.size(); i++)
        {
            RayHit tHit;
            if(_faces[i].Hit(ray, tHit))
            {
                flag = true;
                if(tHit.T < t)
                {
                    t = tHit.T;
                    hit = tHit;
                }                
            }
        }       
        hit.Object = this;     
        hit.Material = _material;
        return flag;
    }

    void Mesh::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        Object::Load(vertexData, materials);
        for(auto& f: Faces)
        {
            //auto v0 = &vertexData[f.X - 1];
            //auto v1 = &vertexData[f.Y - 1];
            //auto v2 = &vertexData[f.Z - 1];
            _faces.push_back(Face(&vertexData[f.X - 1], &vertexData[f.Y - 1], &vertexData[f.Z - 1]));
        }
    }   

    Triangle::Triangle(pugi::xml_node node) : Object(node)
    {
        auto ind = node.child("Indices").text().as_string();
        std::stringstream ss;
        ss << ind;
        int x, y, z;
        ss >> x >> y >> z;
        Indices = Vector3i(x, y, z);
    }

    std::ostream& Triangle::Print(std::ostream& os) const
    {
        os << "Material " << MaterialId << std::endl;
        os << "Indices " << Indices << std::endl;
        return os;
    }

    void Triangle::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        Object::Load(vertexData, materials);
        //auto v0 = &vertexData[Indices.X - 1];
        //auto v1 = &vertexData[Indices.Y - 1];
        //auto v2 = &vertexData[Indices.Z - 1];
        _face = Face(&vertexData[Indices.X - 1], &vertexData[Indices.Y - 1], &vertexData[Indices.Z - 1]);        
    }   

    bool Triangle::Hit(const Ray& ray, RayHit& hit)
    {
        hit.Object = this;
        hit.Material = _material;
        return _face.Hit(ray, hit);
    }


    Sphere::Sphere(pugi::xml_node node) : Object(node)
    {
        Center = node.child("Center").text().as_int();
        Radius = node.child("Radius").text().as_float();
    }

    std::ostream& Sphere::Print(std::ostream& os) const
    {
        os << "Material " << MaterialId << std::endl;
        os << "Center " << Center << std::endl;
        os << "Radius " << Radius << std::endl;
        return os;
    }

    void Sphere::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        Object::Load(vertexData, materials);
        _center = vertexData[Center - 1];
    }   
    
    bool Sphere::Hit(const Ray& ray, RayHit& hit)
    {
        Vector3f oc = ray.Origin - _center;
        float a = Vector3f::Dot(ray.Direction, ray.Direction);
        float b = 2.0 * Vector3f::Dot(oc, ray.Direction);
        float c = Vector3f::Dot(oc,oc) - Radius * Radius;
        float discriminant = b*b - 4*a*c;
        if(discriminant < 0)
        {
            return false;
        }
        else
        {
            hit.T = (-b - std::sqrt(discriminant)) / (2.0*a);
            if(hit.T <= 0)
            {
                return false;
            }
            hit.Object = this;
            hit.Point = ray.Origin + ray.Direction * hit.T;
            hit.Normal = (hit.Point - _center).Normalized();
            hit.Material = _material;
            return true;
        }
    }

    Face::Face() 
    {    }
    
    Face::Face(Vector3f* v0, Vector3f* v1, Vector3f* v2)
        : V0(v0), V1(v1), V2(v2)
    {
        //V0 = v0;
        //V1 = v1;
        //V2 = v2;
        Normal = Vector3f::Cross((*v1) - (*v0), (*v2) - (*v0)).Normalized();
        V0V1 = (*V1) - (*V0);
        V0V2 = (*V2) - (*V0);
    }

    bool Face::Hit(const Ray& ray, RayHit& hit)
    {
        Vector3f pvec = Vector3f::Cross(ray.Direction, V0V2);
        
        float det = Vector3f::Dot(V0V1, pvec);
        if (det < 0)
            return false;

        float invDet = 1.0 / det;
        Vector3f tvec = ray.Origin - (*V0);
        float u = Vector3f::Dot(tvec, pvec) * invDet;
        if (u < 0 || u > 1)
            return false;

        Vector3f qvec = Vector3f::Cross(tvec, V0V1);
        float v = Vector3f::Dot(ray.Direction, qvec) * invDet;
        if (v < 0 || u + v > 1)
            return false;
        float t = Vector3f::Dot(V0V2, qvec) * invDet;
        if(t <= 0)
        {
            return false;
        }
        hit.T = t;
        hit.Point = ray.Origin + ray.Direction * t;
        hit.Normal = Normal;
        return true;
    }
}