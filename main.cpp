#include "pugixml.hpp"
#include "scene.h"
#include <iostream>
#include <chrono>
#include <stdlib.h>

using namespace raytracer;

int main(int argc, char *argv[])
{
    int numThreads = 1;
    if(argc > 2)
    {
        numThreads = std::atoi(argv[2]);
    }
    pugi::xml_document doc;
    auto start = std::chrono::high_resolution_clock::now();
    doc.load_file(argv[1]);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "pugiload: " << duration.count() << " ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    auto scene = Scene(doc.child("Scene"));
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "parsing" << duration.count() << " ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    scene.Load();
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "bvh " << duration.count() << " ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    scene.Render(numThreads);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "render" << duration.count() << " ms" << std::endl;
    return 0;
}