#include "light.h"

namespace raytracer
{
    AmbientLight::AmbientLight(pugi::xml_node node)
    {
        Intensity = Vec3fFrom(node);
    }    

    Light::Light(pugi::xml_node node)
    {
        Position = Vec3fFrom(node.child("Position"));
    }

    PointLight::PointLight(pugi::xml_node node) : Light(node)
    {
        Intensity = Vec3fFrom(node.child("Intensity"));
    }

    AreaLight::AreaLight(pugi::xml_node node) : Light(node)
    {
        Normal = Vec3fFrom(node.child("Normal"));
        Radiance = Vec3fFrom(node.child("Radiance"));
        Size = node.child("Size").text().as_float();
        float x = std::fabs(Normal.x());
        float y = std::fabs(Normal.y());
        float z = std::fabs(Normal.z());
        Vector3f np;
        if(x <= y && x <= z) np = Vector3f(1, Normal.y(), Normal.z());
        else if(y <= x && y <= z) np = Vector3f(Normal.x(), 1, Normal.z());
        else np = Vector3f(Normal.x(), Normal.y(), 1);
        np.normalize();
        u = np.cross(Normal).normalized();
        v = Normal.cross(u).normalized();
    }

    Vector3f PointLight::SamplePoint()
    {
        return Position;
    }

    Vector3f AreaLight::SamplePoint()
    {
        float r1 = rnd(generator) - .5f;
        float r2 = rnd(generator) - .5f;
        return Position + Size * (u * (r1) + v * (r2));
    }

    Vector3f PointLight::GetLuminance(Vector3f point, Vector3f lsample)
    {
        float r = (Position - point).norm();
        return Intensity / (r * r);
    }

    Vector3f AreaLight::GetLuminance(Vector3f point, Vector3f lsample)
    {
        float r = (lsample - point).norm();
        Vector3f l = (point - lsample).normalized();
        float teta = Normal.dot(l);
        if(teta < 0) teta *= -1;
        return (Radiance * teta * Size * Size) / (r * r);
    }
}