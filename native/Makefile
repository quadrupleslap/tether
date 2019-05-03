.PHONY: default clean

# Current Platform

ifeq ($(OS), Windows_NT)

tether: tether-winapi.lib
	copy tether-winapi.lib tether.lib

else ifeq ($(shell uname), Linux)

tether: tether-gtk.a
	cp tether-gtk.a libtether.a

else ifeq ($(shell uname), Darwin)

tether: tether-cocoa.a
	cp tether-cocoa.a libtether.a

endif

# macOS

tether-cocoa.a: tether-cocoa.o
	ar rcs $@ $^

tether-cocoa.o: cocoa.m
	clang -c -ObjC -fobjc-arc -Wall -Wextra -o $@ $^

# Windows

tether-winapi.lib: tether-winapi.obj
	lib /out:$@ $^

tether-winapi.obj: winapi.cpp
	clang-cl /c /EHsc /std:c++17 /W4 /WX /Fo$@ $^

# Linux

tether-gtk.a: tether-gtk.o
	ar rcs $@ $^

tether-gtk.o: gtk.c
	clang -c -Wall -Wextra `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -o $@ $^

# Nuking Things

clean:
	rm -f *.o *.a *.lib *.obj
