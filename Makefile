SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:%.cpp=%.o)

a.out: $(OBJS)
	@echo '$@'
	@ g++ $(OBJS) -o a.out

%.o: %.cpp
	@echo '$@'
	@ g++ -std=c++11 -O3 -c $< -o $@

clean:
	@echo '$@'
	@ rm -fv $(OBJS)
