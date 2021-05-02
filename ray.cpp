#include "ray.h"

namespace raytracer
{
    Ray::Ray(Vector3f origin, Vector3f direction)
    {
        Origin = origin;
        Direction = direction;
        InvDir = Vector3f(1/Direction.x(), 1/Direction.y(), 1/Direction.z());
        Sign[0] = (Direction.x() > 0 ? 0 : 1);
        Sign[1] = (Direction.y() > 0 ? 0 : 1);
        Sign[2] = (Direction.z() > 0 ? 0 : 1);
        N = 1;
    }
}