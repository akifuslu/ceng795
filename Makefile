all:
	clang++ *.cpp -o raytracer -std=c++17 -Ofast -flto -ljpeg
