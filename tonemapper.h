#pragma once
#include "Eigen/Dense"
#include "camera.h"
#include <algorithm>

namespace raytracer{


    class PhotographicToneMapper
    {
        static float luminance(Vector3f& rgb)
        {
            return 0.212671 * rgb.x() + 0.71516 * rgb.y() + 0.072169 * rgb.z();
        }
        public:
            static void Map(std::vector<Vector3f>& pixels, const Camera& cam, std::vector<unsigned char>& data)
            {
                
                std::vector<float> ls(pixels.size(), 0);
                std::vector<float> lws(pixels.size(), 0);
                for(int i = 0; i < pixels.size(); i++)
                {
                    lws[i] = luminance(pixels[i]);
                }
                float avglw = 0;
                for(int i = 0; i < lws.size(); i++)
                {
                    avglw += std::log(lws[i] + 1e-6);
                }
                avglw = std::exp(avglw / ls.size());
                for(int i = 0; i < ls.size(); i++)
                {
                    ls[i] = (cam.KV / avglw) * lws[i];
                }
                std::vector<float> lss(ls);
                std::sort(lss.begin(), lss.end());
                float ratio = (100 - cam.BurnPercent) / 100;
                int rn = ls.size() * ratio - 1;
                float lwhite = lss[rn] * lss[rn];
                for(int i = 0; i < ls.size(); i++)
                {
                    auto tmp = (ls[i] * (1.0f + (ls[i] / lwhite))) / (1.0f + ls[i]);
                    ls[i] = tmp;
                }      
                float g = 1/cam.Gamma;
                for(int i = 0; i < pixels.size(); i++)
                {
                    auto temp = pixels[i];
                    temp.x() = std::clamp(powf(temp.x() / lws[i], cam.Saturation) * ls[i], 0.0f, 1.0f);
                    temp.y() = std::clamp(powf(temp.y() / lws[i], cam.Saturation) * ls[i], 0.0f, 1.0f);
                    temp.z() = std::clamp(powf(temp.z() / lws[i], cam.Saturation) * ls[i], 0.0f, 1.0f);

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