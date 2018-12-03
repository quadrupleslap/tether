#include <stdio.h>
#include <string.h>
#include <tether.h>

tether window;

void noop(void *ctx) {
    (void)ctx;
    printf("nop\n");
}

void message(void *ctx, const char *data) {
    (void)ctx;
    printf("msg  %s\n", data);

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
                .data = NULL,
                .drop = noop
            },
            .closed = (tether_fn) {
                .call = noop,
                .data = NULL,
                .drop = noop
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
            .data = NULL,
            .drop = noop
        },
        .closed = (tether_fn) {
            .call = noop,
            .data = NULL,
            .drop = noop
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
        .data = NULL,
        .drop = noop
    });

    return 0;
}
