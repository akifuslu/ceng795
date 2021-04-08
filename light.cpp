#include "light.h"

namespace raytracer
{
    AmbientLight::AmbientLight(pugi::xml_node node)
    {
        Intensity = Vector3f(node);
    }    

    PointLight::PointLight(pugi::xml_node node)
    {
        Intensity = Vector3f(node.child("Intensity"));
        Position = Vector3f(node.child("Position"));
    }
}