.PHONY:cloud

cloud::cloud.cc
	g++ -o $@ $^ -std=c++17 -L./lib -lpthread -lstdc++fs -ljsoncpp -lbundle


.PHONY:clean
clean:
	rm -f clound