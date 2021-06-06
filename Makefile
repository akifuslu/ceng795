all:
	clang++ *.cpp -o raytracer -std=c++11 -Ofast -flto -ljpeg