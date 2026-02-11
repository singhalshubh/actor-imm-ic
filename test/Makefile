TARGETS= clean main

all: $(TARGETS)

%: %.cpp
	g++ -g -O3 -std=c++17 -I$(TRNG_ROOT)/include -L$(TRNG_ROOT)/build/trng -o $@ $^ -ltrng4 -lm

clean:
	rm -f $(TARGETS) inf.txt


	
