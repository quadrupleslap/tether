# macOS

tether-cocoa.a: tether-cocoa.o
	ar rcs $@ $^

tether-cocoa.o:
	clang -c -ObjC -fobjc-arc -Wall -Wextra -DTETHER_COCOA tether.c -o $@

# Windows

tether-windows.lib:
	echo 'TODO'

# Linux

tether-gtk.a: tether-gtk.o
	ar rcs $@ $^

tether-gtk.o:
	gcc -c -Wall -Wextra `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` tether.c -o $@

# Nuking Things

clean:
	rm -f *.o *.a *.lib
