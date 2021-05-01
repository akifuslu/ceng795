#pragma once
#include "vector.h"
#include "material.h"

namespace raytracer
{
    class Ray
    {
        public:
            Ray(Vector3f origin, Vector3f direction);
            Vector3f Origin;
            Vector3f Direction;
            Vector3f InvDir;
            int Sign[3];
            float N = 1;
            float Dist;
    };
}