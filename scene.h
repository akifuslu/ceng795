#pragma once
#include "vecfrom.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "texture.h"
#include "pugixml.hpp"
#include "Eigen/Dense"
#include <vector>
#include <unordered_map>
#include "resourcelocator.h"
#include "objectlight.h"

using namespace Eigen;

namespace raytracer
{
    class Scene
    {
        public:
            Scene(pugi::xml_node node);
            void Load();
            void Render(int numThreads);
            bool RayCast(Ray& ray, RayHit& hit, float maxDist, bool closest);
            Vector3f BackgroundColor;
            float ShadowRayEpsilon;
            float IntersectionTestEpsilon;
            int MaxRecursionDepth;
            std::vector<Camera> Cameras;
            AmbientLight ambientLight;
            EnvironmentLight* environmentLight;
            std::vector<Light*> Lights;
            std::vector<Material> Materials;
            std::vector<Vector3f> VertexData;
            std::vector<Vector2f> UVData;
            std::vector<Object*> Objects;
            IHittable* Root;
            std::vector<Translation3f> Translations;
            std::vector<AngleAxisf> Rotations;
            std::vector<AlignedScaling3f> Scalings;
            std::vector<Transform<float, 3, Affine>> Composite;
            BackgroundTexture* BackTexture = nullptr;
            std::unordered_map<int, Texture*> Textures;

            friend std::ostream& operator<<(std::ostream& os, const Scene& scene);
        private:
            Vector3f Trace(Ray& ray, Camera& cam, int depth, Vector2i& xy);
    };
}