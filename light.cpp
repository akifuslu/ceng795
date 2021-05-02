#include "light.h"

namespace raytracer
{
    AmbientLight::AmbientLight(pugi::xml_node node)
    {
        Intensity = Vec3fFrom(node);
    }    

    PointLight::PointLight(pugi::xml_node node)
    {
        Intensity = Vec3fFrom(node.child("Intensity"));
        Position = Vec3fFrom(node.child("Position"));
    }
}