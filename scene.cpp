#include "scene.h"
#include <stdlib.h>
#include <string>
#include <sstream>
#include <cfloat>
#include "lodepng.h"
#include <cmath>
#include <thread>
#include <chrono>
#include <future>

namespace raytracer
{
    Scene::Scene(pugi::xml_node node)
    {
        BackgroundColor =  Vector3f(node.child("BackgroundColor"));
        ShadowRayEpsilon = node.child("ShadowRayEpsilon").text().as_float();
        if(ShadowRayEpsilon == 0)
            ShadowRayEpsilon = .000001f;
        IntersectionTestEpsilon = node.child("IntersectionTestEpsilon").text().as_float();
        auto cameras = node.child("Cameras");
        for(auto& camera: cameras.children())
        {
            Cameras.push_back(Camera(camera));
        }
        auto lights = node.child("Lights");
        ambientLight = AmbientLight(lights.child("AmbientLight"));
        for(auto& light: lights.children("PointLight"))
        {
            PointLights.push_back(PointLight(light));
        }
        auto materials = node.child("Materials");
        for(auto& material: materials.children())
        {
            Materials.push_back(Material(material));
        }
        auto vertices = node.child("VertexData");
        std::stringstream vs;
        vs << vertices.first_child().text().as_string();
        float x, y, z;
        while(vs >> x >> y >> z)
        {            
            VertexData.push_back(Vector3f(x, y, z));
        }
        auto objects = node.child("Objects");
        for(auto& object: objects.children())
        {
            if(std::strcmp("Mesh", object.name()) == 0)
            {
                Objects.push_back(new Mesh(object));
            }
            else if(std::strcmp("Triangle", object.name()) == 0)
            {
                Objects.push_back(new Triangle(object));
            }
            else if(std::strcmp("Sphere", object.name()) == 0)
            {
                Objects.push_back(new Sphere(object));
            }
        }
    }

    void Scene::Load()
    {
        for (size_t i = 0; i < Objects.size(); i++)
        {
            Objects[i]->Load(VertexData, Materials);
            auto hs = Objects[i]->GetHittables();
            Hittables.insert(Hittables.end(), hs.begin(), hs.end());
        }
        IHittable** hs = new IHittable*[Hittables.size()];
        for(int i = 0; i < Hittables.size(); i++)
            hs[i] = Hittables[i];
        Root = new AABB(hs, Hittables.size());
    }

    bool Scene::RayCast(const Ray& ray, RayHit& hit, float maxDist, bool closest)
    {
        return Root->Hit(ray, hit);
    }

