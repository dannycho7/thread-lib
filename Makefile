APP_CXXFLAGS=-m32 -std=c++11 -g
CXXFLAGS=-m32 -std=c++11 -c

main: threads.o
app: thread_manager.hpp thread_manager.cpp app.cpp
	$(CXX) $(APP_CXXFLAGS) ./thread_manager.cpp ./app.cpp -o app.out
threads.o: thread_manager.cpp thread_manager.hpp
	$(CXX) $(CXXFLAGS) ./thread_manager.cpp -o ./threads.o
clean:
	rm -f *.o *.out
