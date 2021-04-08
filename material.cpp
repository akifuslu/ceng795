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
    }

    std::ostream& operator<<(std::ostream& os, const Material& mat)
    {
        os << "Ambient " << mat.AmbientReflectance << std::endl;
        os << "Diffuse " << mat.DiffuseReflectance << std::endl;
        os << "Specular " << mat.SpecularReflectance << std::endl;
        os << "Phong " << mat.PhongExponent << std::endl;
        return os;
    }
}