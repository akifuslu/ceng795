#pragma once
#include "lodepng.h"
#include "jpeg.h"
#include "pugixml.hpp"
#include <string>
#include <vector>
#include "imagelocator.h"
#include "Eigen/Dense"
#include <random>
#include <algorithm>

using namespace Eigen;

namespace raytracer
{

    enum ImageMode{
        RGB,
        RGBA
    };

    enum TextureType{
        IMAGE,
        PERLIN
    };

    enum DecalMode{
        REPLACE_KD,
        BLEND_KD,
        REPLACE_ALL
    };

    enum Interpolation{
        NEAREST,
        BILINEAR
    };

    enum NoiseConversion{
        LINEAR,
        ABSVAL
    };

    struct SamplerData
    {
        float u, v;
        Vector3f point;
    };

    class Image
    {
        public:
            Image(pugi::xml_node node);
            std::vector<unsigned char> Pixels;
            Vector3f Fetch(int x, int y);
            unsigned Width = 0;
            unsigned Height = 0;
            ImageMode Mode;
    };

    class Perlin
    {
        public:
            Perlin();
    };

    class Sampler
    {
        public:
            virtual Vector3f Sample(SamplerData& data) {return Vector3f::Zero();}
            virtual Vector3f SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& n, float f)
            {
                return Vector3f::Zero();
            }
    };

    class ImageSampler : public Sampler
    {
        public:
            ImageSampler(pugi::xml_node node);
            Image* Image;
            Interpolation Interpolation = Interpolation::NEAREST;
            float Normalizer;
            virtual Vector3f Sample(SamplerData& data) override;
            virtual Vector3f SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& n, float f) override;
    };

    class PerlinSampler : public Sampler
    {
        public:
            PerlinSampler(pugi::xml_node node);
            NoiseConversion Conversion;
            float NoiseScale;
            virtual Vector3f Sample(SamplerData& data) override;
            virtual Vector3f SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& n, float f) override;
        private:
            int _tableSize = 16;
            std::vector<int> _ptable;
            std::vector<Vector3f> _grad;
            inline int hash(int x, int y, int z)
            {
                return _ptable[_ptable[_ptable[x] + y] + z];   
            }
            inline float f(float t)
            {
                return t * t * t * (t * (t * 6 - 15) + 10);
            }
            inline float lerp(float a, float b, float t)
            {
                t = t > 1 ? 1 : t;
                t = t < 0 ? 0 : t;
                return a + (b - a) * t;
            }
    };    

    class Texture
    {
        public:
            Texture(pugi::xml_node node);
            void Load(std::vector<Image*>& images);
            Sampler* Sampler;
            virtual void Test() {}
        private:
            std::string _type;
    };

    class BackgroundTexture : public Texture
    {
        public:
            BackgroundTexture(pugi::xml_node node);
    };

    class DiffuseTexture : public Texture
    {
        public:
            DiffuseTexture(pugi::xml_node node);
            DecalMode Mode;
            Vector3f Color(SamplerData& data);
    };

    class NormalTexture : public Texture
    {
        public:
            NormalTexture(pugi::xml_node node);
            Vector3f SampleNormal(SamplerData data);
    };

    class BumpTexture : public Texture
    {
        public:
            BumpTexture(pugi::xml_node node);
            Vector3f SampleBump(SamplerData& data, Matrix<float, 3, 3>& tbn);
            float Factor;
    };
}