#pragma once
#include "vector.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "pugixml.hpp"
#include <vector>

namespace raytracer
{
    class Scene
    {
        public:
            Scene(pugi::xml_node node);
            void Load();
            void Render(int numThreads);
            Vector3f BackgroundColor;
            float ShadowRayEpsilon;
            float IntersectionTestEpsilon;
            std::vector<Camera> Cameras;
            AmbientLight ambientLight;
            std::vector<PointLight> PointLights;
            std::vector<Material> Materials;
            std::vector<Vector3f> VertexData;
            std::vector<Object*> Objects;
            std::vector<IHittable*> Hittables;
            IHittable* Root;
            friend std::ostream& operator<<(std::ostream& os, const Scene& scene);
        private:
            bool RayCast(const Ray& ray, RayHit& hit, float maxDist, bool closest);
            void Trace(int yStart, int yEnd, std::vector<unsigned char>& block, Camera& cam);
    };
}