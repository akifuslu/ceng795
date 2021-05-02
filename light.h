#pragma once

#include "Eigen/Dense"
#include "pugixml.hpp"
#include "vecfrom.h"

using namespace Eigen;

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