#pragma once
#include "Eigen/Dense"
#include "material.h"

using namespace Eigen;

namespace raytracer
{
    class Ray
    {
        public:
            Ray(){};
            Ray(Vector3f origin, Vector3f direction);
            Ray(Vector3f origin, Vector3f direction, float time);
            Vector3f Origin;
            Vector3f Direction;
            Vector3f InvDir;
            int Sign[3];
            float N = 1;
            float Dist;
            float Time;
    };
}