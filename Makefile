
all: source.c
	gcc -Wall source.c `pkg-config fuse --cflags --libs` -lzip -g -o source
