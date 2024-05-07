STD = -std=c++11
INPUT = input.bmp
FLAG = -lpthread
ImageFilters: ImageFilters.out
	echo "ImageFilters created"
	# ./ImageFilters.out

ImageFilters.out: main.cpp
	g++ $(STD) -o ImageFilters.out main.cpp

clean:
	rm -f *.out	

run:
	g++ $(STD) main.cpp $(FLAG) -o ImageFilters.out && ./ImageFilters.out $(INPUT)