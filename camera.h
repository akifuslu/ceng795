#pragma once
#include "vector.h"
#include "ray.h"
#include "pugixml.hpp"
#include <string>

namespace raytracer
{
    class Camera
    {
        public:
            Camera(pugi::xml_node node);
            Ray GetRay(int x, int y);
            Vector3f Position;
            Vector3f Gaze;
            Vector3f Up;
            Vector4f NearPlane;
            float NearDistance;
            Vector2i ImageResolution;
            std::string ImageName;
            friend std::ostream& operator<<(std::ostream& os, const Camera& cam);
        private:
            Vector3f img_center;
            Vector3f v;
            Vector3f q;
    };
}