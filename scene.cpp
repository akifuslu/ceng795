#include "scene.h"
#include <stdlib.h>
#include <string>
#include <sstream>
#include <cfloat>
#include "lodepng.h"
#include <cmath>
#include <thread>
#include <chrono>

namespace raytracer
{
    Scene::Scene(pugi::xml_node node)
    {
        BackgroundColor =  Vector3f(node.child("BackgroundColor"));
        ShadowRayEpsilon = node.child("ShadowRayEpsilon").text().as_float();
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
        }
        
    }

    bool Scene::RayCast(const Ray& ray, RayHit& hit, float maxDist, bool closest)
    {
        hit.T = maxDist;
        bool flag = false;
        for(int k = 0; k < Objects.size(); k++)
        {
            RayHit tHit;
            if(Objects[k]->Hit(ray, tHit))
            {
                flag = true;                            
                if(tHit.T < hit.T)
                {
                    hit = tHit;
                    if(!closest)
                    {
                        return true;
                    }
                }
            }
        }
        return flag;
    }

    void Scene::Trace(int yStart, int yEnd, std::vector<unsigned char>& pixels, Camera& cam)
    {
        int p = yStart * cam.ImageResolution.X * 4;
        for (int i = yStart; i < yEnd; i++)
        {
            for (int j = 0; j < cam.ImageResolution.X; j++)
            {
                Ray ray = cam.GetRay(j, i);
                RayHit hit;                    
                if(RayCast(ray, hit, FLT_MAX, true))
                {
                    Vector3f color = hit.Material.AmbientReflectance * ambientLight.Intensity;
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
                    pixels[p++] = color.X > 255 ? 255 : color.X;
                    pixels[p++] = color.Y > 255 ? 255 : color.Y;
                    pixels[p++] = color.Z > 255 ? 255 : color.Z;
                    pixels[p++] = 255;
                }
                else
                {
                    pixels[p++] = BackgroundColor.X;
                    pixels[p++] = BackgroundColor.Y;
                    pixels[p++] = BackgroundColor.Z;
                    pixels[p++] = 255;
                }  
            }
        }
    }

    void Scene::Render(int numThreads)
    {
        for(auto& cam: Cameras)
        {
            std::vector<unsigned char> pixels;
            pixels.resize(cam.ImageResolution.X * cam.ImageResolution.Y * 4);
            int blockSize = cam.ImageResolution.Y / numThreads;
            std::thread threads[numThreads];
            for(int i = 0; i < numThreads; i++)
            {
                threads[i] = std::thread(&Scene::Trace, this, i * blockSize, (i + 1) * blockSize, std::ref(pixels), std::ref(cam));
                //Trace(i * blockSize, (i + 1) * blockSize, pixels, cam);
            }
            for (int i = 0; i < numThreads; i++)
            {
                threads[i].join();
            }
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