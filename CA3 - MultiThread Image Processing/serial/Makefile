STD = -std=c++11
INPUT = input.bmp
FLAG = 
ImageFilters: ImageFilters.out
	echo "ImageFilters created"
	# ./ImageFilters.out

ImageFilters.out: main.cpp
	g++ $(STD) -o ImageFilters.out main.cpp

clean:
	rm -f *.out	

run:
	g++ -std=c++11 main.cpp -o ImageFilters.out && ./ImageFilters.out $(INPUT)