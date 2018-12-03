#include <stdio.h>
#include <string.h>
#include <tether.h>

tether window;

void drop(void *ctx) {
    printf("%s dropped\n", (const char *)ctx);
}

void closed(void *ctx) {
    (void)ctx;
    printf("window closed\n");
}

void message(void *ctx, const char *data) {
    (void)ctx;
    printf("%s received\n", data);

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
            .message = (tether_fn) {
                .call = message,
                .data = "popup/message",
                .drop = drop
            },
            .closed = (tether_fn) {
                .call = closed,
                .data = "popup/closed",
                .drop = drop
            }
        });

        tether_load(popup, "get a WINDOW so complicated");
        tether_drop(popup);
    }
}

void start(void *ctx) {
    (void)ctx;

    window = tether_new((tether_options) {
        .initial_width = 800,
        .initial_height = 600,
        .minimum_width = 0,
        .minimum_height = 0,
        .borderless = false,
        .debug = true,
        .message = (tether_fn) {
            .call = message,
            .data = "main/message",
            .drop = drop
        },
        .closed = (tether_fn) {
            .call = closed,
            .data = "main/closed",
            .drop = drop
        }
    });

    tether_load(window, "\
        <title>Hello</title>\
        <script defer>window.tether('ready')</script>\
        Hello, <b>world</b>! It's your boy, <span id=name></span>!\
        <button onclick=\"window.tether('popup')\">Popup</button>\
    ");
}

int main(void) {
    tether_start((tether_fn) {
        .call = start,
        .data = "start",
        .drop = drop,
    });

    return 0;
}
