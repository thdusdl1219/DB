hw1: hw1.o record.o
	g++ -o hw1.exe record.o hw1.o -g

hw1.o : hw1.cpp hw1.h
	g++ -c hw1.cpp -g

record.o : record.cpp record.h
	g++ -c record.cpp -g

clean: 
	rm *.o
	rm *.exe
