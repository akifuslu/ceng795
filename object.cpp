#include "object.h"
#include <sstream>
#include <cfloat>
#include <cmath>
#include "happly.h"
#include <string>
#include "scene.h"

namespace raytracer
{

    Object::Object(pugi::xml_node node)
    {
        Id = node.attribute("id").as_int();
        MaterialId = node.child("Material").text().as_int();
        Transformations = node.child("Transformations").text().as_string();        
    }

    std::ostream& Object::Print(std::ostream& os) const
    {
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Object& obj)
    {
        return obj.Print(os);
    }

    void Object::Load(const Scene& scene)
    {
        _material = scene.Materials[MaterialId - 1];
        LocalToWorld = Transform<float, 3, Affine>::Identity();
        std::stringstream ss;
        ss << Transformations;
        char type;
        int id;
        while(ss >> type >> id)
        {
            if(type == 't')
            {
                LocalToWorld = scene.Translations[id - 1] * LocalToWorld;
            }
            else if(type == 's')
            {
                LocalToWorld = scene.Scalings[id - 1] * LocalToWorld;
            }
            else if(type == 'r')
            {
                LocalToWorld = scene.Rotations[id - 1] * LocalToWorld;
            }            
        }
        //std::cout << LocalToWorld(0,0) << ',' << LocalToWorld(0,1) << ',' << LocalToWorld(0,2) << ',' << LocalToWorld(0,3) << std::endl;
        //std::cout << LocalToWorld(1,0) << ',' << LocalToWorld(1,1) << ',' << LocalToWorld(1,2) << ',' << LocalToWorld(1,3) << std::endl;
        //std::cout << LocalToWorld(2,0) << ',' << LocalToWorld(2,1) << ',' << LocalToWorld(2,2) << ',' << LocalToWorld(2,3) << std::endl;
        //std::cout << LocalToWorld(3,0) << ',' << LocalToWorld(3,1) << ',' << LocalToWorld(3,2) << ',' << LocalToWorld(3,3) << std::endl;
        //std::cout << "------------" << std::endl;
        WorldToLocal = LocalToWorld.inverse(TransformTraits::Affine);
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
            _faces = new Face*[fInd.size()];
            _fCount = fInd.size();
            for(int i = 0; i < fInd.size(); i++)
            {
                auto v0 = new Vector3f(vPos[fInd[i][0]][0], vPos[fInd[i][0]][1], vPos[fInd[i][0]][2]);
                auto v1 = new Vector3f(vPos[fInd[i][1]][0], vPos[fInd[i][1]][1], vPos[fInd[i][1]][2]);
                auto v2 = new Vector3f(vPos[fInd[i][2]][0], vPos[fInd[i][2]][1], vPos[fInd[i][2]][2]);
                _faces[i] = new Face(v0, v1, v2, &_material);
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

    void Mesh::Load(const Scene& scene)
    {
        Object::Load(scene);
        if(_ply)
        {
            for(int i = 0; i < _fCount; i++)
            {
                _faces[i]->_material = &_material;
            }
        }
        else
        {        
            _fCount = Faces.size();
            _faces = new Face*[Faces.size()];
            for(int i = 0; i < Faces.size(); i++)
            {
                auto v0 = new Vector3f(scene.VertexData[Faces[i].x() - 1]);
                auto v1 = new Vector3f(scene.VertexData[Faces[i].y() - 1]);
                auto v2 = new Vector3f(scene.VertexData[Faces[i].z() - 1]);
                _faces[i] = new Face(v0, v1, v2, &_material);
            }            
        }
        bvh = new BVH((IHittable**)_faces, _fCount);
        aabb = AABB(bvh->aabb);
        aabb.ApplyTransform(LocalToWorld);
    }   

    bool Mesh::Hit(const Ray& wray, RayHit& hit)
    {
        if(!aabb.Intersect(wray))
        {
            return false;
        }
        Ray ray(WorldToLocal * wray.Origin, WorldToLocal.linear() * wray.Direction);
        bool ret = bvh->Hit(ray, hit);
        hit.Point = LocalToWorld * hit.Point;
        hit.Normal = (LocalToWorld.linear().inverse().transpose() * hit.Normal).normalized();
        hit.Object = this;
        return ret;
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

    void Triangle::Load(const Scene& scene)
    {
        Object::Load(scene);
        auto v0 = new Vector3f(scene.VertexData[Indices.x() - 1]);
        auto v1 = new Vector3f(scene.VertexData[Indices.y() - 1]);
        auto v2 = new Vector3f(scene.VertexData[Indices.z() - 1]);
        _face = Face(v0, v1, v2, &_material);        
        aabb = AABB(_face.aabb);
        aabb.ApplyTransform(LocalToWorld);
    }   

    bool Triangle::Hit(const Ray& wray, RayHit& hit)
    {
        if(!aabb.Intersect(wray))
        {
            return false;
        }
        Ray ray(WorldToLocal * wray.Origin, WorldToLocal.linear() * wray.Direction);
        bool ret = _face.Hit(ray, hit);
        hit.Point = LocalToWorld * hit.Point;
        hit.Normal = (LocalToWorld.linear().inverse().transpose() * hit.Normal).normalized();
        return ret;
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

    void Sphere::Load(const Scene& scene)
    {
        Object::Load(scene);
        _center = scene.VertexData[CenterId - 1];
        aabb.Bounds[0].x() = _center.x() - Radius;
        aabb.Bounds[0].y() = _center.y() - Radius;
        aabb.Bounds[0].z() = _center.z() - Radius;
        aabb.Bounds[1].x() = _center.x() + Radius;
        aabb.Bounds[1].y() = _center.y() + Radius;
        aabb.Bounds[1].z() = _center.z() + Radius;
        aabb.Center = (aabb.Bounds[0] + aabb.Bounds[1]) / 2; 
        aabb.ApplyTransform(LocalToWorld);
    }   
    
    bool Sphere::Hit(const Ray& wray, RayHit& hit)
    {
        Ray ray(WorldToLocal * wray.Origin, WorldToLocal.linear() * wray.Direction);
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

            hit.Point = LocalToWorld * hit.Point;
            hit.Normal = (LocalToWorld.linear().inverse().transpose() * hit.Normal).normalized();
            hit.Material = _material;
            return true;
        }
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
        aabb.Bounds[0].x() = std::min(std::min(V0->x(), V1->x()), V2->x());
        aabb.Bounds[0].y() = std::min(std::min(V0->y(), V1->y()), V2->y());
        aabb.Bounds[0].z() = std::min(std::min(V0->z(), V1->z()), V2->z());
        aabb.Bounds[1].x() = std::max(std::max(V0->x(), V1->x()), V2->x());
        aabb.Bounds[1].y() = std::max(std::max(V0->y(), V1->y()), V2->y());
        aabb.Bounds[1].z() = std::max(std::max(V0->z(), V1->z()), V2->z());
        aabb.Center = (aabb.Bounds[0] + aabb.Bounds[1]) / 2;
    }

    bool Face::Hit(const Ray& ray, RayHit& hit)
    {
        hit.T = FLT_MAX;
        Vector3f pvec = ray.Direction.cross(V0V2);
        float det = V0V1.dot(pvec);
        if (std::fabs(det) < 0.000001f)
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
            if(hs[i]->aabb.Center(axis) < p)
            {
                auto tmp = hs[i];
                hs[i] = hs[mid];
                hs[mid] = tmp;
                mid++;
            }
        }
        if(mid == 0 || mid == count) mid = count / 2;
        return mid;
    }

    IHittable* Build(IHittable** hs, int count, int axis)
    {
        if(count == 1) return hs[0];
        if(count == 2) return new BVH(hs[0], hs[1]);
        BVH* bvh = new BVH();
        bvh->aabb.Bounds[0] = hs[0]->aabb.Bounds[0];
        bvh->aabb.Bounds[1] = hs[0]->aabb.Bounds[1];
        for(int i = 1; i < count; i++)
        {
            bvh->aabb.Bounds[0].x() = std::min(bvh->aabb.Bounds[0].x(), hs[i]->aabb.Bounds[0].x());
            bvh->aabb.Bounds[0].y() = std::min(bvh->aabb.Bounds[0].y(), hs[i]->aabb.Bounds[0].y());
            bvh->aabb.Bounds[0].z() = std::min(bvh->aabb.Bounds[0].z(), hs[i]->aabb.Bounds[0].z());
            bvh->aabb.Bounds[1].x() = std::max(bvh->aabb.Bounds[1].x(), hs[i]->aabb.Bounds[1].x());
            bvh->aabb.Bounds[1].y() = std::max(bvh->aabb.Bounds[1].y(), hs[i]->aabb.Bounds[1].y());
            bvh->aabb.Bounds[1].z() = std::max(bvh->aabb.Bounds[1].z(), hs[i]->aabb.Bounds[1].z());
        }
        bvh->aabb.Center = (bvh->aabb.Bounds[0] + bvh->aabb.Bounds[1]) / 2;
        float pivot = bvh->aabb.Center.x();
        if(axis == 1)
            pivot = bvh->aabb.Center.y();
        else if(axis == 2)
            pivot = bvh->aabb.Center.z();
        int mid = Split(hs, count, pivot, axis);
        bvh->Left = Build(hs, mid, (axis + 1) % 3);
        bvh->Right = Build(&hs[mid], count - mid, (axis + 1) % 3);
        return bvh;
    }

    BVH::BVH(IHittable* left, IHittable* right)
    {
        Left = left;
        Right = right;
        aabb.Bounds[0].x() = std::min(left->aabb.Bounds[0].x(), right->aabb.Bounds[0].x());
        aabb.Bounds[0].y() = std::min(left->aabb.Bounds[0].y(), right->aabb.Bounds[0].y());
        aabb.Bounds[0].z() = std::min(left->aabb.Bounds[0].z(), right->aabb.Bounds[0].z());        
        aabb.Bounds[1].x() = std::max(left->aabb.Bounds[1].x(), right->aabb.Bounds[1].x());
        aabb.Bounds[1].y() = std::max(left->aabb.Bounds[1].y(), right->aabb.Bounds[1].y());
        aabb.Bounds[1].z() = std::max(left->aabb.Bounds[1].z(), right->aabb.Bounds[1].z()); 
        aabb.Center = (aabb.Bounds[0] + aabb.Bounds[1]) / 2;
    }

    BVH::BVH(IHittable** hs, int count)
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
        aabb.Bounds[0] = hs[0]->aabb.Bounds[0];
        aabb.Bounds[1] = hs[0]->aabb.Bounds[1];
        for(int i = 1; i < count; i++)
        {
            aabb.Bounds[0].x() = std::min(aabb.Bounds[0].x(), hs[i]->aabb.Bounds[0].x());
            aabb.Bounds[0].y() = std::min(aabb.Bounds[0].y(), hs[i]->aabb.Bounds[0].y());
            aabb.Bounds[0].z() = std::min(aabb.Bounds[0].z(), hs[i]->aabb.Bounds[0].z());
            aabb.Bounds[1].x() = std::max(aabb.Bounds[1].x(), hs[i]->aabb.Bounds[1].x());
            aabb.Bounds[1].y() = std::max(aabb.Bounds[1].y(), hs[i]->aabb.Bounds[1].y());
            aabb.Bounds[1].z() = std::max(aabb.Bounds[1].z(), hs[i]->aabb.Bounds[1].z());
        }
        aabb.Center = (aabb.Bounds[0] + aabb.Bounds[1]) / 2;
        if(count > 2)
        {
            int mid = Split(hs, count, aabb.Center.x(), 0);
            Left = Build(hs, mid, 1);
            Right = Build(&hs[mid], count - mid, 1);
        }
    }

    bool AABB::Intersect(const Ray& ray)
    {
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

        return true;
    }

    void AABB::ApplyTransform(const Transform<float, 3, Affine>& transform)
    {
        float xdiff = Bounds[1].x() - Bounds[0].x();
        float ydiff = Bounds[1].y() - Bounds[0].y();
        float zdiff = Bounds[1].z() - Bounds[0].z();                
        Vector3f corners[8];
        corners[0] = Bounds[0];
        corners[1] = Vector3f(Bounds[0].x() + xdiff, Bounds[0].y(), Bounds[0].z());                 
        corners[2] = Vector3f(Bounds[0].x(), Bounds[0].y() + ydiff, Bounds[0].z());
        corners[3] = Vector3f(Bounds[0].x(), Bounds[0].y(), Bounds[0].z() + zdiff);
        corners[4] = Vector3f(Bounds[0].x() + xdiff, Bounds[0].y() + ydiff, Bounds[0].z());                 
        corners[5] = Vector3f(Bounds[0].x(), Bounds[0].y() + ydiff, Bounds[0].z() + zdiff);
        corners[6] = Vector3f(Bounds[0].x() + xdiff, Bounds[0].y(), Bounds[0].z() + zdiff);
        corners[7] = Bounds[1];
        for(int i = 0; i < 8; i++)
        {
            corners[i] = transform * corners[i];
            for(int j = 0; j < 3; j++)
            {
                if(corners[i](j) < Bounds[0](j))
                    Bounds[0](j) = corners[i](j);
                if(corners[i](j) > Bounds[1](j))
                    Bounds[1](j) = corners[i](j);
            }
        }
    }


    bool BVH::Hit(const Ray& ray, RayHit& hit)
    {
        if(!aabb.Intersect(ray))
        {
            return false;
        }
        hit.T = FLT_MAX;
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

    MeshInstance::MeshInstance(pugi::xml_node node) : Object(node)
    {   
        BaseMeshId = node.attribute("baseMeshId").as_int();
        ResetTransform = node.attribute("resetTransform").as_bool();
    }

    void MeshInstance::Load(const Scene& scene)
    {
        Object::Load(scene);
        for(int i = 0; i < scene.Objects.size(); i++)
        {
            if(scene.Objects[i]->Id == BaseMeshId)
            {
                bvh = ((Mesh*)scene.Objects[i])->bvh;
                if(!ResetTransform)
                {
                    auto& bt = scene.Objects[i]->LocalToWorld;
                    LocalToWorld = LocalToWorld * bt;
                    WorldToLocal = LocalToWorld.inverse();
                }
                break;
            }
        }
        aabb = AABB(bvh->aabb);
        aabb.ApplyTransform(LocalToWorld);        
    }

    bool MeshInstance::Hit(const Ray& wray, RayHit& hit)
    {
        if(!aabb.Intersect(wray))
        {
            return false;
        }
        Ray ray(WorldToLocal * wray.Origin, WorldToLocal.linear() * wray.Direction);
        bool ret = bvh->Hit(ray, hit);
        hit.Point = LocalToWorld * hit.Point;
        hit.Normal = (LocalToWorld.linear().inverse().transpose() * hit.Normal).normalized();
        hit.Object = this;
        hit.Material = _material;
        return ret;
    }
}