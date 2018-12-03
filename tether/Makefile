# macOS

tether-cocoa.a:
	echo 'TODO'

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
