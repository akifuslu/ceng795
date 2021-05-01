#include "ray.h"

namespace raytracer
{
    Ray::Ray(Vector3f origin, Vector3f direction)
    {
        Origin = origin;
        Direction = direction;
        InvDir = Vector3f(1/Direction.X, 1/Direction.Y, 1/Direction.Z);
        Sign[0] = (Direction.X > 0 ? 0 : 1);
        Sign[1] = (Direction.Y > 0 ? 0 : 1);
        Sign[2] = (Direction.Z > 0 ? 0 : 1);
        N = 1;
    }
}