#pragma once
#include "Eigen/Dense"
#include <algorithm>

namespace raytracer{


    class ToneMapper
    {
        public:
            ToneMapper(pugi::xml_node node)
            {

            }
            virtual void Map(std::vector<Vector3f>& pixels, std::vector<unsigned char>& data) = 0;

    };

    class PhotographicToneMapper : public ToneMapper
    {
        public:
            PhotographicToneMapper(pugi::xml_node node) : ToneMapper(node)
            {
                auto options = Vec2fFrom(node.child("TMOOptions"));
                _kv = options.x();
                _burn = options.y();
                auto g = node.child("Gamma").text().as_string();
                _sat = node.child("Saturation").text().as_float();
                if(std::strcmp(g, "sRGB") == 0)
                    _srgb = true;
                else
                 _gamma = node.child("Gamma").text().as_float();

            }
        private:
            float _kv;
            float _burn;
            float _gamma;
            float _sat;
            bool _srgb;
            float luminance(Vector3f& rgb)
            {
                return 0.212671 * rgb.x() + 0.71516 * rgb.y() + 0.072169 * rgb.z();
            }
            float linear_to_srgb(float linear) 
            {
                float srgb;
                if (linear <= 0.0031308f) {
                    srgb = linear * 12.92f;
                } else {
                    srgb = 1.055f * std::powf(linear, 1.0f / 2.4f) - 0.055f;
                }
                return srgb * 255.f;
            }
        public:
            virtual void Map(std::vector<Vector3f>& pixels, std::vector<unsigned char>& data) override
            {
                
                std::vector<float> ls(pixels.size(), 0);
                std::vector<float> lws(pixels.size(), 0);
                for(int i = 0; i < pixels.size(); i++)
                {
                    if(pixels[i].x() < 0 || pixels[i].y() < 0 || pixels[i].z() < 0)
                        pixels[i] = Vector3f::Zero();
                    lws[i] = luminance(pixels[i]);
                }
                float avglw = 0;
                for(int i = 0; i < lws.size(); i++)
                {
                    if(lws[i] > 0)
                        avglw += std::log(lws[i] + 1e-6);
                }
                avglw = std::exp(avglw / ls.size());
                for(int i = 0; i < ls.size(); i++)
                {
                    ls[i] = (_kv / avglw) * lws[i];
                }
                std::vector<float> lss(ls);
                std::sort(lss.begin(), lss.end());
                float ratio = (100 - _burn) / 100;
                int rn = ls.size() * ratio - 1;
                float lwhite = lss[rn] * lss[rn];
                for(int i = 0; i < ls.size(); i++)
                {
                    auto tmp = (ls[i] * (1.0f + (ls[i] / lwhite))) / (1.0f + ls[i]);
                    ls[i] = tmp;
                }      
                float g = 1/_gamma;
                for(int i = 0; i < pixels.size(); i++)
                {                    
                    auto temp = pixels[i];
                    temp.x() = std::clamp(powf(temp.x() / lws[i], _sat) * ls[i], 0.0f, 1.0f);
                    temp.y() = std::clamp(powf(temp.y() / lws[i], _sat) * ls[i], 0.0f, 1.0f);
                    temp.z() = std::clamp(powf(temp.z() / lws[i], _sat) * ls[i], 0.0f, 1.0f);

                    unsigned char rd;
                    unsigned char gd;
                    unsigned char bd; 
                    if(!_srgb)
                    {
                        rd = (int)floor(pow(temp.x(),  g) * 255.0f);
                        gd = (int)floor(pow(temp.y(),  g) * 255.0f);
                        bd = (int)floor(pow(temp.z(),  g) * 255.0f);
                    }
                    else
                    {
                        rd = (int)linear_to_srgb(temp.x());
                        gd = (int)linear_to_srgb(temp.y());
                        bd = (int)linear_to_srgb(temp.z());
                    }

                    data[i*4]     = rd;
                    data[i*4 + 1] = gd;
                    data[i*4 + 2] = bd;
                    data[i*4 + 3] = 255;
                } 
            }

    };

    class FilmicTonemapper : public ToneMapper
    {
        public:
            FilmicTonemapper(pugi::xml_node node) : ToneMapper(node)
            {
                auto options = Vec2fFrom(node.child("TMOOptions"));
                _exposureBias = options.x();
                _w = options.y();
                _gamma = node.child("Gamma").text().as_float();
            }
        private:
            
