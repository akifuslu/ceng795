#include "light.h"
#include <cfloat>

namespace raytracer
{
    AmbientLight::AmbientLight(pugi::xml_node node)
    {
        Intensity = Vec3fFrom(node);
    }    

    Light::Light(pugi::xml_node node)
    {

    }

    PointLight::PointLight(pugi::xml_node node) : Light(node)
    {
        Position = Vec3fFrom(node.child("Position"));
        Intensity = Vec3fFrom(node.child("Intensity"));
    }

    AreaLight::AreaLight(pugi::xml_node node) : Light(node)
    {
        Position = Vec3fFrom(node.child("Position"));
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

    DirectionalLight::DirectionalLight(pugi::xml_node node) : Light(node)
    {
        Direction = Vec3fFrom(node.child("Direction")).normalized();
        Radiance = Vec3fFrom(node.child("Radiance"));
    }

    SpotLight::SpotLight(pugi::xml_node node) : Light(node)
    {
        Position = Vec3fFrom(node.child("Position"));
        Direction = Vec3fFrom(node.child("Direction")).normalized();
        Intensity = Vec3fFrom(node.child("Intensity"));
        CoverageAngle = node.child("CoverageAngle").text().as_float(1);
        FalloffAngle = node.child("FalloffAngle").text().as_float(1);
    }

    EnvironmentLight::EnvironmentLight(pugi::xml_node node) : Light(node)
    {
        int imgId = node.child("ImageId").text().as_int();
        _hdr = ImageLocator::GetInstance().GetImage(imgId);
    }

    float PointLight::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
    {
        sample = Position;
        dir = sample - point;
        float r = dir.norm();
        dir.normalize();
        return r;
    }

    float AreaLight::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
    {
        float r1 = rnd(generator) - .5f;
        float r2 = rnd(generator) - .5f;
        sample = Position + Size * (u * (r1) + v * (r2));
        dir = sample - point;
        float r = dir.norm();
        dir.normalize();
        return r;
    }

    float DirectionalLight::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
    {
        sample = Vector3f::Zero();
        dir = -Direction;
        return FLT_MAX;
    }

    float SpotLight::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
    {
        sample = Position;
        dir = -Direction;
        return (point - sample).norm();
    }

    float EnvironmentLight::SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
    {
        // random reject sampling
        while(true)
        {
            float x = rnd(generator) * 2 - 1;
            float y = rnd(generator) * 2 - 1;
            float z = rnd(generator) * 2 - 1;
            Vector3f candid = Vector3f(x, y, z);
            if(candid.norm() <= 1 && normal.dot(candid) > 0)
            {
                sample = candid.normalized();
                dir = sample;
                return FLT_MAX;
            }
        }
    }

    Vector3f PointLight::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        float r = (Position - point).norm();
        return Intensity / (r * r);
    }

    Vector3f AreaLight::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        float r = (lsample - point).norm();
        Vector3f l = (point - lsample).normalized();
        float teta = Normal.dot(l);
        if(teta < 0) teta *= -1;
        return (Radiance * teta * Size * Size) / (r * r);
    }

    Vector3f DirectionalLight::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        return Radiance;
    }

    Vector3f SpotLight::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        auto dir = (point - lsample).normalized();
        float r = (point - lsample).norm();
        auto alpha = CoverageAngle;
        auto beta = FalloffAngle;
        auto theta = std::fabs(std::acos(dir.dot(Direction)) * 180 / M_PI);
        if(theta > (alpha/2))
        {
            return Vector3f::Zero();
        }
        else if(theta > (beta/2))
        {
            float f = (std::cos(theta) - std::cos(alpha/2)) / (std::cos(beta/2) - std::cos(alpha/2));
            f = std::pow(f, 4);
            return f * Intensity / (r*r);
        }
        return Intensity / (r*r);
    }

    Vector3f EnvironmentLight::GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
    {
        return GetColor(lsample) * 2 * M_PI;
        //float x = std::fabs(normal.x());
        //float y = std::fabs(normal.y());
        //float z = std::fabs(normal.z());
        //Vector3f np;
        //if(x <= y && x <= z) np = Vector3f(1, normal.y(), normal.z());
        //else if(y <= x && y <= z) np = Vector3f(normal.x(), 1, normal.z());
        //else np = Vector3f(normal.x(), normal.y(), 1);
        //np.normalize();
        //auto u = np.cross(normal).normalized();
        //auto w = normal.cross(u).normalized();
        //float theta = std::acos(lsample.dot(normal));
        //float sigma = std::atan(lsample.dot(w) / lsample.dot(u));
        //float uvu = (-sigma + M_PI) / (2 * M_PI);
        //float uvv = theta / M_PI;
        //int px = uvu * (_hdr->Width - 1);
        //int py = uvv * (_hdr->Height - 1);
        //auto l = _hdr->Fetch(px, py);
        //return l * 2 * M_PI;
    }

    Vector3f EnvironmentLight::GetColor(Vector3f direction)
    {
        float phi_g   = std::atan2(direction.z(), direction.x());
    	float theta_g = std::acos(direction.y());
	    float u = (-phi_g + M_PI) / (2 * M_PI);
	    float v = (theta_g) / M_PI;
        int px = u * (_hdr->Width - 1);
        int py = v * (_hdr->Height - 1);
        auto l = _hdr->Fetch(px, py);
        return l;
    }

}