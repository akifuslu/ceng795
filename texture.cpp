#include "texture.h"
#include<iostream>
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

namespace raytracer{

    Image::Image(pugi::xml_node node)
    {
        auto name = std::string(node.text().as_string());
        int npos = name.find_last_of('.');
        auto extension = name.substr(npos + 1);
        if(extension.compare("png") == 0)
        {
            stride = 4;
            Mode = ImageMode::RGBA;
            auto code = lodepng::decode(Pixels, Width, Height, name);
            std::cout << lodepng_error_text(code) << std::endl;
        }
        else if(extension.compare("jpg") == 0)
        {
            Mode = ImageMode::RGB;
            int mode;
            read_jpeg_header(name.c_str(), Width, Height, mode);   
            if(mode == 0)
            {
                Mode = ImageMode::GRAYSCALE;
                stride = 1;
            }
            else if(mode == 1)
            {
                Mode = ImageMode::RGB;
                stride = 3;
            }
            else
            {
                std::cout << "UNSUPPORTED JPG FORMAT!" << std::endl;
                exit(0);
            }            
            Pixels.resize(Height * Width * stride);
            read_jpeg(name.c_str(), Pixels, Width, Height);     
        }
        else if(extension.compare("exr") == 0)
        {
            stride = 4;
            Mode = ImageMode::HDR;
            float* out; // width * height * RGBA
            const char* err = nullptr;
            int w, h;
            LoadEXR(&out, &w, &h, name.c_str(), &err);
            Width = w;
            Height = h;
            _hdr = new Hdr();
            _hdr->Pixels = out;
        }
        else
        {
            std::cout << "UNSUPPORTED IMAGE FORMAT!" << std::endl;
        }
    }

    Vector3f Image::Fetch(int x, int y)
    {
        if(x >= Width)
            x = Width - 1;
        if(y >= Height)
            y = Height - 1;
        Vector3f p;
        int o = stride * y * Width + stride * x;
        if(Mode == RGB || Mode == RGBA)
        {
            p.x() = Pixels[o];
            p.y() = Pixels[o + 1];
            p.z() = Pixels[o + 2];
        }
        else if(Mode == HDR)
        {
            p.x() = _hdr->Pixels[o];
            p.y() = _hdr->Pixels[o + 1];
            p.z() = _hdr->Pixels[o + 2];
        }
        else
        {
            p.x() = Pixels[o];
            p.y() = Pixels[o];
            p.z() = Pixels[o];            
        }        
        return p;
    }

    ImageSampler::ImageSampler(pugi::xml_node node)
    {
        auto inter = std::string(node.child("Interpolation").text().as_string());
        if(inter.compare("nearest") == 0)
        {
            Interpolation = Interpolation::NEAREST;
        }
        else if(inter.compare("bilinear") == 0)
        {
            Interpolation = Interpolation::BILINEAR;
        }
        Normalizer = node.child("Normalizer").text().as_float(255);
        int id = node.child("ImageId").text().as_int();
        Image = ResourceLocator::GetInstance().GetImage(id);
    }

    Vector3f ImageSampler::Sample(SamplerData& data)
    {
        float u = data.u;
        float v = data.v;
        u = u - std::floor(u);
        v = v - std::floor(v);
        if(Interpolation == Interpolation::NEAREST)
        {   
            int x = (int)(u * (Image->Width - 1));
            int y = (int)(v * (Image->Height - 1));
            return Image->Fetch(x, y) / Normalizer;
        }
        else if(Interpolation == Interpolation::BILINEAR)
        {
            int p = (int)(u * (Image->Width - 1));
            int q = (int)(v * (Image->Height - 1));
            float dx = (u * (Image->Width - 1)) - p;
            float dy = (v * (Image->Height - 1)) - q;
            auto p00 = Image->Fetch(p,q);
            auto p01 = Image->Fetch(p, q + 1);
            auto p10 = Image->Fetch(p + 1, q);
            auto p11 = Image->Fetch(p + 1, q + 1);
            auto ret = p00 * (1 - dx) * (1 - dy) + p10 * (dx) * (1 - dy) + p01 * (1 - dx) * dy + p11 * dx * dy;
            return ret / Normalizer;
        }
        return Vector3f::Zero();
    }

