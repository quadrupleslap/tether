# Cocoa (macOS)

tether-cocoa.a: tether-cocoa.o
	ar rcs $@ $^

tether-cocoa.o: cocoa.m
	clang -c -ObjC -fobjc-arc -Wall -Wextra -o $@ $^

# WinAPI (Windows)

tether-winapi.lib:
	echo 'TODO'

# GTK (Linux)

tether-gtk.a: tether-gtk.o
	ar rcs $@ $^

tether-gtk.o: gtk.c
	gcc -c -Wall -Wextra `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -o $@ $^

# Nuking Things

clean:
	rm -f *.o *.a *.lib
