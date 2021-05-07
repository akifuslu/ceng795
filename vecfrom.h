#pragma once
#include "Eigen/Dense"
#include "Eigen/Geometry"
#include "pugixml.hpp"
#include <sstream>
#include <random>
#include <iostream>

using namespace Eigen;

namespace raytracer
{
    static Vector3f Vec3fFrom(pugi::xml_node node)
    {
        float x, y, z;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y >> z;
        return Vector3f(x, y, z);
    }

    static Vector4f Vec4fFrom(pugi::xml_node node)
    {
        float x, y, z, w;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y >> z >> w;
        return Vector4f(x, y, z, w);
    }

    static Vector2i Vec2iFrom(pugi::xml_node node)
    {
        int x, y;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y;
        return Vector2i(x, y);
    }

    static Vector3i Vec3iFrom(pugi::xml_node node)
    {
        int x, y, z;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y >> z;
        return Vector3i(x, y, z);
    }

    static Translation3f TranslationFrom(pugi::xml_node node)
    {
        float x, y, z;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y >> z;
        return Translation3f(x, y, z);
    }

    static AngleAxisf RotationFrom(pugi::xml_node node)
    {
        float angle, x, y, z;
        std::stringstream stream(node.first_child().value());
        stream >> angle >> x >> y >> z;
        double pi = 3.14159265359;
        float rad = ((angle * pi) / (180.0f));            
        return AngleAxisf(rad, Vector3f(x, y, z));
    }

    static AlignedScaling3f ScalingFrom(pugi::xml_node node)
    {
        float x, y, z;
        std::stringstream stream(node.first_child().value());
        stream >> x >> y >> z;
        return AlignedScaling3f(x, y, z);
    }

    static std::default_random_engine gen;
    static std::uniform_real_distribution<float> rand(-.5f,0.5f);

    static Vector3f Reflect(Vector3f in, Vector3f norm, float roughness)
    {
        in.normalize();
        norm.normalize();
        Vector3f reflect = in - norm * 2 * norm.dot(in);
        reflect.normalize();
        if(roughness != 0)
        {
            Vector3f rp;
            float x = std::fabs(reflect.x());
            float y = std::fabs(reflect.y()); 
            float z = std::fabs(reflect.z()); 
            if(x <= y && x <= z)
                rp = Vector3f(1, reflect.y(), reflect.z());
            else if(y <= x && y <= z)
                rp = Vector3f(reflect.x(), 1, reflect.z());
            else
                rp = Vector3f(reflect.x(), reflect.y(), 1);

            Vector3f u = reflect.cross(rp).normalized();
            Vector3f v = reflect.cross(u).normalized();                                  
            float e1 = rand(gen);
            float e2 = rand(gen);
            reflect = (reflect + roughness * (e1 * u + e2 * v)).normalized();
        }
        return reflect;
    }

}