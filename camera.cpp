#include "camera.h"
#include <cmath>
#include <iostream>

namespace raytracer
{
    Camera::Camera(pugi::xml_node node)
    {
        Position = Vector3f(node.child("Position"));
        Gaze = Vector3f(node.child("Gaze"));
        Up = Vector3f(node.child("Up"));
        NearPlane = Vector4f(node.child("NearPlane"));
        NearDistance = node.child("NearDistance").text().as_float();
        ImageResolution = Vector2i(node.child("ImageResolution"));
        ImageName = node.child("ImageName").text().as_string();
        GazePoint = Vector3f(node.child("GazePoint"));
        FovY = node.child("FovY").text().as_float();
        if(std::strcmp(node.attribute("type").as_string(), "lookAt") == 0)
        {
            Gaze = (GazePoint - Position);
            double pi = 3.14159265359;
            float rad = ((FovY * pi) / (2 * 180.0f));            
            auto tmp = std::tan(rad) * NearDistance;
            NearPlane.W = tmp;
            NearPlane.Z = - NearPlane.W;
            float aspect = (float)ImageResolution.X / (float)ImageResolution.Y;
            NearPlane.Y = NearPlane.W * aspect;
            NearPlane.X = -NearPlane.Y;
        }
        Gaze.Normalize();
        Up.Normalize();
        w = Gaze * -1;
        u = Vector3f::Cross(Up, w).Normalized();
        v = Vector3f::Cross(w, u).Normalized();
        img_center = Position - w * NearDistance;
        q = img_center + v * NearPlane.W + u * NearPlane.X;
    }

    Ray Camera::GetRay(int x, int y)
    {
        float su = (x + .5) * ((NearPlane.Y - NearPlane.X) / ImageResolution.X);
        float sv = (y + .5) * ((NearPlane.W - NearPlane.Z) / ImageResolution.Y);
        Vector3f s = q + (u * su) - (v * sv);
        return Ray(Position, (s - Position).Normalized());
    }

    std::ostream& operator<<(std::ostream& os, const Camera& cam)
    {
        os << "Position " << cam.Position << std::endl;
        os << "Gaze " << cam.Gaze << std::endl;
        os << "Up " << cam.Up << std::endl;
        os << "NearPlane " << cam.NearPlane << std::endl;
        os << "NearDistance " << cam.NearDistance << std::endl;                        
        os << "ImageResolution " << cam.ImageResolution << std::endl;                        
        os << "ImageName " << cam.ImageName << std::endl;  
        return os;                      
    }
}