#include "material.h"

namespace raytracer
{

    Material::Material(){}

    Material::Material(pugi::xml_node node)
    {
        AmbientReflectance = Vector3f(node.child("AmbientReflectance"));
        DiffuseReflectance = Vector3f(node.child("DiffuseReflectance"));
        SpecularReflectance = Vector3f(node.child("SpecularReflectance"));                
        PhongExponent = node.child("PhongExponent").text().as_float();
        MirrorReflectance = Vector3f(node.child("MirrorReflectance"));
        RefractionIndex = node.child("RefractionIndex").text().as_float();
        AbsorptionIndex = node.child("AbsorptionIndex").text().as_float();
        AbsorptionCoefficient = Vector3f(node.child("AbsorptionCoefficient"));
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
}