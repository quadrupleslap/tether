# Cocoa (macOS)

tether-cocoa.a: tether-cocoa.o
	ar rcs $@ $^

tether-cocoa.o: cocoa.m
	clang -c -ObjC -fobjc-arc -Wall -Wextra -o $@ $^

# WinAPI (Windows)

tether-winapi.lib: tether-winapi.obj
	lib /out:$@ $^

tether-winapi.obj: winapi.cpp
	cl /c /EHsc /std:c++17 /await /Fo$@ $^

# GTK (Linux)

tether-gtk.a: tether-gtk.o
	ar rcs $@ $^

tether-gtk.o: gtk.c
	gcc -c -Wall -Wextra `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -o $@ $^

# Nuking Things

clean:
	rm -f *.o *.a *.lib *.obj
