srcdir=./
builddir=./

CC=gcc
CFLAGS=-std=c99 -Wall

all:
	$(CC) -I. -I$(srcdir) -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include $(CFLAGS) `pkg-config --libs glib-2.0` $(srcdir)csv2json.c -o $(builddir)csv2json
