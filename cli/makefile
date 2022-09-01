src = $(wildcard *.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

target = client

$(target):$(obj)
	g++ $^ -o $@ -lpthread -ljsoncpp -lmysqlclient -levent

%.o:%.cpp
	g++ -c $^ -o $@

.PHONY:clean
clean:
	-rm -rf *.o client