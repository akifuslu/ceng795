#pragma once

#include "Eigen/Dense"
#include "pugixml.hpp"
#include "vecfrom.h"
#include "texture.h"
#include "resourcelocator.h"

using namespace Eigen;

namespace raytracer
{
    class AmbientLight
    {
        public:
            AmbientLight() {Intensity = Vector3f::Zero();};
            AmbientLight(pugi::xml_node node);
            Vector3f Intensity;
    };

    class Light
    {
        public:
            Light(pugi::xml_node node);
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir)
            {
                return 0;
            }
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample)
            {
                return Vector3f::Zero();
            }
    };

    class PointLight : public Light
    {
        public:
            PointLight(pugi::xml_node node);
            Vector3f Position;          
            Vector3f Intensity;
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
    };

    class AreaLight : public Light
    {
        public:
            AreaLight(pugi::xml_node node);
            Vector3f Position;          
            Vector3f Normal;
            Vector3f Radiance;
            float Size;
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
        private:
            Vector3f u, v;
            std::default_random_engine generator;
            std::uniform_real_distribution<float> rnd;
            std::vector<float> rndX;
            std::vector<float> rndY;
            int c;
    };

    class DirectionalLight : public Light
    {
        public:
            DirectionalLight(pugi::xml_node node);
            Vector3f Direction;
            Vector3f Radiance;
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
    };

    class SpotLight : public Light
    {
        public:
            SpotLight(pugi::xml_node node);
            Vector3f Position;
            Vector3f Direction;
            Vector3f Intensity;
            float CoverageAngle;
            float FalloffAngle;
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
    };

    class EnvironmentLight : public Light
    {
        public:
            EnvironmentLight(pugi::xml_node node);
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
            Vector3f GetColor(Vector3f direction);
        private:
            Image* _hdr;
            std::default_random_engine generator;
            std::uniform_real_distribution<float> rnd;
    };
}