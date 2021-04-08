#include "ray.h"

namespace raytracer
{
    Ray::Ray(Vector3f origin, Vector3f direction)
    {
        Origin = origin;
        Direction = direction;
    }
}