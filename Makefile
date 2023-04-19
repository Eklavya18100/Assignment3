all: cache_simulate

cache_simulate: A3.cpp 
	g++ A3.cpp -o cache_simulate

clean:
	rm cache_simulate