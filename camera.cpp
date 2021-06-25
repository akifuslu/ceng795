#include "camera.h"
#include <cmath>
#include <iostream>

namespace raytracer
{
    Camera::Camera(pugi::xml_node node)
    {
        Position = Vec3fFrom(node.child("Position"));
        Gaze = Vec3fFrom(node.child("Gaze"));
        Up = Vec3fFrom(node.child("Up"));
        NearPlane = Vec4fFrom(node.child("NearPlane"));
        NearDistance = node.child("NearDistance").text().as_float();
        ImageResolution = Vec2iFrom(node.child("ImageResolution"));
        ImageName = node.child("ImageName").text().as_string();
        GazePoint = Vec3fFrom(node.child("GazePoint"));
        FovY = node.child("FovY").text().as_float();
        NumSamples = node.child("NumSamples").text().as_int(1);
        if(node.child("FocusDistance").text().as_float())
        {
            FocusDistance = node.child("FocusDistance").text().as_float();
            ApertureSize = node.child("ApertureSize").text().as_float();
            FocusEnabled = true;
        }
        if(node.child("Tonemap"))
        {
            Tonemap = true;
            auto opt = Vec2fFrom(node.child("Tonemap").child("TMOOptions"));
            KV = opt.x();
            BurnPercent = opt.y();
            Saturation = node.child("Tonemap").child("Saturation").text().as_float();
            Gamma = node.child("Tonemap").child("Gamma").text().as_float();
        }
        row = std::sqrt(NumSamples);
        col = NumSamples / row;
        if(std::strcmp(node.attribute("type").as_string(), "lookAt") == 0)
        {
            Gaze = (GazePoint - Position);
            double pi = 3.14159265359;
            float rad = ((FovY * pi) / (2 * 180.0f));            
            auto tmp = std::tan(rad) * NearDistance;
            NearPlane.w() = tmp;
            NearPlane.z() = - NearPlane.w();
            float aspect = (float)ImageResolution.x() / (float)ImageResolution.y();
            NearPlane.y() = NearPlane.w() * aspect;
            NearPlane.x() = -NearPlane.y();
        }
        Gaze.normalize();
        Up.normalize();
        w = Gaze * -1;
        u = Up.cross(w).normalized();
        v = w.cross(u).normalized();
        if(std::strcmp(node.attribute("handedness").as_string(), "left") == 0)
            u *= -1;
        img_center = Position - w * NearDistance;
        lu = img_center + v * NearPlane.w() + u * NearPlane.x();
        suv = ((NearPlane.y() - NearPlane.x()) / (float)ImageResolution.x());
        svv = ((NearPlane.w() - NearPlane.z()) / (float)ImageResolution.y());
    }

    std::vector<Ray> Camera::GetRay(int x, int y)
    {
        std::vector<Ray> samples;
        if(NumSamples <= 1)
        {
            float su = (x + .5) * suv;
            float sv = (y + .5) * svv;
            Vector3f s = lu + (u * su) - (v * sv);
            samples.push_back(Ray(Position, (s - Position).normalized()));
        }
        else
        {
            for(int i = 0; i < row; i++)
            {
                for(int j = 0; j < col; j++)
                {
                    float rx = (j + rnd(generator)) / col;
                    float ry = (i + rnd(generator)) / row;
                    float su = (x + rx) * suv;
                    float sv = (y + ry) * svv;
                    Vector3f q = lu + (u * su) - (v * sv);
                    float t = rnd(generator);
                    if(!FocusEnabled)
                    {
                        samples.push_back(Ray(Position, (q - Position).normalized(), t));
                    }
                    else
                    {
                        Vector3f dir = (q - Position).normalized();
                        float tfd = FocusDistance / dir.dot(-w);
                        Vector3f p = Position + dir * tfd;
                        float rsu = (rnd(generator) - .5f) * ApertureSize;
                        float rsv = (rnd(generator) - .5f) * ApertureSize;
                        Vector3f s = Position + rsu * u + rsv * v;
                        dir = (p - s).normalized();
                        samples.push_back(Ray(s, dir, t));
                    }                    
                }
            }
        }      
        return samples;
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