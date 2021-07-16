#include "objectlight.h"

namespace raytracer
{
    LightSphere::LightSphere(pugi::xml_node node) : Light(node), Sphere(node)
    {
        Radiance = Vec3fFrom(node.child("Radiance"));
    }

    float LightSphere::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
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

    bool LightSphere::Hit(const Ray& ray, RayHit& hit)
    {
        if(ray.Ignore == Id)
            return false;
        return Sphere::Hit(ray, hit);
    }
 
}