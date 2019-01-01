#include <tether.h>

void message(void *data, const char *message) {}
void closed(void *data) {}

void start(void) {
    tether window = tether_new((tether_options) {
        .initial_width = 640,
        .initial_height = 480,
        .minimum_width = 160,
        .minimum_height = 120,
        .borderless = false,
        .debug = true,
        .data = NULL,
        .message = message,
        .closed = closed
    });

    tether_title(window, "Stuff");
    tether_load(
        window,
        "<button onclick='requestFullscreen(latin)'>Test Fullscreen</button>"
        "<p id=latin style='background:#0F0'>Caecilius est in horto.</p>"
        "<script>"
        "function requestFullscreen(element) {"
        "    if (element.requestFullscreen) {"
        "        element.requestFullscreen();"
        "    } else if (element.mozRequestFullScreen) {"
        "        element.mozRequestFullScreen();"
        "    } else if (element.webkitRequestFullscreen) {"
        "        element.webkitRequestFullscreen();"
        "    } else if (element.msRequestFullscreen) {"
        "        element.msRequestFullscreen();"
        "    }"
        "}"
        "</script>"
    );
}

int main(void) {
    tether_start(start);
    return 0;
}
