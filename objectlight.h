#pragma once
#include "object.h"
#include "light.h"

namespace raytracer
{
    class LightSphere : public Sphere, public Light
    {
        public:
            LightSphere(pugi::xml_node node);
            virtual float SamplePoint(Vector3f point, Vector3f normal, Vector3f& sample, Vector3f& dir) override;
            virtual Vector3f GetLuminance(Vector3f point, Vector3f normal, Vector3f lsample) override;
            virtual bool Hit(const Ray& ray, RayHit& hit) override;
            Vector3f Radiance;
        private:
            std::default_random_engine generator;
            std::uniform_real_distribution<float> rnd;

    };
}

// calculate in local space
// convert hit point to local space
// sintetamax = r / d
// tetamax = arcsin(r/d)
// costetamax = sqrt(1 - sin2(tetamax))
// p(phi) = 1/2pi
// p(theta) = sinteta / (1 - costetamax)
// tetai = p-1(x) = arccos(1-x+xcostetamax)
// phii = 2pi rnd
// llocal = wcosteta + vsintetacosphi + usintetasinphi
// convert llocal to lworld
// 

// Lum = R / (2pi(1-costetamax))