            float _exposureBias;
            float _w;
            float _gamma;

            Array3f uncharted2_tonemap_partial(Array3f x)
            {
                float A = 0.15f;
                float B = 0.50f;
                float C = 0.10f;
                float D = 0.20f;
                float E = 0.02f;
                float F = 0.30f;
                return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
            }

            Vector3f uncharted2_filmic(Vector3f v)
            {
                Array3f curr = uncharted2_tonemap_partial(v * _exposureBias);
                Array3f W = Array3f(_w, _w, _w);
                Array3f white_scale = Array3f(1.0f, 1.0f, 1.0f) / uncharted2_tonemap_partial(W);
                return curr * white_scale;
            }

        public:
            virtual void Map(std::vector<Vector3f>& pixels, std::vector<unsigned char>& data) override
            {                
                for(int i = 0; i < pixels.size(); i++)
                {
                    pixels[i] = uncharted2_filmic(pixels[i]);                    
                }
                float g = 1/_gamma;
                for(int i = 0; i < pixels.size(); i++)
                {
                    auto temp = pixels[i];
                    temp.x() = std::clamp(pixels[i].x(), 0.0f, 1.0f);
                    temp.y() = std::clamp(pixels[i].y(), 0.0f, 1.0f);
                    temp.z() = std::clamp(pixels[i].z(), 0.0f, 1.0f);

                    unsigned char rd = (int)floor(pow(temp.x(),  g) * 255.0f);
                    unsigned char gd = (int)floor(pow(temp.y(),  g) * 255.0f);
                    unsigned char bd = (int)floor(pow(temp.z(),  g) * 255.0f);

                    data[i*4]     = rd;
                    data[i*4 + 1] = gd;
                    data[i*4 + 2] = bd;
                    data[i*4 + 3] = 255;
                } 

            }
    };

    class ACESToneMapper : public ToneMapper
    {
        public:
            ACESToneMapper(pugi::xml_node node) : ToneMapper(node)
            {
                auto options = Vec2fFrom(node.child("TMOOptions"));
                _exposureBias = options.x();
                _gamma = node.child("Gamma").text().as_float();
                aces_input_matrix <<
                    0.59719f, 0.35458f, 0.04823f,
                    0.07600f, 0.90834f, 0.01566f,
                    0.02840f, 0.13383f, 0.83777f;

                aces_output_matrix <<
                     1.60475f, -0.53108f, -0.07367f,
                    -0.10208f,  1.10813f, -0.00605f,
                    -0.00327f, -0.07276f,  1.07602f;
            }
        private:
            float _exposureBias;
            float _gamma;
            Matrix3f aces_input_matrix;
            Matrix3f aces_output_matrix;

            Array3f rtt_and_odt_fit(Array3f v)
            {
                Array3f a = v * (v + 0.0245786f) - 0.000090537f;
                Array3f b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
                return a / b;
            }

            Vector3f aces_fitted(Vector3f v)
            {
                v = aces_input_matrix * v;
                v = rtt_and_odt_fit(v);
                return aces_output_matrix * v;
            }
        public:
            virtual void Map(std::vector<Vector3f>& pixels, std::vector<unsigned char>& data) override
            {                
                for(int i = 0; i < pixels.size(); i++)
                {
                    pixels[i] = aces_fitted(pixels[i] * _exposureBias);                    
                }
                float g = 1/_gamma;
                for(int i = 0; i < pixels.size(); i++)
                {
                    auto temp = pixels[i];
                    temp.x() = std::clamp(pixels[i].x(), 0.0f, 1.0f);
                    temp.y() = std::clamp(pixels[i].y(), 0.0f, 1.0f);
                    temp.z() = std::clamp(pixels[i].z(), 0.0f, 1.0f);

                    unsigned char rd = (int)floor(pow(temp.x(),  g) * 255.0f);
                    unsigned char gd = (int)floor(pow(temp.y(),  g) * 255.0f);
                    unsigned char bd = (int)floor(pow(temp.z(),  g) * 255.0f);

                    data[i*4]     = rd;
                    data[i*4 + 1] = gd;
                    data[i*4 + 2] = bd;
                    data[i*4 + 3] = 255;
                } 

            }
    };

}