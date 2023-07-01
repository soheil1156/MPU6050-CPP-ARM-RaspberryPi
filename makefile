CXX = g++
CXXFLAGS = -std=c++11 -I/usr/local/include 
LDFLAGS = 

TARGET = MPU6050
SRCS = main.cpp 
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)  $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)