all: handler

handler: handler.cpp; g++ -pthread handler.cpp -w -o handler.o

clean: ; rm -rf *.o result/*.txt; rm -rf console.txt; rm -rf error.txt; cp ../check2/* result/.
