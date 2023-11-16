#CC=clang++
#CFLAGS=-Wall -g -fsanitize=address -fno-omit-frame-pointer
CC=g++
CFLAGS=-Wall -g -I/usr/include/libxml2/
(ODIR)/%.o: %.cpp
ODIR=obj

CPP_FILES := $(wildcard *.cpp)
OBJ:= $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
#LIBS=-fsanitize=address -fno-omit-frame-pointer
LIBS= -lxml2

$(ODIR)/%.o: %.cpp 
	$(CC) -c -o $@ $< $(CFLAGS)

all: tally_converter
 
tally_converter: $(OBJ) $(OBJLIBS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

clean:
	rm -f $(ODIR)/*.o tally_converter 
