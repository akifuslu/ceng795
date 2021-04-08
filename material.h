#pragma once
#include "vector.h"
#include "pugixml.hpp"

namespace raytracer
{
    class Material
    {
        public:
            Material();
            Material(pugi::xml_node node);
            Vector3f AmbientReflectance;
            Vector3f DiffuseReflectance;
            Vector3f SpecularReflectance;
            float PhongExponent;
            friend std::ostream& operator<<(std::ostream& os, const Material& mat);
    };
}