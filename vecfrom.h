#pragma once
#include "Eigen/Dense"
#include "pugixml.hpp"
#include <sstream>

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


}