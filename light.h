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

    class Light
    {
        public:
            Light(pugi::xml_node node);
            Vector3f Position;            
            virtual Vector3f SamplePoint()
            {
                return Vector3f::Zero();
            }
            virtual Vector3f GetLuminance(Vector3f point, Vector3f lsample)
            {
                return Vector3f::Zero();
            }
    };

    class PointLight : public Light
    {
        public:
            PointLight(pugi::xml_node node);
            Vector3f Intensity;
            virtual Vector3f SamplePoint() override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f lsample) override;
    };

    class AreaLight : public Light
    {
        public:
            AreaLight(pugi::xml_node node);
            Vector3f Normal;
            Vector3f Radiance;
            float Size;
            virtual Vector3f SamplePoint() override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f lsample) override;
        private:
            Vector3f u, v;
            std::default_random_engine generator;
            std::uniform_real_distribution<float> rnd;
    };
}