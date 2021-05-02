#include "object.h"
#include <sstream>
#include <cfloat>
#include <cmath>
#include "happly.h"

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

    void Object::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        _material = materials[MaterialId - 1];
    }   

    Mesh::Mesh(pugi::xml_node node) : Object(node)
    {
        const char* ply = node.child("Faces").attribute("plyFile").as_string();
        if(std::strcmp(ply, "") == 0)
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
        else
        {
            _ply = true;
            happly::PLYData plyIn(ply);
            std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
            std::vector<std::vector<size_t>> fInd = plyIn.getFaceIndices();            
            for(int i = 0; i < fInd.size(); i++)
            {
                auto v0 = new Vector3f(vPos[fInd[i][0]][0], vPos[fInd[i][0]][1], vPos[fInd[i][0]][2]);
                auto v1 = new Vector3f(vPos[fInd[i][1]][0], vPos[fInd[i][1]][1], vPos[fInd[i][1]][2]);
                auto v2 = new Vector3f(vPos[fInd[i][2]][0], vPos[fInd[i][2]][1], vPos[fInd[i][2]][2]);
                _faces.push_back(Face(v0, v1, v2, &_material));
            }
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

    void Mesh::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        Object::Load(vertexData, materials);
        if(_ply)
        {
            for(auto& f: _faces)
            {
                f._material = &_material;
            }
        }
        else
        {        
            for(auto& f: Faces)
            {
                _faces.push_back(Face(&vertexData[f.x() - 1], &vertexData[f.y() - 1], &vertexData[f.z() - 1], &_material));
            }            
        }
    }   

    std::vector<IHittable*> Mesh::GetHittables()
    {
        std::vector<IHittable*> h;
        for(int i = 0; i < _faces.size(); i++)
        {
            h.push_back(&_faces[i]);
        }
        return h;
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
        _face = Face(&vertexData[Indices.x() - 1], &vertexData[Indices.y() - 1], &vertexData[Indices.z() - 1], &_material);        
    }   

    std::vector<IHittable*> Triangle::GetHittables()
    {
        std::vector<IHittable*> h;
        h.push_back(&_face);
        return h;
    }

    Sphere::Sphere(pugi::xml_node node) : Object(node)
    {
        CenterId = node.child("Center").text().as_int();
        Radius = node.child("Radius").text().as_float();
    }

    std::ostream& Sphere::Print(std::ostream& os) const
    {
        os << "Material " << MaterialId << std::endl;
        os << "Center " << CenterId << std::endl;
        os << "Radius " << Radius << std::endl;
        return os;
    }

    void Sphere::Load(std::vector<Vector3f>& vertexData, std::vector<Material>& materials)
    {
        Object::Load(vertexData, materials);
        _center = vertexData[CenterId - 1];
        Bounds[0].x() = _center.x() - Radius;
        Bounds[0].y() = _center.y() - Radius;
        Bounds[0].z() = _center.z() - Radius;
        Bounds[1].x() = _center.x() + Radius;
        Bounds[1].y() = _center.y() + Radius;
        Bounds[1].z() = _center.z() + Radius;
        Center = (Bounds[0] + Bounds[1]) / 2;        
    }   
    
    bool Sphere::Hit(const Ray& ray, RayHit& hit)
    {
        hit.T = FLT_MAX;
        Vector3f oc = ray.Origin - _center;
        float a = ray.Direction.dot(ray.Direction);
        float b = 2.0 * oc.dot(ray.Direction);
        float c = oc.dot(oc) - Radius * Radius;
        float discriminant = b*b - 4*a*c;
        if(discriminant < 0)
        {
            return false;
        }
        else
        {
            discriminant = std::sqrt(discriminant);
            float t = (-b -discriminant) / (2*a);
            if(t < 0.01)
            {
                t = (-b +discriminant) / (2*a);
            }
            if(t < 0.01)
                return false;
            hit.T = t;
            hit.Object = this;
            hit.Point = ray.Origin + ray.Direction * hit.T;
            hit.Normal = (hit.Point - _center).normalized();
            hit.Material = _material;
            return true;
        }
    }

    std::vector<IHittable*> Sphere::GetHittables()
    {
        std::vector<IHittable*> h;
        h.push_back(this);
        return h;
    }

    Face::Face() 
    {    }
    
    Face::Face(Vector3f* v0, Vector3f* v1, Vector3f* v2, Material* material)
        : V0(v0), V1(v1), V2(v2)
    {
        _material = material;
        Normal = ((*v1) - (*v0)).cross((*v2) - (*v0)).normalized();
        V0V1 = (*V1) - (*V0);
        V0V2 = (*V2) - (*V0);
        Bounds[0].x() = std::min(std::min(V0->x(), V1->x()), V2->x());
        Bounds[0].y() = std::min(std::min(V0->y(), V1->y()), V2->y());
        Bounds[0].z() = std::min(std::min(V0->z(), V1->z()), V2->z());
        Bounds[1].x() = std::max(std::max(V0->x(), V1->x()), V2->x());
        Bounds[1].y() = std::max(std::max(V0->y(), V1->y()), V2->y());
        Bounds[1].z() = std::max(std::max(V0->z(), V1->z()), V2->z());
        Center = (Bounds[0] + Bounds[1]) / 2;
    }

    bool Face::Hit(const Ray& ray, RayHit& hit)
    {
        hit.T = FLT_MAX;
        Vector3f pvec = ray.Direction.cross(V0V2);
        float det = V0V1.dot(pvec);
        if (std::fabs(det) < 0)
        {            
            return false;
        }

        float invDet = 1.0 / det;
        Vector3f tvec = ray.Origin - (*V0);
        float u = tvec.dot(pvec) * invDet;
        if (u < 0 || u > 1)
            return false;

        Vector3f qvec = tvec.cross(V0V1);
        float v = ray.Direction.dot(qvec) * invDet;
        if (v < 0 || u + v > 1)
            return false;
        float t = V0V2.dot(qvec) * invDet;
        if(t < 1e-2)
        {
            return false;
        }
        hit.T = t;
        hit.Point = ray.Origin + ray.Direction * t;
        if(ray.Direction.dot(Normal) > 0)
        {
            hit.Normal = Normal * -1;            
        }
        else
        {
            hit.Normal = Normal;
        }
        hit.Material = *_material;
        return true;
    }

    int Split(IHittable** hs, int count, float p, int axis)
    {
        int mid = 0;
        for(int i = 0; i < count; i++)
        {
            if(hs[i]->Center(axis) < p)
            {
                auto tmp = hs[i];
                hs[i] = hs[mid];
                hs[mid] = tmp;
                mid++;
            }
        }

        //switch (axis)
        //{
        //case 0:
        //    for(int i = 0; i < count; i++)
        //    {
        //        if(hs[i]->Center.X < p)
        //        {
        //            auto tmp = hs[i];
        //            hs[i] = hs[mid];
        //            hs[mid] = tmp;
        //            mid++;
        //        }
        //    }
        //    break;
        //case 1:
        //    for(int i = 0; i < count; i++)
        //    {
        //        if(hs[i]->Center.Y < p)
        //        {
        //            auto tmp = hs[i];
        //            hs[i] = hs[mid];
        //            hs[mid] = tmp;
        //            mid++;
        //        }
        //    }
        //    break;        
        //case 2:
        //    for(int i = 0; i < count; i++)
        //    {
        //        if(hs[i]->Center.Z < p)
        //        {
        //            auto tmp = hs[i];
        //            hs[i] = hs[mid];
        //            hs[mid] = tmp;
        //            mid++;
        //        }
        //    }
        //    break;
        //default:
        //    break;
        //}
        if(mid == 0 || mid == count) mid = count / 2;
        return mid;
    }

    IHittable* Build(IHittable** hs, int count, int axis)
    {
        if(count == 1) return hs[0];
        if(count == 2) return new AABB(hs[0], hs[1]);
        AABB* aabb = new AABB();
        aabb->Bounds[0] = hs[0]->Bounds[0];
        aabb->Bounds[1] = hs[0]->Bounds[1];
        for(int i = 1; i < count; i++)
        {
            aabb->Bounds[0].x() = std::min(aabb->Bounds[0].x(), hs[i]->Bounds[0].x());
            aabb->Bounds[0].y() = std::min(aabb->Bounds[0].y(), hs[i]->Bounds[0].y());
            aabb->Bounds[0].z() = std::min(aabb->Bounds[0].z(), hs[i]->Bounds[0].z());
            aabb->Bounds[1].x() = std::max(aabb->Bounds[1].x(), hs[i]->Bounds[1].x());
            aabb->Bounds[1].y() = std::max(aabb->Bounds[1].y(), hs[i]->Bounds[1].y());
            aabb->Bounds[1].z() = std::max(aabb->Bounds[1].z(), hs[i]->Bounds[1].z());
        }
        aabb->Center = (aabb->Bounds[0] + aabb->Bounds[1]) / 2;
        float pivot = aabb->Center.x();
        if(axis == 1)
            pivot = aabb->Center.y();
        else if(axis == 2)
            pivot = aabb->Center.z();
        int mid = Split(hs, count, pivot, axis);
        aabb->Left = Build(hs, mid, (axis + 1) % 3);
        aabb->Right = Build(&hs[mid], count - mid, (axis + 1) % 3);
        return aabb;
    }

    AABB::AABB(IHittable* left, IHittable* right)
    {
        Left = left;
        Right = right;
        Bounds[0].x() = std::min(left->Bounds[0].x(), right->Bounds[0].x());
        Bounds[0].y() = std::min(left->Bounds[0].y(), right->Bounds[0].y());
        Bounds[0].z() = std::min(left->Bounds[0].z(), right->Bounds[0].z());        
        Bounds[1].x() = std::max(left->Bounds[1].x(), right->Bounds[1].x());
        Bounds[1].y() = std::max(left->Bounds[1].y(), right->Bounds[1].y());
        Bounds[1].z() = std::max(left->Bounds[1].z(), right->Bounds[1].z()); 
        Center = (Bounds[0] + Bounds[1]) / 2;
    }

    AABB::AABB(IHittable** hs, int count)
    {
        if(count == 1)
        {
            Left = Right = hs[0];
        }
        if(count == 2)
        {
            Left = hs[0];
            Right = hs[1];
        }
        Bounds[0] = hs[0]->Bounds[0];
        Bounds[1] = hs[0]->Bounds[1];
        for(int i = 1; i < count; i++)
        {
            Bounds[0].x() = std::min(Bounds[0].x(), hs[i]->Bounds[0].x());
            Bounds[0].y() = std::min(Bounds[0].y(), hs[i]->Bounds[0].y());
            Bounds[0].z() = std::min(Bounds[0].z(), hs[i]->Bounds[0].z());
            Bounds[1].x() = std::max(Bounds[1].x(), hs[i]->Bounds[1].x());
            Bounds[1].y() = std::max(Bounds[1].y(), hs[i]->Bounds[1].y());
            Bounds[1].z() = std::max(Bounds[1].z(), hs[i]->Bounds[1].z());
        }
        Center = (Bounds[0] + Bounds[1]) / 2;
        if(count > 2)
        {
            int mid = Split(hs, count, Center.x(), 0);
            Left = Build(hs, mid, 1);
            Right = Build(&hs[mid], count - mid, 1);
        }
    }

    bool AABB::Hit(const Ray& ray, RayHit& hit)
    {
        hit.T = FLT_MAX;
        float imin = FLT_MIN;
        float imax = FLT_MAX;
        float t0 = (Bounds[ray.Sign[0]].x() - ray.Origin.x()) * ray.InvDir.x();
        float t1 = (Bounds[1 - ray.Sign[0]].x() - ray.Origin.x()) * ray.InvDir.x();
        if(t0 > imin) imin = t0;
        if(t1 < imax) imax = t1;
        if(imin > imax) return false;

        t0 = (Bounds[ray.Sign[1]].y() - ray.Origin.y()) * ray.InvDir.y();
        t1 = (Bounds[1 - ray.Sign[1]].y() - ray.Origin.y()) * ray.InvDir.y();
        if(t0 > imin) imin = t0;
        if(t1 < imax) imax = t1;
        if(imin > imax) return false;

        t0 = (Bounds[ray.Sign[2]].z() - ray.Origin.z()) * ray.InvDir.z();
        t1 = (Bounds[1 - ray.Sign[2]].z() - ray.Origin.z()) * ray.InvDir.z();
        if(t0 > t1) std::swap(t0, t1);
        if(t0 > imin) imin = t0;
        if(t1 < imax) imax = t1;
        if(imin > imax) return false;

        RayHit lhit, rhit;
        lhit.T = FLT_MAX;
        rhit.T = FLT_MAX;
        bool lh = false; 
        bool rh = false; 
        if(Left != NULL)
            lh = Left->Hit(ray, lhit);
        if(Right != NULL)
            rh = Right->Hit(ray, rhit);

        if(lh)
        {
            hit = lhit;
        }
        if(rh)
        {
            hit = lhit.T < rhit.T ? lhit : rhit;
        }
        return lh || rh;
    }

}