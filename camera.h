#pragma once
#include "Eigen/Dense"
#include "ray.h"
#include "pugixml.hpp"
#include <string>
#include "vecfrom.h"

using namespace Eigen;

namespace raytracer
{
    class Camera
    {
        public:
            Camera(pugi::xml_node node);
            Ray GetRay(int x, int y);
            Vector3f Position;
            Vector3f Gaze;
            Vector3f GazePoint;
            float FovY;
            Vector3f Up;
            Vector4f NearPlane;
            float NearDistance;
            Vector2i ImageResolution;
            std::string ImageName;
            friend std::ostream& operator<<(std::ostream& os, const Camera& cam);
        private:
            Vector3f img_center;
            Vector3f u, v, w;
            Vector3f q;
    };
}