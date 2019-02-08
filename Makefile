CXXFLAGS=-m32 -std=c++11 -c

main: threads.o

threads.o: thread_manager.cpp thread_manager.h
	$(CXX) $(CXXFLAGS) ./thread_manager.cpp -o ./threads.o
clean:
	rm -f *.o
