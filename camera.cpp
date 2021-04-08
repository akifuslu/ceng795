#include "camera.h"

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

        Gaze.Normalize();
        Up.Normalize();
        img_center = Position + Gaze * NearDistance;
        v = Vector3f::Cross(Gaze, Up);
        q = img_center + Up * NearPlane.W + v * NearPlane.X;
    }

    Ray Camera::GetRay(int x, int y)
    {
        float sv = (x + .5) * ((NearPlane.Y - NearPlane.X) / ImageResolution.X);
        float su = (y + .5) * ((NearPlane.W - NearPlane.Z) / ImageResolution.Y);
        Vector3f s = q + (v * sv) - (Up * su);
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