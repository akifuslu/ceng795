#pragma once
#include "pugixml.hpp"
#include <iostream>

namespace raytracer
{
    class Vector3f
    {
        public:
            Vector3f();
            Vector3f(float x, float y, float z);
            Vector3f(pugi::xml_node node);
            float X;
            float Y;
            float Z;
            friend std::ostream& operator<<(std::ostream& os, const Vector3f& v);
            Vector3f operator+(const Vector3f& v) const;
            Vector3f operator-(const Vector3f& v) const;
            Vector3f operator*(const Vector3f& v) const;
            Vector3f operator*(const float& f) const;
            Vector3f operator/(const float& f) const;
            float Magnitude() const;
            Vector3f Normalized() const;
            void Normalize();            
            static Vector3f Cross(const Vector3f& a, const Vector3f& b);
            static float Dot(const Vector3f& a, const Vector3f& b);
    };

    class Vector4f
    {
        public:
            Vector4f();
            Vector4f(float x, float y, float z, float w);
            Vector4f(pugi::xml_node node);
            float X;
            float Y;
            float Z;
            float W;
            friend std::ostream& operator<<(std::ostream& os, const Vector4f& v);
    };

    class Vector2i
    {
        public:
            Vector2i();
            Vector2i(int x, int y);
            Vector2i(pugi::xml_node node);
            int X;
            int Y;
            friend std::ostream& operator<<(std::ostream& os, const Vector2i& v);
    };

    class Vector3i
    {
        public:
            Vector3i();
            Vector3i(int x, int y, int z);
            Vector3i(pugi::xml_node node);
            int X;
            int Y;
            int Z;
            friend std::ostream& operator<<(std::ostream& os, const Vector3i& v);
    };
}