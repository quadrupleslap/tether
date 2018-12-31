#include <stdio.h>
#include <string.h>
#include <tether.h>

tether window;

void closed(void *ctx) {
    printf("%s closed\n", (char *)ctx);
    if (!strcmp((char *)ctx, "main")) tether_exit();
}

void message(void *ctx, const char *data) {
    printf("%s received %s\n", (char *)ctx, data);

    if (!strcmp(data, "ready")) {
        tether_eval(window, "document.getElementById('name').textContent = 'Guzma'");
    } else if (!strcmp(data, "popup")) {
        tether popup = tether_new((tether_options) {
            .initial_width = 200,
            .initial_height = 100,
            .minimum_width = 200,
            .minimum_height = 100,
            .borderless = false,
            .debug = false,
            .data = "popup",
            .message = message,
            .closed = closed
        });

        tether_title(popup, "Something");
        tether_load(popup, "get a WINDOW so complicated");
    }
}

void start(void) {
    window = tether_new((tether_options) {
        .initial_width = 800,
        .initial_height = 600,
        .minimum_width = 0,
        .minimum_height = 0,
        .borderless = false,
        .debug = true,
        .data = "main",
        .message = message,
        .closed = closed
    });

    tether_title(window, "The future is now!");
    tether_load(window, "\
        <script defer>window.tether('ready')</script>\
        Hello, <b>world</b>! It's your boy, <span id=name></span>!\
        <button onclick=\"window.tether('popup')\">Popup</button>\
    ");
}

int main(void) {
    tether_start(start);
    return 0;
}
