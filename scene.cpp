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
#include "tonemapper.h"

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
        auto images = node.child("Textures").child("Images");
        for(auto& image: images.children())
        {
            ResourceLocator::GetInstance().AddImage(new Image(image));
        }
                auto brdfs = node.child("BRDFs");
        for(auto& brdf: brdfs.children())
        {
            if(std::strcmp("OriginalPhong", brdf.name()) == 0)
            {
                ResourceLocator::GetInstance().AddBRDF(new OriginalPhong(brdf));
            }
            else if(std::strcmp("ModifiedPhong", brdf.name()) == 0)
            {
                ResourceLocator::GetInstance().AddBRDF(new ModifiedPhong(brdf));
            }
            else if(std::strcmp("OriginalBlinnPhong", brdf.name()) == 0)
            {
                ResourceLocator::GetInstance().AddBRDF(new OriginalBlinnPhong(brdf));
            }
            else if(std::strcmp("ModifiedBlinnPhong", brdf.name()) == 0)
            {
                ResourceLocator::GetInstance().AddBRDF(new ModifiedBlinnPhong(brdf));
            }
            else if(std::strcmp("TorranceSparrow", brdf.name()) == 0)
            {
                ResourceLocator::GetInstance().AddBRDF(new TorranceSparrow(brdf));
            }
        }

        auto lights = node.child("Lights");
        if(lights.child("AmbientLight"))
        {
            ambientLight = AmbientLight(lights.child("AmbientLight"));
        }
        else
        {
            ambientLight = AmbientLight();
        }
        
        for(auto& light: lights.children("PointLight"))
        {
            Lights.push_back(new PointLight(light));
        }
        for(auto& light: lights.children("AreaLight"))
        {
            Lights.push_back(new AreaLight(light));
        }
        for(auto& light: lights.children("DirectionalLight"))
        {
            Lights.push_back(new DirectionalLight(light));
        }
        for(auto& light: lights.children("SpotLight"))
        {
            Lights.push_back(new SpotLight(light));
        }
        environmentLight = nullptr;
        for(auto& light: lights.children("SphericalDirectionalLight"))
        {
            environmentLight = new EnvironmentLight(light);
            Lights.push_back(environmentLight);
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
        auto uvs = node.child("TexCoordData");
        std::stringstream us;
        us << uvs.text().as_string();
        float u, v;
        while(us >> u >> v)
        {
            UVData.push_back(Vector2f(u, v));
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
            else if(std::strcmp("LightSphere", object.name()) == 0)
            {
                auto ls = new LightSphere(object);
                Objects.push_back(ls);
                Lights.push_back(ls);
            }
            else if(std::strcmp("LightMesh", object.name()) == 0)
            {
                auto ls = new LightMesh(object);
                Objects.push_back(ls);
                Lights.push_back(ls);
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
            else if(std::strcmp("Composite", transform.name()) == 0)
            {
                Composite.push_back(CompositeFrom(transform));
            }
        }
        auto textures = node.child("Textures");
        for(auto& texture: textures.children("TextureMap"))
        {
            auto type = std::string(texture.child("DecalMode").text().as_string());
            int id = texture.attribute("id").as_int();
            if(type.compare("replace_background") == 0)
            {
                BackTexture = new BackgroundTexture(texture);
            }
            else if(type.compare("replace_normal") == 0)
            {
                Textures[id] = new NormalTexture(texture);
            }
            else if(type.compare("bump_normal") == 0)
            {
                Textures[id] = new BumpTexture(texture);
            }
            else
            {
                Textures[id] = new DiffuseTexture(texture);
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
        ray.Dist = hit.T;
        return ret;
    }

    Vector3f Scene::Trace(Ray& ray, Camera& cam, int depth, Vector2i& xy)
    {
        Vector3f color(0, 0, 0);
        RayHit hit;              
        if(depth < 0)
            return color;      
        if(RayCast(ray, hit, FLT_MAX, true))
        {
            if(hit.Material.Type == 3)
            {
                Vector3f reflect = Reflect(ray.Direction, hit.Normal, hit.Material.Roughness);
                ray = Ray(hit.Point + hit.Normal * ShadowRayEpsilon, reflect, ray.Time);
                auto cl = Trace(ray, cam, depth - 1, xy);
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
                    Vector3f l1 = Trace(rray, cam, depth - 1, xy);
                    RayHit rmphit;
                    if(ray.N != 1)
                    {
                        RayCast(rray, rmphit, FLT_MAX, false);
                        l1.x() = l1.x() * std::exp(hit.Material.AbsorptionCoefficient.x() * rmphit.T * -1);
                        l1.y() = l1.y() * std::exp(hit.Material.AbsorptionCoefficient.y() * rmphit.T * -1);
                        l1.z() = l1.z() * std::exp(hit.Material.AbsorptionCoefficient.z() * rmphit.T * -1);
                    }
                    color = color + l1;
                }
                else
                {
                    float cphi = std::sqrt(cphi2);
                    Vector3f reftrac = (ray.Direction + normal * ctheta) * (n1/n2) - normal * cphi;
                    reftrac.normalize();
                    float r1 = (n2 * ctheta - n1 * cphi) / (n2 * ctheta + n1 * cphi);
                    float r2 = (n1 * ctheta - n2 * cphi) / (n1 * ctheta + n2 * cphi);   
                    float fr = (r1*r1 + r2*r2) / 2;
                    float ft = 1 - fr;
                    Ray rray = Ray(hit.Point + normal * ShadowRayEpsilon, reflect, ray.Time);
                    rray.N = ray.N;
                    Vector3f l1 = Trace(rray, cam, depth - 1, xy) * fr;
                    Ray tray = Ray(hit.Point - normal * ShadowRayEpsilon, reftrac, ray.Time);
                    tray.N = n2;
                    Vector3f l0 = Trace(tray, cam, depth - 1, xy) * ft;
                    RayHit tmphit, rmphit;
                    if(ray.N == 1)
                    {
                        // reftracing into dielectric
                        // apply beer law for reftracted val
                        RayCast(tray, tmphit, FLT_MAX, false);
                        l0.x() = l0.x() * std::exp(hit.Material.AbsorptionCoefficient.x() * tmphit.T * -1);
                        l0.y() = l0.y() * std::exp(hit.Material.AbsorptionCoefficient.y() * tmphit.T * -1);
                        l0.z() = l0.z() * std::exp(hit.Material.AbsorptionCoefficient.z() * tmphit.T * -1);
                    }
                    else
                    {
                        // reftracting into vacuum
                        // apply beer law for reflected val
                        RayCast(rray, rmphit, FLT_MAX, false);
                        l1.x() = l1.x() * std::exp(hit.Material.AbsorptionCoefficient.x() * rmphit.T * -1);
                        l1.y() = l1.y() * std::exp(hit.Material.AbsorptionCoefficient.y() * rmphit.T * -1);
                        l1.z() = l1.z() * std::exp(hit.Material.AbsorptionCoefficient.z() * rmphit.T * -1);
                    }                    
                    color = color + l0 + l1;
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
                auto cl = Trace(rray, cam, depth - 1, xy);
                color = color + hit.Material.MirrorReflectance.cwiseProduct(cl) * fr;
            }

            if(ray.N != 1)
                return color;

            auto ls = dynamic_cast<LightSphere*>(hit.Object);
            auto lm = dynamic_cast<LightMesh*>(hit.Object);
            if(ls != nullptr)
            {
                color += ls->Radiance;
            }
            else if(lm != nullptr)
            {
                color += lm->Radiance;
            }
            else
            {
                color += hit.Material.Shade(*this, ray, hit, cam.Gamma);
            }            
        }
        else
        {
            if(BackTexture != nullptr)
            {
                SamplerData data;
                data.u = (float)xy.x() / cam.ImageResolution.x();
                data.v = (float)xy.y() / cam.ImageResolution.y();
                color = BackTexture->Sample(data) * 255;
            }
            else if(environmentLight != nullptr)
            {
                color = environmentLight->GetColor(ray.Direction);
            }
            else
            {
                color = BackgroundColor;
            }
        }  
        return color;
    }

    void Scene::Render(int numThreads)
    {
        for(auto& cam: Cameras)
        {
            std::vector<unsigned char> pixels;
            std::vector<Vector3f> fpixels;
            if(cam.Tonemap)
            {
                fpixels.resize(cam.ImageResolution.x() * cam.ImageResolution.y());
            }
            else
            {
                pixels.resize(cam.ImageResolution.x() * cam.ImageResolution.y() * 4);
            }
            int size = cam.ImageResolution.x() * cam.ImageResolution.y();
            int cores = std::thread::hardware_concurrency();
            volatile std::atomic<int> count(0);
            std::vector<std::future<void>> futures;
            while(cores--)
            {
                futures.emplace_back(
                    std::async([=, &count, &cam, &pixels, &fpixels]()
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
                            Vector2i xy(x, y);
                            for(int r = 0; r < rays.size(); r++)
                            {                                
                                cl += Trace(rays[r], cam, MaxRecursionDepth, xy);
                            }
                            cl /= rays.size();
                            if(cam.Tonemap)
                            {
                                fpixels[index] = cl;
                            }
                            else
                            {                            
                                pixels[4 * index] = cl.x() > 255 ? 255 : cl.x();
                                pixels[4 * index + 1] = cl.y() > 255 ? 255 : cl.y();
                                pixels[4 * index + 2] = cl.z() > 255 ? 255 : cl.z();
                                pixels[4 * index + 3] = 255;
                            }
                        }
                    }
                    )
                );
            }
            futures.clear();
            if(!cam.Tonemap)
            {
                std::vector<unsigned char> png;
                lodepng::encode(png, pixels, cam.ImageResolution.x(), cam.ImageResolution.y());
                lodepng::save_file(png, cam.ImageName);
            }
            else
            {
                WriteEXR(fpixels, cam.ImageResolution.x(), cam.ImageResolution.y(), cam.ImageName.c_str());
                // tonemap
                std::vector<unsigned char> px;
                px.resize(fpixels.size() * 4);
                cam.toneMapper->Map(fpixels, px);
                std::vector<unsigned char> png;
                lodepng::encode(png, px, cam.ImageResolution.x(), cam.ImageResolution.y());
                lodepng::save_file(png, cam.ImageName.append(".png"));
            }            
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