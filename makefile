CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall

# Source files
SRCS := main.cpp \
        bvh_accelerator.cpp \
        BVHFlatten.cpp \
        bvhrecursivebuild.cpp \
        bvhsah.cpp

OBJS := $(SRCS:.cpp=.o)
TARGET := main.exe

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
