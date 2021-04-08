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
    doc.load_file(argv[1]);
    auto scene = Scene(doc.child("Scene"));
    scene.Load();
    auto start = std::chrono::high_resolution_clock::now();
    scene.Render(numThreads);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << duration.count() << " ms" << std::endl;
    return 0;
}