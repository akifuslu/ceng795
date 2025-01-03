#pragma once
#include "Eigen/Dense"
#include "ray.h"
#include "pugixml.hpp"
#include <string>
#include "vecfrom.h"
#include <vector>
#include <random>
#include "tonemapper.h"

using namespace Eigen;

namespace raytracer
{
    class Camera
    {
        public:
            Camera(pugi::xml_node node);
            std::vector<Ray> GetRay(int x, int y);
            Vector3f Position;
            Vector3f Gaze;
            Vector3f GazePoint;
            float FovY;
            Vector3f Up;
            Vector4f NearPlane;
            float NearDistance;
            Vector2i ImageResolution;
            std::string ImageName;
            int NumSamples;
            float FocusDistance;
            float ApertureSize;
            bool FocusEnabled = false;
            friend std::ostream& operator<<(std::ostream& os, const Camera& cam);
            bool Tonemap = false;
            float Gamma;
            ToneMapper* toneMapper;
        private:
            std::default_random_engine generator;
            std::uniform_real_distribution<float> rnd;
            Vector3f img_center;
            Vector3f u, v, w;
            float suv, svv;
            Vector3f lu;
            int row;
            int col;
    };
}