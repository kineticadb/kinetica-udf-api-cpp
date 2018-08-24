# Ensure that the platform supports -m64
EXAMPLE_CXX_FLAGS := $(shell echo "int main() { return 0; }" | gcc -xc -m64 - >/dev/null 2>&1 && echo -n "-m64" || echo -n "")

all: proc-example

proc-example: makefile proc-example.cpp ../kinetica/Proc.cpp ../kinetica/Proc.hpp
	g++ -o proc-example proc-example.cpp ../kinetica/Proc.cpp -I.. -Wall $(EXAMPLE_CXX_FLAGS)
