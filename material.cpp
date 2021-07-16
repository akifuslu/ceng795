#include "material.h"
#include <iostream>
#include "scene.h"
#include "ray.h"
#include "object.h"

namespace raytracer
{

    Material::Material(){}

    Material::Material(pugi::xml_node node)
    {
        Degamma = node.attribute("degamma").as_bool(false);        
        AmbientReflectance = Vec3fFrom(node.child("AmbientReflectance"));
        DiffuseReflectance = Vec3fFrom(node.child("DiffuseReflectance"));
        SpecularReflectance = Vec3fFrom(node.child("SpecularReflectance"));                
        PhongExponent = node.child("PhongExponent").text().as_float();
        MirrorReflectance = Vec3fFrom(node.child("MirrorReflectance"));
        RefractionIndex = node.child("RefractionIndex").text().as_float();
        AbsorptionIndex = node.child("AbsorptionIndex").text().as_float();
        AbsorptionCoefficient = Vec3fFrom(node.child("AbsorptionCoefficient"));
        Roughness = node.child("Roughness").text().as_float();
        if(std::strcmp(node.attribute("type").as_string(), "conductor") == 0)
        {
            Type = 1;            
        }
        else if(std::strcmp(node.attribute("type").as_string(), "dielectric") == 0)
        {
            Type = 2;            
        }
        else if(std::strcmp(node.attribute("type").as_string(), "mirror") == 0)
        {
            Type = 3;            
        }
        else
        {
            Type = 0;
        }        

        int brdf = node.attribute("BRDF").as_int(-1);
        if(brdf != -1)
        {
            Brdf = ResourceLocator::GetInstance().GetBRDF(brdf);
        }
        else
        {
            Brdf = new OriginalBlinnPhong();
            Brdf->Exponent = PhongExponent;
        }
        Brdf->n = RefractionIndex;
        Brdf->k = AbsorptionIndex;
    }

    std::ostream& operator<<(std::ostream& os, const Material& mat)
    {
        os << "Ambient " << mat.AmbientReflectance << std::endl;
        os << "Diffuse " << mat.DiffuseReflectance << std::endl;
        os << "Specular " << mat.SpecularReflectance << std::endl;
        os << "Phong " << mat.PhongExponent << std::endl;
        os << "Type " << mat.Type << std::endl;
        os << "Mirror " << mat.MirrorReflectance << std::endl;
        return os;
    }

    Vector3f Material::Shade(Scene& scene, Ray& ray, RayHit& hit, float gamma)
    {
        Vector3f color = Vector3f::Zero();
        SamplerData data;
        data.point = hit.Point;
        data.u = hit.u;
        data.v = hit.v;
        if(hit.Texture != nullptr && hit.Texture->Mode == DecalMode::REPLACE_ALL)
        {
            return hit.Texture->Color(data);
        }
        Vector3f ka = AmbientReflectance;
        Vector3f kd = DiffuseReflectance;
        Vector3f ks = SpecularReflectance;
        if(Degamma)
        {
            ka = Vec3Pow(ka, gamma);
            kd = Vec3Pow(kd, gamma);
            ks = Vec3Pow(ks, gamma);
        }
        if(hit.Texture != nullptr)
        {
            if(hit.Texture->Mode == DecalMode::REPLACE_KD)
            {                        
                kd = hit.Texture->Color(data);
            }
            else if(hit.Texture->Mode == DecalMode::BLEND_KD)
            {
                kd = (kd + hit.Texture->Color(data)) / 2;
            }
        }            
        color += ka.cwiseProduct(scene.ambientLight.Intensity);
        for(int l = 0; l < scene.Lights.size(); l++)
        {
            auto light = scene.Lights[l];                    
            Vector3f sp = hit.Point + hit.Normal * scene.ShadowRayEpsilon;
            Vector3f lsample;
            Vector3f ldir;
            float r = light->SamplePoint(sp, hit.Normal, lsample, ldir);
            // SHADOW CHECK                
            Ray sRay = Ray(sp, ldir, ray.Time);                
            auto obj = dynamic_cast<Object*>(light);
            if(obj != nullptr)
                sRay.Ignore = obj->Id;
            else
                sRay.Ignore = -1;
            RayHit sHit;
            bool sf = scene.RayCast(sRay, sHit, r, false);
            if(sf && sHit.T < r)
                continue;

            auto viewDir = (ray.Origin - hit.Point).normalized();
            auto lum = light->GetLuminance(hit.Point, hit.Normal, lsample);
            color += Brdf->Shade(kd, ks, ldir, hit.Normal, viewDir, lum);
        }
        return color;
    }


    BRDF::BRDF() {}

    BRDF::BRDF(pugi::xml_node node)
    {
        Exponent = node.child("Exponent").text().as_float();
    }

    OriginalPhong::OriginalPhong(pugi::xml_node node) : BRDF(node) {}

    ModifiedPhong::ModifiedPhong(pugi::xml_node node) : BRDF(node) 
    {
        Normalized = node.attribute("normalized").as_bool(false);
    }

    OriginalBlinnPhong::OriginalBlinnPhong(pugi::xml_node node) : BRDF(node) {}

