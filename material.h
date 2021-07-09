#pragma once
#include "Eigen/Dense"
#include "pugixml.hpp"
#include "vecfrom.h"
#include "resourcelocator.h"

using namespace Eigen;

namespace raytracer
{
    class Material;
    class Ray;
    class RayHit;
    class Scene;

    class BRDF
    {
        public:
            BRDF();
            BRDF(pugi::xml_node node);
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) = 0;
            float Exponent;
            float n, k;
    };

    class Material
    {
        public:
            Material();
            Material(pugi::xml_node node);
            Vector3f Shade(Scene& scene, Ray& ray, RayHit& hit, float gamma);
            Vector3f AmbientReflectance;
            Vector3f DiffuseReflectance;
            Vector3f SpecularReflectance;
            float PhongExponent;
            Vector3f MirrorReflectance;
            float RefractionIndex;
            float AbsorptionIndex;
            Vector3f AbsorptionCoefficient;
            float Roughness;
            bool Degamma;
            int Type;
            BRDF* Brdf;
            friend std::ostream& operator<<(std::ostream& os, const Material& mat);            
    };

    class OriginalPhong : public BRDF
    {
        public:
            OriginalPhong(pugi::xml_node node);
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) override;
    };

    class ModifiedPhong : public BRDF
    {
        public:
            ModifiedPhong(pugi::xml_node node);
            bool Normalized;
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) override;
    };

    class OriginalBlinnPhong : public BRDF
    {
        public:
            OriginalBlinnPhong() {};
            OriginalBlinnPhong(pugi::xml_node node);
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) override;
    };

    class ModifiedBlinnPhong : public BRDF
    {
        public:
            ModifiedBlinnPhong(pugi::xml_node node);
            bool Normalized;
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) override;
    };

    class TorranceSparrow : public BRDF
    {
        public:
            TorranceSparrow(pugi::xml_node node);
            bool Kdfresnel;
            virtual Vector3f Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance) override;
    };


}