    Vector3f ImageSampler::SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& n, float f)
    {
        float u = data.u;
        float v = data.v;
        u = u - std::floor(u);
        v = v - std::floor(v);
        int x = (int)(u * (Image->Width - 1));
        int y = (int)(v * (Image->Height - 1));
        float dxr = (Image->Fetch(x + 1, y).x() - Image->Fetch(x, y).x());
        float dxg = (Image->Fetch(x + 1, y).y() - Image->Fetch(x, y).y());
        float dxb = (Image->Fetch(x + 1, y).z() - Image->Fetch(x, y).z());

        float dyr = (Image->Fetch(x, y + 1).x() - Image->Fetch(x, y).x());
        float dyg = (Image->Fetch(x, y + 1).y() - Image->Fetch(x, y).y());
        float dyb = (Image->Fetch(x, y + 1).z() - Image->Fetch(x, y).z());

        float dx = (dxr + dxg + dxb) / 3;
        float dy = (dyr + dyg + dyb) / 3;
        return (n - f * (t * dx + b * dy)).normalized();
    }

    PerlinSampler::PerlinSampler(pugi::xml_node node)
    {
        auto conv = std::string(node.child("NoiseConversion").text().as_string());
        if(conv.compare("linear") == 0)
        {
            Conversion = NoiseConversion::LINEAR;
        }
        else if(conv.compare("absval") == 0)
        {
            Conversion = NoiseConversion::ABSVAL;
        }
        NoiseScale = node.child("NoiseScale").text().as_float(1);
        unsigned seed = 60;
        std::mt19937 g(seed);
        std::uniform_real_distribution<float> dist;
        auto rnd = std::bind(dist, g);

        // fill ptable 
        for(int i = 0; i < _tableSize; i++)
            _ptable.push_back(i);        
        std::shuffle(_ptable.begin(), _ptable.end(), g);
        for(int i = 0; i < _tableSize; i++)
            _ptable.push_back(_ptable[i]);
        // generate grads
        _grad.resize(_tableSize);
        for(int i = 0; i < _tableSize; i++)
        {
            _grad[i] = Vector3f(2 * rnd() - 1, 2 * rnd() - 1, 2 * rnd() - 1); 
        }
    }

    Vector3f PerlinSampler::Sample(SamplerData& data)
    {
        auto p = data.point * NoiseScale;
        int x0 = (int)std::floor(p.x()) & (_tableSize - 1);
        int y0 = (int)std::floor(p.y()) & (_tableSize - 1);
        int z0 = (int)std::floor(p.z()) & (_tableSize - 1);
        int x1 = (x0 + 1);
        int y1 = (y0 + 1);
        int z1 = (z0 + 1);

        auto g000 = _grad[hash(x0, y0, z0)];
        auto g001 = _grad[hash(x0, y0, z1)];
        auto g010 = _grad[hash(x0, y1, z0)];
        auto g011 = _grad[hash(x0, y1, z1)];
        auto g100 = _grad[hash(x1, y0, z0)];
        auto g101 = _grad[hash(x1, y0, z1)];
        auto g110 = _grad[hash(x1, y1, z0)];
        auto g111 = _grad[hash(x1, y1, z1)];

        float dx = p.x() - std::floor(p.x());
        float dy = p.y() - std::floor(p.y());
        float dz = p.z() - std::floor(p.z());

        float u = f(dx);
        float v = f(dy);
        float w = f(dz);

        auto v000 = Vector3f(dx, dy, dz);
        auto v001 = Vector3f(dx, dy, dz - 1);
        auto v010 = Vector3f(dx, dy - 1, dz);
        auto v011 = Vector3f(dx, dy - 1, dz - 1);
        auto v100 = Vector3f(dx - 1, dy, dz);
        auto v101 = Vector3f(dx - 1, dy, dz - 1);
        auto v110 = Vector3f(dx - 1, dy - 1, dz);
        auto v111 = Vector3f(dx - 1, dy - 1, dz - 1);

        float d000 = g000.dot(v000);
        float d001 = g001.dot(v001);
        float d010 = g010.dot(v010);
        float d011 = g011.dot(v011);
        float d100 = g100.dot(v100);
        float d101 = g101.dot(v101);
        float d110 = g110.dot(v110);
        float d111 = g111.dot(v111);

        float sum = 0;
        float sx1 = lerp(d000, d100, u);
        float sx2 = lerp(d010, d110, u);
        float sy1 = lerp(sx1, sx2, v);
        sx1 = lerp(d001, d101, u);
        sx2 = lerp(d011, d111, u);
        float sy2 = lerp (sx1, sx2, v);
        sum = lerp(sy1, sy2, w);

        if(Conversion == NoiseConversion::LINEAR)
        {
            sum = (sum + 1) / 2;
        }
        else if(Conversion == NoiseConversion::ABSVAL)
        {
            sum = std::fabs(sum);
        }
        return Vector3f(sum, sum, sum);
    }

    Vector3f PerlinSampler::SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& np, float f)
    {
        auto n = data.normal;
        float eps = 0.001f;
        float c = Sample(data).x();
        data.point.x() += eps;
        float dx = (Sample(data).x() - c) / eps;
        data.point.x() -= eps;
        data.point.y() += eps;
        float dy = (Sample(data).y() - c) / eps;
        data.point.y() -= eps;
        data.point.z() += eps;
        float dz = (Sample(data).z() - c) / eps;
        data.point.z() -= eps;
    
        auto grad = Vector3f(dx, dy, dz);
        auto gp = grad.dot(n) * n;
        auto go = grad - gp;
        return n - f * go; 
    }

    CheckerBoardSampler::CheckerBoardSampler(pugi::xml_node node)
    {
        BlackColor = Vec3fFrom(node.child("BlackColor"));
        WhiteColor = Vec3fFrom(node.child("WhiteColor"));
        Scale = node.child("Scale").text().as_float();
        Offset = node.child("Offset").text().as_float();
    }

    Vector3f CheckerBoardSampler::Sample(SamplerData& data)
    {
        auto p = data.point * Scale + Vector3f(Offset, Offset, Offset);
        if(((int)std::floor(p.x()) + (int)std::floor(p.y()) + (int)std::floor(p.z())) % 2 == 0)
            return BlackColor;
        return WhiteColor;
    }

    VoronoiSampler::VoronoiSampler(pugi::xml_node node)
    {
        Size = node.child("Size").text().as_float(1);
    }

    Vector3f VoronoiSampler::Sample(SamplerData& data)
    {
        auto p = data.point * Size;
        float cx = std::floor(p.x());   
        float cy = std::floor(p.y());
        float cz = std::floor(p.z());
        float min = 10;
        for(int i = -1; i <= 1; i++)
        {
            for(int j = -1; j <= 1; j++)
            {
                for(int k = -1; k <= 1; k++)
                {
                    Vector3f cell = Vector3f(cx + i, cy + j, cz + k);
                    Vector3f cellPosition = cell + random3to3(cell);
                    Vector3f toCell = cellPosition - p;
                    float distToCell = toCell.norm();
                    if(distToCell < min){
                        min = distToCell;
                    }                    
                }
            }
        }
        return Vector3f(min, min, min);
    }

    Vector3f VoronoiSampler::SampleBump(SamplerData& data, Vector3f& t, Vector3f& b, Vector3f& np, float f)
    {
        auto n = data.normal;
        float eps = 0.001f;
        float c = Sample(data).x();
        data.point.x() += eps;
        float dx = (Sample(data).x() - c) / eps;
        data.point.x() -= eps;
        data.point.y() += eps;
        float dy = (Sample(data).y() - c) / eps;
        data.point.y() -= eps;
        data.point.z() += eps;
        float dz = (Sample(data).z() - c) / eps;
        data.point.z() -= eps;
    
        auto grad = Vector3f(dx, dy, dz);
        auto gp = grad.dot(n) * n;
        auto go = grad - gp;
        return n - f * go; 
    }

    Texture::Texture(pugi::xml_node node)
    {
        _type = std::string(node.attribute("type").as_string());
        if(_type.compare("perlin") == 0)
        {
            Sampler = new PerlinSampler(node);
        }
        else if(_type.compare("checkerboard") == 0)
        {
            Sampler = new CheckerBoardSampler(node);
        }
        else if(_type.compare("voronoi") == 0)
        {
            Sampler = new VoronoiSampler(node);
        }
        else if(_type.compare("image") == 0)
        {
            Sampler = new ImageSampler(node);
        }
    }

    BackgroundTexture::BackgroundTexture(pugi::xml_node node) : Texture(node)
    {

    }

    Vector3f BackgroundTexture::Sample(SamplerData& data)
    {
        return Sampler->Sample(data);
    }

    DiffuseTexture::DiffuseTexture(pugi::xml_node node) : Texture(node)
    {
        auto decal = std::string(node.child("DecalMode").text().as_string());
        if(decal.compare("replace_kd") == 0)
        {
            Mode = DecalMode::REPLACE_KD;            
        }
        else if(decal.compare("blend_kd") == 0)
        {
            Mode = DecalMode::BLEND_KD;            
        }
        else if(decal.compare("replace_all") == 0)
        {
            Mode = DecalMode::REPLACE_ALL;            
        }
    }

    Vector3f DiffuseTexture::Color(SamplerData& data)
    {
        return Sampler->Sample(data);
    }

    NormalTexture::NormalTexture(pugi::xml_node node) : Texture(node)
    {

    }

    Vector3f NormalTexture::SampleNormal(SamplerData data)
    {
        Vector3f n = Sampler->Sample(data);
        n = n - Vector3f(0.5, 0.5, 0.5);
        return n.normalized();
    }

    BumpTexture::BumpTexture(pugi::xml_node node) : Texture(node)
    {
        Factor = node.child("BumpFactor").text().as_float(1);        
    }

    Vector3f BumpTexture::SampleBump(SamplerData& data, Matrix<float, 3, 3>& tbn)
    {
        Vector3f t = tbn.col(0).transpose();
        Vector3f b = tbn.col(1).transpose();
        Vector3f np = tbn.col(2).transpose();
        auto n = t.cross(b);
        if(n.dot(np) < 0)
            n *= -1;
        return Sampler->SampleBump(data, t, b, n, Factor);
    }


    void WriteEXR(std::vector<Vector3f> pixels, const int width, const int height,
                   const char *filename)
    {
        EXRHeader header;
        EXRImage image;

        InitEXRHeader(&header);
        InitEXRImage(&image);

        image.num_channels = 3;

        std::vector<float> images[3];
        images[0].resize(pixels.size());
        images[1].resize(pixels.size());
        images[2].resize(pixels.size());

        for(int i = 0; i < width * height; i++)
        {
            images[0][i] = pixels[i].x();
            images[1][i] = pixels[i].y();
            images[2][i] = pixels[i].z();
        }

        float* image_ptr[3];
        image_ptr[0] = &(images[2].at(0)); // B
        image_ptr[1] = &(images[1].at(0)); // G
        image_ptr[2] = &(images[0].at(0)); // R

        image.images = (unsigned char**)image_ptr;
        image.width = width;
        image.height = height;

        header.num_channels = 3;
        header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels); 
        // Must be BGR(A) order, since most of EXR viewers expect this channel order.
        strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
        strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
        strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

        header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels); 
        header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
        for(int i = 0; i < header.num_channels; i++)
        {
            // pixel type of input image
                header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
            // pixel type of output image to be stored in .EXR
                header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; 
        }

        const char* err;
        int ret = SaveEXRImageToFile(&image, &header, filename, &err);
        if(ret != TINYEXR_SUCCESS)
        {
            fprintf(stderr, "Save EXR err: %s\n", err);
        }
        //printf("Saved exr file. [ %s ] \n", fname.c_str());

        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
        }
}