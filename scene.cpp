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
        BackgroundColor =  Vec3fFrom(node.child("BackgroundColor"));
        ShadowRayEpsilon = node.child("ShadowRayEpsilon").text().as_float();
        if(ShadowRayEpsilon == 0)
            ShadowRayEpsilon = 0.001f;
        MaxRecursionDepth = node.child("MaxRecursionDepth").text().as_int();
        if(MaxRecursionDepth == 0)
            MaxRecursionDepth = 1;

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
            else if(std::strcmp("MeshInstance", object.name()) == 0)
            {
                Objects.push_back(new MeshInstance(object));
            }
        }
        auto transformations = node.child("Transformations");
        for(auto& transform: transformations.children())
        {
            if(std::strcmp("Translation", transform.name()) == 0)
            {
                Translations.push_back(TranslationFrom(transform));
            }
            else if(std::strcmp("Rotation", transform.name()) == 0)
            {
                Rotations.push_back(RotationFrom(transform));
            }
            else if(std::strcmp("Scaling", transform.name()) == 0)
            {
                Scalings.push_back(ScalingFrom(transform));
            }
        }
    }

    void Scene::Load()
    {
        IHittable** hs = new IHittable*[Objects.size()];
        for (size_t i = 0; i < Objects.size(); i++)
        {
            Objects[i]->Load(*this);
            hs[i] = Objects[i];
        }
        Root = new BVH(hs, Objects.size());
    }

    bool Scene::RayCast(Ray& ray, RayHit& hit, float maxDist, bool closest)
    {
        auto ret = Root->Hit(ray, hit);
        ray.Dist = (hit.Point - ray.Origin).norm();
        //bool ret = false;
        //hit.T = maxDist;
        //for(int i = 0; i < Objects.size(); i++)
        //{
        //    RayHit thit;            
        //    thit.T = FLT_MAX;
        //    if(Objects[i]->aabb->Hit(ray, thit) && thit.T < hit.T)
        //    {
        //        ret = true;
        //        hit = thit;
        //    }
        //}
        return ret;
    }

    Vector3f Scene::Trace(Ray& ray, Camera& cam, int depth)
    {
        Vector3f color(0, 0, 0);
        RayHit hit;              
        if(depth == 0)
            return color;      
        if(RayCast(ray, hit, FLT_MAX, true))
        {
            if(hit.Material.Type == 3)
            {
                Vector3f reflect = Reflect(ray.Direction, hit.Normal, hit.Material.Roughness);
                ray = Ray(hit.Point + hit.Normal * ShadowRayEpsilon, reflect, ray.Time);
                auto cl = Trace(ray, cam, depth - 1);
                color = color + hit.Material.MirrorReflectance.cwiseProduct(cl);
            }
            else if(hit.Material.Type == 2)
            {
                float n1 = ray.N; //from
                float n2 = ray.N == 1 ? hit.Material.RefractionIndex : 1;//to
                float ctheta = -ray.Direction.dot(hit.Normal);
                Vector3f normal = hit.Normal;
                if(ctheta < 0) // flip normal
                {
                    ctheta *= -1;
                    normal = normal * -1;
                }
                Vector3f reflect = Reflect(ray.Direction, normal, hit.Material.Roughness);
                float cphi2 = 1 - (n1/n2)*(n1/n2)*(1 - ctheta*ctheta);
                if(cphi2 < 0) // no reftrac
                {
                    Ray rray = Ray(hit.Point + normal * ShadowRayEpsilon, reflect, ray.Time);
                    rray.N = ray.N;
                    color = color + Trace(rray, cam, depth - 1);
                }
                else
                {
                    float cphi = std::sqrt(cphi2);
                    Vector3f reftrac = (ray.Direction + normal * ctheta) * (n1/n2) - normal * cphi;
                    float r1 = (n2 * ctheta - n1 * cphi) / (n2 * ctheta + n1 * cphi);
                    float r2 = (n1 * ctheta - n2 * cphi) / (n1 * ctheta + n2 * cphi);   
                    float fr = (r1*r1 + r2*r2) / 2;
                    float ft = 1 - fr;
                    Ray rray = Ray(hit.Point + normal * ShadowRayEpsilon, reflect, ray.Time);
                    rray.N = ray.N;
                    color = color + Trace(rray, cam, depth - 1) * fr;
                    Ray tray = Ray(hit.Point - normal * ShadowRayEpsilon, reftrac, ray.Time);
                    tray.N = n2;
                    Vector3f l0 = Trace(tray, cam, depth - 1) * ft;
                    RayHit tmphit;
                    RayCast(tray, tmphit, FLT_MAX, false);
                    if(ray.N == 1)
                    {
                        l0.x() = l0.x() * std::exp(hit.Material.AbsorptionCoefficient.x() * tray.Dist * -1);
                        l0.y() = l0.y() * std::exp(hit.Material.AbsorptionCoefficient.y() * tray.Dist * -1);
                        l0.z() = l0.z() * std::exp(hit.Material.AbsorptionCoefficient.z() * tray.Dist * -1);
                    }
                    color = color + l0;
                }                
            }
            else if(hit.Material.Type == 1)
            {
                Vector3f reflect = Reflect(ray.Direction, hit.Normal, hit.Material.Roughness);
                float ndi = -hit.Normal.dot(ray.Direction);
                float n = hit.Material.RefractionIndex;
                float k = hit.Material.AbsorptionIndex;
                float rs = ((n*n + k*k) - 2 * n * ndi + ndi * ndi) / ((n*n + k*k) + 2 * n * ndi + ndi * ndi);
                float rp = ((n*n + k*k) * ndi*ndi - 2*n*ndi + 1) / ((n*n + k*k) * ndi*ndi + 2*n*ndi + 1);
                float fr = (rs + rp) / 2;
                Ray rray = Ray(hit.Point + hit.Normal * ShadowRayEpsilon, reflect, ray.Time);
                auto cl = Trace(rray, cam, depth - 1);
                color = color + hit.Material.MirrorReflectance.cwiseProduct(cl) * fr;
            }

            if(ray.N != 1)
                return color;

            color = color + hit.Material.AmbientReflectance.cwiseProduct(ambientLight.Intensity);

            for(int l = 0; l < PointLights.size(); l++)
            {
                auto light = PointLights[l];                            
                // SHADOW CHECK
                Vector3f wi = (light.Position - hit.Point).normalized();
                Vector3f sp = hit.Point + hit.Normal * ShadowRayEpsilon;
                float r = (light.Position - hit.Point).norm();
                Ray sRay = Ray(sp, wi, ray.Time);                
                RayHit sHit;
                bool sf = RayCast(sRay, sHit, r, false);
                if(sf && sHit.T < r)
                    continue;
                // DIFFUSE
                float teta = wi.dot(hit.Normal);
                teta = teta < 0 ? 0 : teta;
                color += hit.Material.DiffuseReflectance.cwiseProduct(light.Intensity) * teta / (r * r);                            
                // SPECULAR
                Vector3f wo = (cam.Position - hit.Point).normalized();
                Vector3f h = (wi + wo).normalized();
                teta = h.dot(hit.Normal);
                teta = teta < 0 ? 0 : teta;
                teta = std::pow(teta, hit.Material.PhongExponent);
                color += hit.Material.SpecularReflectance.cwiseProduct(light.Intensity) * teta / (r * r);
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
            pixels.resize(cam.ImageResolution.x() * cam.ImageResolution.y() * 4);
            int size = cam.ImageResolution.x() * cam.ImageResolution.y();
            int cores = std::thread::hardware_concurrency();
            if(numThreads == 1)
                cores = 1;
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
                            int x = index % cam.ImageResolution.x();
                            int y = index / cam.ImageResolution.x();                            
                            auto rays = cam.GetRay(x, y);
                            Vector3f cl = Vector3f::Zero();
                            for(int r = 0; r < rays.size(); r++)
                            {
                                cl += Trace(rays[r], cam, MaxRecursionDepth);
                            }
                            cl /= rays.size();
                            pixels[4 * index] = cl.x() > 255 ? 255 : cl.x();
                            pixels[4 * index + 1] = cl.y() > 255 ? 255 : cl.y();
                            pixels[4 * index + 2] = cl.z() > 255 ? 255 : cl.z();
                            pixels[4 * index + 3] = 255;
                        }
                    }
                    )
                );
            }
            futures.clear();
            std::vector<unsigned char> png;
            lodepng::encode(png, pixels, cam.ImageResolution.x(), cam.ImageResolution.y());
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