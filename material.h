#pragma once
#include "Eigen/Dense"
#include "pugixml.hpp"
#include "vecfrom.h"

using namespace Eigen;

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
            Vector3f MirrorReflectance;
            float RefractionIndex;
            float AbsorptionIndex;
            Vector3f AbsorptionCoefficient;
            float Roughness;
            int Type;
            friend std::ostream& operator<<(std::ostream& os, const Material& mat);
    };
}