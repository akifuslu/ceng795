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


    void ComputeBounds(std::vector<IHittable*> hittables, Vector3f* min, Vector3f* max)
    {        
        for(int i = 0; i < hittables.size(); i++)
        {
            Vector3f mn, mx;
            hittables[i]->Bounds(mn, mx);
            if(mn.X < min->X)
                min->X = mn.X;
            if(mn.Y < min->Y)
                min->Y = mn.Y;
            if(mn.Z < min->Z)
                min->Z = mn.Z;
            if(mx.X > max->X)
                max->X = mx.X;
            if(mx.Y > max->Y)
                max->Y = mx.Y;
            if(mx.Z > max->Z)
                max->Z = mx.Z;
        }
    }

    IHittable* BuildBVH(std::vector<IHittable*> hittables, int axis) // 0-x, 1-y, 2-z
    {
        if(hittables.size() == 0)
        {
            return NULL;
        }
        if(hittables.size() == 1)
        {
            return hittables[0];
        }
        AABB* aabb = new AABB();
        aabb->Min = new Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
        aabb->Max = new Vector3f(FLT_MIN, FLT_MIN, FLT_MIN);
        ComputeBounds(hittables, aabb->Min, aabb->Max);
        auto t = (*aabb->Min + *aabb->Max) / 2;
        aabb->Center = new Vector3f(t.X, t.Y, t.Z);
        if(hittables.size() == 2)
        {
            aabb->Left = hittables[0];
            aabb->Right = hittables[1];
            return aabb;
        }
        //float xlen = aabb->Max->X - aabb->Min->X; 
        //float ylen = aabb->Max->Y - aabb->Min->Y;        
        //float zlen = aabb->Max->Z - aabb->Min->Z;
        //if(xlen > ylen && xlen > zlen)
        //    axis = 0;
        //if(ylen > xlen && ylen > zlen)
        //    axis = 1;
        //if(zlen > ylen && zlen > xlen)
        //    axis = 2;

        std::vector<IHittable*> lhs;
        std::vector<IHittable*> rhs;
        float c = 0;        
        switch(axis)
        {
            case 0:
                c = aabb->Center->X;
                //std::sort(hittables.begin(), hittables.end(), 
                //        [](const IHittable* & a, const IHittable* & b) -> bool
                //        { 
                //            return a->Center->X < b->Center->X; 
                //        });
                for(int i = 0; i < hittables.size(); i++)
                {
                    if(hittables[i]->Center->X < c)
                        lhs.push_back(hittables[i]);
                    else
                        rhs.push_back(hittables[i]);
                }
                axis = 1;
                break;
            case 1:
                c = aabb->Center->Y;
                for(int i = 0; i < hittables.size(); i++)
                {
                    if(hittables[i]->Center->Y < c)
                        lhs.push_back(hittables[i]);
                    else
                        rhs.push_back(hittables[i]);
                }
                axis = 2;
                break;
            case 2:
                c = aabb->Center->Z;
                for(int i = 0; i < hittables.size(); i++)
                {
                    if(hittables[i]->Center->Z < c)
                        lhs.push_back(hittables[i]);
                    else
                        rhs.push_back(hittables[i]);
                }
                axis = 0;
                break;
        }
        if(lhs.size() == 0)
        {            
            lhs.push_back(rhs[rhs.size() - 1]);
            rhs.erase(rhs.begin() + rhs.size() - 1);
        }
        if(rhs.size() == 0)
        {            
            rhs.push_back(lhs[lhs.size() - 1]);
            lhs.erase(lhs.begin() + lhs.size() - 1);
        }

        aabb->Left = BuildBVH(lhs, axis);
        aabb->Right = BuildBVH(rhs, axis);    
    }

    void Scene::Load()
    {
        for (size_t i = 0; i < Objects.size(); i++)
        {
            Objects[i]->Load(VertexData, Materials);
            auto hs = Objects[i]->GetHittables();
            Hittables.insert(Hittables.end(), hs.begin(), hs.end());
        }
        Root = BuildBVH(Hittables, 0);
    }

    bool Scene::RayCast(const Ray& ray, RayHit& hit, float maxDist, bool closest)
    {
        hit.T = maxDist;
        return Root->Hit(ray, hit);
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