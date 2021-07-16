all:
	clang++ *.cpp -o input/hw7/directLighting/raytracer -std=c++17 -O3 -flto -ljpeg
