TARGET	= cpp-box
DIR_INC	:= -I./ -I$(STAGING_DIR)/usr/include
CROSS_COMPILE = 
CXXFLAGS =	-g $(DIR_INC) -Os \
			-Wunused-function -Wunused-but-set-variable \
			-std=c++11 -D_GLIBCXX_USE_C99  -fPIC
			
			
LIBS = -lstdc++ -lm -lcchip -ldl -lrt -lasound -lpthread \
			-lavcodec -lavformat -lavutil -lswresample -lz -lsnowboy
LDFLAGS += -L./libs


OBJECTS_OUT	:= $(patsubst %.cpp,%.o,$(shell find ./snowBoy -name "*.cpp"))
OBJECTS_ALL := $(patsubst %.cpp,%.o,$(shell find -name "*.cpp"))
OBJECTS 	:= $(filter-out $(OBJECTS_OUT),$(OBJECTS_ALL))
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Building $@ ..."
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $< 
clean:
	rm -f *.o $(TARGET)

.PHONY:clean
