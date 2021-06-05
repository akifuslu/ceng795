#pragma once
#include<vector>

namespace raytracer
{
    class Image;

    class ImageLocator
    {
    public:
        static ImageLocator& GetInstance()
        {
            static ImageLocator instance;
            return instance;
        }
        ImageLocator(ImageLocator const&) = delete;
        void operator=(ImageLocator const&)  = delete;       

        std::vector<Image*> _images;
        inline void AddImage(Image* image)
        {
            _images.push_back(image);
        }

        inline Image* GetImage(int id)
        {
            return _images[id - 1];
        }

    private:
        ImageLocator() {}
    };
}