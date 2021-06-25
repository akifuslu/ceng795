#include "object.h"
#include <sstream>
#include <cfloat>
#include <cmath>
#include <string>
#include "scene.h"
#include "plyLoader.h"

namespace raytracer
{

    Object::Object(pugi::xml_node node)
    {
        Id = node.attribute("id").as_int();
        MaterialId = node.child("Material").text().as_int();
        Transformations = node.child("Transformations").text().as_string();    
        if(node.child("MotionBlur"))
        {
            MotionBlur = Vec3fFrom(node.child("MotionBlur"));
        }    
        else
        {
            MotionBlur = Vector3f::Zero();
        }        
        _texIds[0] = _texIds[1] = 0;
        if(node.child("Textures"))
        {
            std::stringstream ss;
            ss << node.child("Textures").text().as_string();
            ss >> _texIds[0] >> _texIds[1];
        }
    }

    std::ostream& Object::Print(std::ostream& os) const
    {
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Object& obj)
    {
        return obj.Print(os);
    }

    void Object::Load(Scene& scene)
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
            else if(type == 'c')
            {
                LocalToWorld = scene.Composite[id - 1];
            } 
        }
        WorldToLocal = LocalToWorld.inverse(TransformTraits::Affine);
        // map textures
        for(int i = 0; i < 2; i++)
        {
            if(_texIds[i] != 0)
            {
                Texture* tex = scene.Textures[_texIds[i]];
                auto diff = dynamic_cast<DiffuseTexture*>(tex);
                if(diff != nullptr)
                {
                    DiffuseMap = diff;
                    continue;
                }
                auto bump = dynamic_cast<BumpTexture*>(tex);
                if(bump != nullptr)
                {
                    BumpMap = bump;
                    continue;
                }
                auto normal = dynamic_cast<NormalTexture*>(tex);
                if(normal != nullptr)
                {
                    NormalMap = normal;
                    continue;
                }
            }
        }
    }   

    Mesh::Mesh(pugi::xml_node node) : Object(node)
    {
        const char* ply = node.child("Faces").attribute("plyFile").as_string();
        _offset = node.child("Faces").attribute("vertexOffset").as_int(0);        
        _tOffset = node.child("Faces").attribute("textureOffset").as_int(0);        
        auto shd = node.attribute("shadingMode").as_string();
        if(std::strcmp(shd, "smooth") == 0)
            _smooth = true;
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
            auto tris = load_trimesh_from_ply(ply);
            auto vPos = tris->pos;
            auto uvs = tris->uv;
            auto fInd = tris->indices;
            _fCount = tris->numIndices / 3;
            _faces = new Face*[_fCount];
            int f = 0;
            for(int i = 0; i < _fCount; i++)
            {
                Vector2f* uv0 = nullptr;
                Vector2f* uv1 = nullptr;
                Vector2f* uv2 = nullptr;
                if(uvs != nullptr)
                {
                    uv0 = new Vector2f(uvs[fInd[i * 3] * 2], uvs[fInd[i * 3] * 2 + 1]);
                    uv1 = new Vector2f(uvs[fInd[i * 3 + 1] * 2], uvs[fInd[i * 3 + 1] * 2 + 1]);
                    uv2 = new Vector2f(uvs[fInd[i * 3 + 2] * 2], uvs[fInd[i * 3 + 2] * 2 + 1]);
                }
                else
                {
                    uv0 = uv1 = uv2 = new Vector2f(0, 0);
                }                
                Faces.push_back(Vector3i(fInd[i * 3 + 0], fInd[i * 3 + 1], fInd[i * 3 + 2]));
                auto v0 = new Vector3f(vPos[fInd[i * 3 + 0] * 3], vPos[fInd[i * 3 + 0] * 3 + 1], vPos[fInd[i * 3 + 0] * 3 + 2]);
                auto v1 = new Vector3f(vPos[fInd[i * 3 + 1] * 3], vPos[fInd[i * 3 + 1] * 3 + 1], vPos[fInd[i * 3 + 1] * 3 + 2]);
                auto v2 = new Vector3f(vPos[fInd[i * 3 + 2] * 3], vPos[fInd[i * 3 + 2] * 3 + 1], vPos[fInd[i * 3 + 2] * 3 + 2]);
                _faces[f++] = new Face(v0, v1, v2, &_material, uv0, uv1, uv2);
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

    void Mesh::Load(Scene& scene)
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
                auto v0 = new Vector3f(scene.VertexData[Faces[i].x() - 1 + _offset]);
                auto v1 = new Vector3f(scene.VertexData[Faces[i].y() - 1 + _offset]);
                auto v2 = new Vector3f(scene.VertexData[Faces[i].z() - 1 + _offset]);
                Vector2f* uv0 = new Vector2f(0,0);
                Vector2f* uv1 = uv0;
                Vector2f* uv2 = uv0;
                if(scene.UVData.size() > (Faces[i].x() - 1 + _tOffset)
                    && scene.UVData.size() > (Faces[i].y() - 1 + _tOffset)
                    && scene.UVData.size() > (Faces[i].z() - 1 + _tOffset))
                {
                    uv0 = new Vector2f(scene.UVData[Faces[i].x() - 1 + _tOffset]);
                    uv1 = new Vector2f(scene.UVData[Faces[i].y() - 1 + _tOffset]);
                    uv2 = new Vector2f(scene.UVData[Faces[i].z() - 1 + _tOffset]);
                }
                _faces[i] = new Face(v0, v1, v2, &_material, uv0, uv1, uv2);                
            }
        }
        if(_smooth)
        {
            for(int i = 0; i < Faces.size(); i++)
            {
                int v0id = Faces[i].x() - 1 + _offset;
                int v1id = Faces[i].y() - 1 + _offset;
                int v2id = Faces[i].z() - 1 + _offset;
                Vector3f v0N = Vector3f::Zero();
                Vector3f v1N = Vector3f::Zero();
                Vector3f v2N = Vector3f::Zero();
                for(int j = 0; j < Faces.size(); j++)
                {
                    int v0id2 = Faces[j].x() - 1 + _offset;
                    int v1id2 = Faces[j].y() - 1 + _offset;
                    int v2id2 = Faces[j].z() - 1 + _offset;
                    if(v0id == v0id2 || v0id == v1id2 || v0id == v2id2)
                        v0N = v0N + _faces[j]->Normal;
                    if(v1id == v0id2 || v1id == v1id2 || v1id == v2id2)
                        v1N = v1N + _faces[j]->Normal;
                    if(v2id == v0id2 || v2id == v1id2 || v2id == v2id2)
                        v2N = v2N + _faces[j]->Normal;
                }
                _faces[i]->V0N = v0N.normalized();
                _faces[i]->V1N = v1N.normalized();
                _faces[i]->V2N = v2N.normalized();
                _faces[i]->smooth = true;
            }
        }            

        bvh = new BVH((IHittable**)_faces, _fCount);
        aabb = AABB(bvh->aabb);
        auto ltw = LocalToWorld;
        float sx = std::fabs(MotionBlur.x()) + 1;
        float sy = std::fabs(MotionBlur.y()) + 1;
        float sz = std::fabs(MotionBlur.z()) + 1;        
        ltw.scale(Vector3f(sx, sy, sz));
        aabb.ApplyTransform(ltw);        
    }   

    bool Mesh::Hit(const Ray& wray, RayHit& hit)
    {
        if(!aabb.Intersect(wray))
        {
            return false;
        }
        auto wtl = WorldToLocal;
        wtl.translate(MotionBlur * -wray.Time); 
        Ray ray(wtl * wray.Origin, (wtl.linear() * wray.Direction).normalized());
        bool ret = bvh->Hit(ray, hit);
        hit.Object = this;
        hit.Material = _material;
        hit.Texture = DiffuseMap;
        if(NormalMap != nullptr)
        {
            SamplerData data;
            data.u = hit.u;
            data.v = hit.v;
            hit.Normal = hit.TBN * NormalMap->SampleNormal(data);
        }
        else if(BumpMap != nullptr)
        {
            SamplerData data;
            data.u = hit.u;
            data.v = hit.v;
            data.point = hit.Point;
            data.normal = hit.Normal;
            hit.Normal = BumpMap->SampleBump(data, hit.TBN);
        }
        auto ltw = LocalToWorld;
        ltw.pretranslate(MotionBlur * wray.Time);
        hit.Point = ltw * hit.Point;
        hit.Normal = (ltw.linear().inverse().transpose() * hit.Normal).normalized();

        if(ret){
            hit.T = (hit.Point - wray.Origin).norm();
        }
        else
        {
            hit.T = FLT_MAX;
        }        
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

    void Triangle::Load(Scene& scene)
    {
        Object::Load(scene);
        auto v0 = new Vector3f(scene.VertexData[Indices.x() - 1]);
        auto v1 = new Vector3f(scene.VertexData[Indices.y() - 1]);
        auto v2 = new Vector3f(scene.VertexData[Indices.z() - 1]);
        auto u = new Vector2f(0, 0);
        _face = Face(v0, v1, v2, &_material, u, u, u);        
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
        hit.Texture = DiffuseMap;
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

    void Sphere::Load(Scene& scene)
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
        auto ltw = LocalToWorld;
        float sx = MotionBlur.x() == 0 ? 1 : MotionBlur.x();
        float sy = MotionBlur.y() == 0 ? 1 : MotionBlur.y();
        float sz = MotionBlur.z() == 0 ? 1 : MotionBlur.z();        
        ltw.scale(Vector3f(sx, sy, sz));
        aabb.ApplyTransform(ltw);        
    }   
    
    bool Sphere::Hit(const Ray& wray, RayHit& hit)
    {
        auto wtl = WorldToLocal;
        wtl.translate(MotionBlur * -wray.Time); 
        Ray ray(wtl * wray.Origin, (wtl.linear() * wray.Direction).normalized());
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
            auto p = hit.Normal;

            // calc uv
            float ut = std::acos(p.y() / 1); // theta
            float us = std::atan2(p.z(), p.x()); // phi
            hit.u = (-us + M_PI) / (2 * M_PI);
            hit.v = ut / M_PI;

            p = hit.Point - _center;
            ut = std::acos(p.y() / Radius); // theta
            us = std::atan2(p.z(), p.x()); // phi

            Vector3f T = Vector3f(p.z() * 2 * M_PI, 0, p.x() * (-2) * M_PI);             
            Vector3f B = Vector3f(p.y() * std::cos(us) * M_PI, -Radius * std::sin(ut) * M_PI, p.y() * std::sin(us) * M_PI);
            if(NormalMap != nullptr){
                T.normalize();
                B.normalize();
            }

            hit.TBN(0, 0) = T.x(); hit.TBN(0, 1) = B.x(); hit.TBN(0, 2) = hit.Normal.x();
            hit.TBN(1, 0) = T.y(); hit.TBN(1, 1) = B.y(); hit.TBN(1, 2) = hit.Normal.y();
            hit.TBN(2, 0) = T.z(); hit.TBN(2, 1) = B.z(); hit.TBN(2, 2) = hit.Normal.z();
            if(NormalMap != nullptr)
            {
                SamplerData data;
                data.u = hit.u; data.v = hit.v;
                hit.Normal = hit.TBN * NormalMap->SampleNormal(data);                
            }
            else if(BumpMap != nullptr)
            {
                SamplerData data;
                data.u = hit.u; data.v = hit.v;
                data.point = hit.Point;
                data.normal = hit.Normal;
                hit.Normal = BumpMap->SampleBump(data, hit.TBN).normalized();                
            }
            auto ltw = LocalToWorld;
            ltw.pretranslate(MotionBlur * wray.Time);
            hit.Point = ltw * hit.Point;
            hit.Normal = (ltw.linear().inverse().transpose() * hit.Normal).normalized();
            hit.Object = this;
            hit.Material = _material;
            hit.Texture = DiffuseMap;
            hit.T = (hit.Point - wray.Origin).norm();
            return true;
        }
    }

    Face::Face() 
    {    }
    
    Face::Face(Vector3f* v0, Vector3f* v1, Vector3f* v2, Material* material, Vector2f* uv0, Vector2f* uv1, Vector2f* uv2)
        : V0(v0), V1(v1), V2(v2), UV0(uv0), UV1(uv1), UV2(uv2)
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

        Matrix<float, 2, 3> e;
        e(0,0) = V0V1.x(); e(0,1) = V0V1.y(); e(0,2) = V0V1.z();
        e(1,0) = V0V2.x(); e(1,1) = V0V2.y(); e(1,2) = V0V2.z();
        Matrix<float, 2, 2> u;
        u(0, 0) = (*UV1).x() - (*UV0).x(); u(0, 1) = (*UV1).y() - (*UV0).y();
        u(1, 0) = (*UV2).x() - (*UV0).x(); u(1, 1) = (*UV2).y() - (*UV0).y();
        u = u.inverse().eval();
        Matrix<float, 2, 3> tb;
        tb = u * e;
        auto t = Vector3f(tb(0,0), tb(0,1), tb(0,2));
        auto b = Vector3f(tb(1,0), tb(1,1), tb(1,2));
        TBN(0, 0) = t.x(); TBN(0, 1) = b.x(); TBN(0, 2) = Normal.x();
        TBN(1, 0) = t.y(); TBN(1, 1) = b.y(); TBN(1, 2) = Normal.y();
        TBN(2, 0) = t.z(); TBN(2, 1) = b.z(); TBN(2, 2) = Normal.z();
    }

    bool Face::Hit(const Ray& ray, RayHit& hit)
    {
        hit.T = FLT_MAX;
        Vector3f pvec = ray.Direction.cross(V0V2);
        float det = V0V1.dot(pvec);
        if (std::fabs(det) < 1e-9)
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
        if(t < 0)
        {
            return false;
        }
        hit.T = t;
        hit.Point = (*V0) + u * ((*V1) - (*V0)) + v * ((*V2) - (*V0));
        //hit.Point = ray.Origin + ray.Direction * t;
        auto n = Normal;
        if(smooth)
        {
            n = V0N + u * (V1N - V0N) + v * (V2N - V0N);
            n.normalize();
        }
        if(ray.Direction.dot(n) > 0)
        {
            hit.Normal = n * -1;            
        }
        else
        {
            hit.Normal = n;
        }
        hit.Material = *_material;
        auto uv = (*UV0) + u * ((*UV1) - (*UV0)) + v * ((*UV2) - (*UV0));
        hit.u = uv.x();
        hit.v = uv.y();
        hit.TBN = TBN;
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
        Center = (Bounds[0] + Bounds[1]) / 2;
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

    void MeshInstance::Load(Scene& scene)
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
        auto ltw = LocalToWorld;
        float sx = MotionBlur.x() == 0 ? 1 : MotionBlur.x();
        float sy = MotionBlur.y() == 0 ? 1 : MotionBlur.y();
        float sz = MotionBlur.z() == 0 ? 1 : MotionBlur.z();        
        ltw.scale(Vector3f(sx, sy, sz));
        aabb.ApplyTransform(ltw);       
    }

    bool MeshInstance::Hit(const Ray& wray, RayHit& hit)
    {
        if(!aabb.Intersect(wray))
        {
            return false;
        }
        auto wtl = WorldToLocal;
        wtl.translate(MotionBlur * -wray.Time); 
        Ray ray(wtl * wray.Origin, (wtl.linear() * wray.Direction).normalized());
        bool ret = bvh->Hit(ray, hit);
        auto ltw = LocalToWorld;
        ltw.pretranslate(MotionBlur * wray.Time);
        hit.Point = ltw * hit.Point;
        hit.Normal = (ltw.linear().inverse().transpose() * hit.Normal).normalized();
        hit.Object = this;
        hit.Material = _material;
        hit.Texture = DiffuseMap;
        if(ret){
            hit.T = (hit.Point - wray.Origin).norm();
        }
        else
        {
            hit.T = FLT_MAX;
        }        
        return ret;
    }
}