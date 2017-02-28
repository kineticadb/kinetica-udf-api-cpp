all: proc-example

proc-example: makefile proc-example.cpp ../kinetica/Proc.cpp ../kinetica/Proc.hpp
	g++ -o proc-example proc-example.cpp ../kinetica/Proc.cpp -I.. -Wall -m64
