all:
	mkdir -p bin
	clang++ -O3 main.cc -o bin/search -I rapidjson/include -Wall

debug:
	mkdir -p bin
	clang++ -O3 -g -fsanitize=address -fno-omit-frame-pointer main.cc -o bin/search -I rapidjson/include -Wall