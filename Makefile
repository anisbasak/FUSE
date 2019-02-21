all: anisbasak.c
		gcc -Wall -o anisbasak anisbasak.c `pkg-config fuse --cflags --libs` -lzip -g