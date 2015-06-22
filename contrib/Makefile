SRC = kinect2sdl.c
OBJ = $(patsubst %.c,%.o,$(SRC))

CFLAGS = -g -I/usr/include/SDL -I/usr/include/libfreenect -std=gnu99 -O2 -Wall
LDFLAGS = -lSDL -lgd -lfreenect

.PHONY : all clean

all: kinect2sdl

kinect2sdl: $(OBJ)
	gcc -o $@ $^ $(LDFLAGS)

%.o : %.c
	gcc -c $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) v4l2sdl