    Vector3f Scene::Trace(Ray ray, Camera& cam, int depth)
    {
        Vector3f color(0, 0, 0);
        RayHit hit;              
        if(depth == 0)
            return color;      
        if(RayCast(ray, hit, FLT_MAX, true))
        {
            if(hit.Material.Type == 3)
            {
                auto vo = ray.Direction;
                vo.Normalize();
                auto reflect = vo - hit.Normal * 2 * Vector3f::Dot(hit.Normal, vo);
                reflect.Normalize();
                ray = Ray(hit.Point, reflect);
                auto cl = Trace(ray, cam, depth - 1);
                color = color + hit.Material.MirrorReflectance * cl;
            }
            else if(hit.Material.Type == 2)
            {
                auto vo = ray.Direction;
                vo.Normalize();
                auto reflect = vo - hit.Normal * 2 * Vector3f::Dot(hit.Normal, vo);
                reflect.Normalize();

                float ndi = Vector3f::Dot(hit.Normal, ray.Direction);
                float ri = hit.Material.RefractionIndex;   
                Vector3f normal = hit.Normal;
                if(ndi > 0)
                {
                    normal = normal * -1;
                    ri = 1;
                }
                else
                {
                    ndi *= -1; 
                }                

                float k = 1 - (ray.N/ri) * (ray.N/ri) * (1 - ndi * ndi);
                if(k < 0)
                {
                    auto rray = Ray(hit.Point + normal * ShadowRayEpsilon, reflect);
                    auto cl = Trace(rray, cam, depth - 1);
                    color = color + cl * (Vector3f(1,1,1)-hit.Material.AbsorptionCoefficient);
                }
                else
                {
                    auto reftrac = (ray.Direction + normal * k) * (ray.N/ri) - normal * k;
                    reftrac.Normalize();
                    float rdi = Vector3f::Dot(reftrac, normal * -1);
                    float r1 = (ri * ndi - rdi) / (ri * ndi + rdi);
                    float r2 = (ndi - ri * rdi) / (ndi + ri * rdi);
                    float fr = (r1 * r1 + r2 * r2) / 2;
                    float ft = 1 - fr;
                    auto rray = Ray(hit.Point + normal * ShadowRayEpsilon, reflect);
                    auto cl = Trace(rray, cam, depth - 1);
                    color = color + cl * fr;
                    auto tray = Ray(hit.Point - normal * ShadowRayEpsilon, reftrac);
                    tray.N = ri;
                    cl = Trace(tray, cam, depth - 1);
                    color = color + cl * (1 - fr);
                }                
            }
            else if(hit.Material.Type == 1)
            {
                auto vo = ray.Direction;
                vo.Normalize();
                auto reflect = vo - hit.Normal * 2 * Vector3f::Dot(hit.Normal, vo);
                reflect.Normalize();
                float ndi = Vector3f::Dot(hit.Normal, ray.Direction * -1);
                float n = hit.Material.RefractionIndex;
                float k = hit.Material.AbsorptionIndex;
                float rs = ((n * n + k * k) - 2 * n * ndi + ndi * ndi) / ((n * n + k * k) + 2 * n * ndi + ndi * ndi);
                float rp = ((n * n + k * k) * ndi * ndi - 2 * n * ndi + 1) / ((n * n + k * k) * ndi * ndi + 2 * n * ndi + 1);
                float fr = (rs + rp) / 2;
                ray = Ray(hit.Point, reflect);
                auto cl = Trace(ray, cam, depth - 1);
                color = color + hit.Material.MirrorReflectance * cl * fr;
            }
            color = color + hit.Material.AmbientReflectance * ambientLight.Intensity;

            for(int l = 0; l < PointLights.size(); l++)
            {
                auto light = PointLights[l];                            
                // SHADOW CHECK
                auto wi = (light.Position - hit.Point).Normalized();
                auto sp = hit.Point + hit.Normal * ShadowRayEpsilon;
                float r = (light.Position - hit.Point).Magnitude();
                Ray sRay = Ray(sp, wi);
                RayHit sHit;
                bool sf = RayCast(sRay, sHit, r, false);
                if(sf && sHit.T < r)
                    continue;
                // DIFFUSE
                float teta = Vector3f::Dot(wi, hit.Normal);
                teta = teta < 0 ? 0 : teta;
                color = color + hit.Material.DiffuseReflectance * teta * light.Intensity / (r * r);                            
                // SPECULAR
                auto wo = (cam.Position - hit.Point).Normalized();
                auto h = (wi + wo).Normalized();
                teta = Vector3f::Dot(h, hit.Normal);
                teta = teta < 0 ? 0 : teta;
                teta = std::pow(teta, hit.Material.PhongExponent);
                color = color + hit.Material.SpecularReflectance * teta * light.Intensity / (r * r);
            }
        }
        else
        {
            color = BackgroundColor;
        }  
        return color;
    }

    void Scene::Render(int numThreads)
    {
        for(auto& cam: Cameras)
        {
            std::vector<unsigned char> pixels;
            pixels.resize(cam.ImageResolution.X * cam.ImageResolution.Y * 4);
            int size = cam.ImageResolution.X * cam.ImageResolution.Y;
            int cores = std::thread::hardware_concurrency();
            volatile std::atomic<int> count(0);
            std::vector<std::future<void>> futures;
            while(cores--)
            {
                futures.emplace_back(
                    std::async([=, &count, &cam, &pixels]()
                    {
                        while(true)
                        {
                            int index = count++;
                            if(index >= size)
                                break;
                            int x = index % cam.ImageResolution.X;
                            int y = index / cam.ImageResolution.Y;
                            auto ray = cam.GetRay(x, y);
                            auto cl = Trace(ray, cam, 6);
                            pixels[4 * index] = cl.X > 255 ? 255 : cl.X;
                            pixels[4 * index + 1] = cl.Y > 255 ? 255 : cl.Y;
                            pixels[4 * index + 2] = cl.Z > 255 ? 255 : cl.Z;
                            pixels[4 * index + 3] = 255;
                        }
                    }
                    )
                );
            }
            futures.clear();
            std::vector<unsigned char> png;
            lodepng::encode(png, pixels, cam.ImageResolution.X, cam.ImageResolution.Y);
            lodepng::save_file(png, cam.ImageName);
        }            
    }

    std::ostream& operator<<(std::ostream& os, const Scene& scene)
    {
        os << "BackgroundColor " << scene.BackgroundColor << std::endl;
        os << "ShadowRayEpsilon " << scene.ShadowRayEpsilon << std::endl;
        os << "IntersectionTestEpsilon " << scene.IntersectionTestEpsilon << std::endl;
        os << "Cameras" << std::endl;
        for(auto& cam: scene.Cameras)
        {
            os << cam << std::endl;
        }
        os << "Materials" << std::endl;
        for(auto& mat: scene.Materials)
        {
            os << mat << std::endl;
        }
        os << "VertexData" << std::endl;
        for(auto& vert: scene.VertexData)
        {
            os << vert << std::endl;
        }
        os << "Objects" << std::endl;
        for(auto& obj: scene.Objects)
        {
            os << (*obj) << std::endl;
        }
        return os;
    }   
}