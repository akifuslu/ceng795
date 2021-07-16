#include "objectlight.h"

namespace raytracer
{
    LightSphere::LightSphere(pugi::xml_node node) : Light(node), Sphere(node)
    {
        Radiance = Vec3fFrom(node.child("Radiance"));
    }

    LightMesh::LightMesh(pugi::xml_node node) : Light(node), Mesh(node)
    {
        Radiance = Vec3fFrom(node.child("Radiance"));
    }

    void LightMesh::Load(Scene& scene)
    {
        Mesh::Load(scene);
        totalArea = 0;
        std::vector<float> probs;
        for(int i = 0; i < _fCount; i++)
        {
            float a = _faces[i]->GetArea(LocalToWorld);
            probs.push_back(a);
            totalArea += a;
        }
        rnd = std::discrete_distribution<int>(probs.begin(), probs.end());
    }


    float LightSphere::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir, Vector3f& lnormal)
    {
        auto plocal = WorldToLocal * point;
        float r = Radius;
        float d = (_center - plocal).norm();
        float rd = r / d;
        rd *= rd;
        rd = rd > 1 ? 1 : rd;
        float costhetamax = std::sqrt(1 - rd);
        float r1 = rnd(generator);
        float r2 = rnd(generator);
        float thetai = std::acos(1 - r1 + r1 * costhetamax);
        float phii = 2 * M_PI * r2;
        auto w = (_center - plocal).normalized();
        
        float x = std::fabs(w.x());
        float y = std::fabs(w.y());
        float z = std::fabs(w.z());
        Vector3f np;
        if(x <= y && x <= z) np = Vector3f(1, w.y(), w.z());
        else if(y <= x && y <= z) np = Vector3f(w.x(), 1, w.z());
        else np = Vector3f(w.x(), w.y(), 1);
        np.normalize();
        auto u = np.cross(w).normalized();
        auto v = w.cross(u).normalized();

        Vector3f llocal = w * std::cos(thetai)
            + v * std::sin(thetai) * std::cos(phii)
            + u * std::sin(thetai) * std::sin(phii);

        llocal.normalize();
        dir = (LocalToWorld.linear() * llocal).normalized();
        Vector3f sp = _center - llocal * Radius;
        sp = LocalToWorld * sp;        
        return (sp - point).norm();
    }

    float LightMesh::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir, Vector3f& lnormal)
    {
        int tri = rnd(generator);
        float r1 = urnd(generator);
        float r2 = urnd(generator);
        Vector3f lp = _faces[tri]->SamplePoint(r1, r2);
        lnormal = (LocalToWorld.linear() * _faces[tri]->Normal).normalized();
        sample = LocalToWorld * lp;
        dir = (sample - point).normalized();
        return (sample - point).norm();
    }


    Vector3f LightSphere::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        auto plocal = WorldToLocal * point;
        float r = Radius;
        float d = (_center - plocal).norm();
        float rd = r / d;
        rd *= rd;
        rd = rd > 1 ? 1 : rd;
        float costhetamax = std::sqrt(1 - rd);
        return Radiance * (2 * M_PI * (1 - costhetamax));
    }   

    Vector3f LightMesh::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        float r = (point - lsample).squaredNorm();
        float theta = (point - lsample).normalized().dot(normal);
        if(theta <= 0)
            theta *= -1;            
        return Radiance * totalArea * theta / r;
    }

    bool LightSphere::Hit(const Ray& ray, RayHit& hit)
    {
        if(ray.Ignore == Id)
            return false;
        return Sphere::Hit(ray, hit);
    }

    bool LightMesh::Hit(const Ray& ray, RayHit& hit)
    {
        if(ray.Ignore == Id)
            return false;
        return Mesh::Hit(ray, hit);
    }    
}