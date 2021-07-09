#pragma once
#include<vector>

namespace raytracer
{
    class Image;
    class BRDF;

    class ResourceLocator
    {
    public:
        static ResourceLocator& GetInstance()
        {
            static ResourceLocator instance;
            return instance;
        }
        ResourceLocator(ResourceLocator const&) = delete;
        void operator=(ResourceLocator const&)  = delete;       

        std::vector<Image*> _images;
        inline void AddImage(Image* image)
        {
            _images.push_back(image);
        }

        inline Image* GetImage(int id)
        {
            return _images[id - 1];
        }

        std::vector<BRDF*> _brdfs;
        inline void AddBRDF(BRDF* brdf)
        {
            _brdfs.push_back(brdf);
        }

        inline BRDF* GetBRDF(int id)
        {
            return _brdfs[id - 1];
        }

    private:
        ResourceLocator() {}
    };
}