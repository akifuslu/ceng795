#include "vector.h"
#include <sstream>
#include <cmath>

namespace raytracer
{
    Vector3f::Vector3f() : X(0), Y(0), Z(0) {}
    Vector3f::Vector3f(float x, float y, float z) : X(x), Y(y), Z(z) {}
    Vector3f::Vector3f(pugi::xml_node node)
    {
        std::stringstream stream(node.first_child().value());
        stream >> X >> Y >> Z;
    }

    std::ostream& operator<<(std::ostream& os, const Vector3f& v)
    {
        os << v.X << ',' << v.Y << ',' << v.Z;
        return os;
    }

    Vector3f Vector3f::operator+(const Vector3f& v) const
    {
        return Vector3f(X + v.X, Y + v.Y, Z + v.Z);
    }

    Vector3f Vector3f::operator-(const Vector3f& v) const
    {
        return Vector3f(X - v.X, Y - v.Y, Z - v.Z);
    }

    Vector3f Vector3f::operator*(const Vector3f& v) const
    {
        return Vector3f(X * v.X, Y * v.Y, Z * v.Z);
    }

    Vector3f Vector3f::operator*(const float& f) const
    {
        return Vector3f(X * f, Y * f, Z * f);
    }

    Vector3f Vector3f::operator/(const float& f) const
    {
        return Vector3f(X / f, Y / f, Z / f);
    }

    float Vector3f::Magnitude() const
    {
        return std::sqrt(Vector3f::Dot(*this, *this));
    }

    Vector3f Vector3f::Normalized() const
    {
        return (*this) / Magnitude();
    }

    void Vector3f::Normalize()
    {
        *this = Normalized();
    }

    Vector3f Vector3f::Cross(const Vector3f& a, const Vector3f& b)
    {
        return Vector3f(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
    }

    float Vector3f::Dot(const Vector3f& a, const Vector3f& b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    }

    Vector4f::Vector4f() : X(0), Y(0), Z(0), W(0) {}
    Vector4f::Vector4f(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    Vector4f::Vector4f(pugi::xml_node node)
    {
        std::stringstream stream(node.first_child().value());
        stream >> X >> Y >> Z >> W;
    }

    std::ostream& operator<<(std::ostream& os, const Vector4f& v)
    {
        os << v.X << ',' << v.Y << ',' << v.Z << ',' << v.W;
        return os;
    }

    Vector2i::Vector2i() : X(0), Y(0) {}
    Vector2i::Vector2i(int x, int y) : X(x), Y(y) {}
    Vector2i::Vector2i(pugi::xml_node node)
    {
        std::stringstream stream(node.first_child().value());
        stream >> X >> Y;
    }

    std::ostream& operator<<(std::ostream& os, const Vector2i& v)
    {
        os << v.X << ',' << v.Y;
        return os;
    }

    Vector3i::Vector3i() : X(0), Y(0), Z(0) {}
    Vector3i::Vector3i(int x, int y, int z) : X(x), Y(y), Z(z) {}
    Vector3i::Vector3i(pugi::xml_node node)
    {
        std::stringstream stream(node.first_child().value());
        stream >> X >> Y >> Z;
    }

    std::ostream& operator<<(std::ostream& os, const Vector3i& v)
    {
        os << v.X << ',' << v.Y << ',' << v.Z;
        return os;
    }

}