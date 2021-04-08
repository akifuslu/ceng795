#pragma once
#include "vector.h"
#include "material.h"

namespace raytracer
{
    class Object;
    class Ray
    {
        public:
            Ray(Vector3f origin, Vector3f direction);
            Vector3f Origin;
            Vector3f Direction;
    };

    class RayHit
    {
        public:
            Object* Object;
            Material Material;
            float T;
            Vector3f Point;
            Vector3f Normal;
    };
}