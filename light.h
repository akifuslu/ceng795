#pragma once
#include "vector.h"
#include "pugixml.hpp"

namespace raytracer
{
    class AmbientLight
    {
        public:
            AmbientLight() {};
            AmbientLight(pugi::xml_node node);
            Vector3f Intensity;
    };

    class PointLight
    {
        public:
            PointLight(pugi::xml_node node);
            Vector3f Position;
            Vector3f Intensity;
    };
}