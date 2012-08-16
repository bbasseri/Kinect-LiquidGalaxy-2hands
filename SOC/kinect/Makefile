NAME= kinect

SOURCES := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SOURCES))
DEPS := $(wildcard include/*.h)

CXXFLAGS=-Iinclude -O3 -I/user/include/nite -I/usr/include/ni
LDFLAGS=-lOpenNI -lXnVNite_1_5_2

all: $(OBJS) $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(NAME)