    ModifiedBlinnPhong::ModifiedBlinnPhong(pugi::xml_node node) : BRDF(node) 
    {
        Normalized = node.attribute("normalized").as_bool(false);
    }

    TorranceSparrow::TorranceSparrow(pugi::xml_node node) : BRDF(node) 
    {
        Kdfresnel = node.attribute("kdfresnel").as_bool(false);
    }

    Vector3f OriginalPhong::Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance)
    {
        Vector3f color = Vector3f::Zero();
        // DIFFUSE
        float teta = lightDir.dot(normal);
        teta = teta < 0 ? 0 : teta;
        color += kd.cwiseProduct(luminance) * teta;                            
        // SPECULAR
        auto r = Reflect(lightDir, normal, 0);
        float cosar = r.dot(-viewDir);
        cosar = cosar < 0 ? 0 : cosar;
        color += ks.cwiseProduct(luminance) * std::pow(cosar, Exponent);
        return color;
    }

    Vector3f ModifiedPhong::Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance)
    {
        Vector3f color = Vector3f::Zero();
        // DIFFUSE
        float teta = lightDir.dot(normal);        
        teta = teta < 0 ? 0 : teta;
        if(Normalized)
            color += kd.cwiseProduct(luminance) * teta / M_PI;
        else
            color += kd.cwiseProduct(luminance) * teta;                            
        // SPECULAR
        auto r = Reflect(lightDir, normal, 0);
        float cosar = r.dot(-viewDir);
        cosar = cosar < 0 ? 0 : cosar;
        if(Normalized)
            color += ks.cwiseProduct(luminance) * std::pow(cosar, Exponent) * teta * ((Exponent + 2) / (2 * M_PI));
        else        
            color += ks.cwiseProduct(luminance) * std::pow(cosar, Exponent) * teta;
        return color;
    }

    Vector3f OriginalBlinnPhong::Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance)
    {
        Vector3f color = Vector3f::Zero();
        // DIFFUSE
        float teta = lightDir.dot(normal);
        teta = teta < 0 ? 0 : teta;
        color += kd.cwiseProduct(luminance) * teta;                            
        // SPECULAR
        Vector3f h = (lightDir + viewDir).normalized();
        teta = h.dot(normal);
        teta = teta < 0 ? 0 : teta;
        teta = std::pow(teta, Exponent);
        color += ks.cwiseProduct(luminance) * teta;
        return color;
    }

    Vector3f ModifiedBlinnPhong::Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance)
    {
        Vector3f color = Vector3f::Zero();
        // DIFFUSE
        float teta = lightDir.dot(normal);
        teta = teta < 0 ? 0 : teta;
        if(Normalized)
            color += kd.cwiseProduct(luminance) * teta / M_PI;                            
        else
            color += kd.cwiseProduct(luminance) * teta;                            
        // SPECULAR
        Vector3f h = (lightDir + viewDir).normalized();
        float phi = h.dot(normal);
        phi = phi < 0 ? 0 : phi;
        phi = std::pow(phi, Exponent);
        if(Normalized)
            color += ks.cwiseProduct(luminance) * teta * phi * ((Exponent+8)/(8*M_PI));
        else
            color += ks.cwiseProduct(luminance) * teta * phi;
        return color;
    }

    Vector3f TorranceSparrow::Shade(Vector3f kd, Vector3f ks, Vector3f lightDir, Vector3f normal, Vector3f viewDir, Vector3f luminance)
    {
        Vector3f color = Vector3f::Zero();
        float NdotL = std::max(0.f, normal.dot(lightDir));
        float Rs = 0.0;
        if (NdotL > 0) 
        {
            auto H = (lightDir + viewDir).normalized();
            float NdotH = std::max(0.f, normal.dot(H));
            float NdotV = std::max(0.f, normal.dot(viewDir));
            float VdotH = std::max(0.f, lightDir.dot(H));

            // Fresnel reflectance
            auto ndi = NdotV;
            float rs = ((n*n + k*k) - 2 * n * ndi + ndi * ndi) / ((n*n + k*k) + 2 * n * ndi + ndi * ndi);
            float rp = ((n*n + k*k) * ndi*ndi - 2*n*ndi + 1) / ((n*n + k*k) * ndi*ndi + 2*n*ndi + 1);
            float F = (rs + rp) / 2;
            
            //float F0 = (n-1)/(n+1);
            //F0 *= F0;
            //float F = pow(1.0 - VdotH, 5.0);
            //F *= (1.0 - F0);
            //F += F0;

            // Microfacet distribution by Blinn
            float D = (Exponent + 2) * pow(NdotH, Exponent) / (2 * M_PI);

            // Geometric shadowing
            float two_NdotH = 2.0 * NdotH;
            float g1 = (two_NdotH * NdotV) / VdotH;
            float g2 = (two_NdotH * NdotL) / VdotH;
            float G = std::min(1.0f, std::min(g1, g2));

            Rs = (F * D * G) / (4 * NdotL * NdotV);
            float kdf = Kdfresnel ? (1 - F) : 1;
            return kd.cwiseProduct(luminance) * NdotL * M_1_PI * kdf  + ks.cwiseProduct(luminance) * NdotL * Rs;
        }
        return color;
    }
}