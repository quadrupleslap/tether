// This library supports multiple platforms.
//
// - Webkit2GTK is the default.
// - Win32 can be enabled with the `TETHER_WIN32` flag.
// - macOS can be enabled with the `TETHER_MACOS` flag.

#include <stdbool.h>
#include <stddef.h>

typedef struct tether *tether;

typedef struct tether_fn {
    void *call, *data;
    void (*drop)(void *data);
} tether_fn;

typedef struct tether_options {
    size_t initial_width,
           initial_height,
           minimum_width,
           minimum_height;
    bool borderless,
         debug;
    tether_fn handler; // (const char *) -> ()
} tether_options;

void tether_start(tether_fn cb); // (() -> ()) -> ()
void tether_dispatch(tether_fn cb); // (() -> ()) -> ()
void tether_exit(void);

tether tether_new(tether_options opts);
tether tether_clone(tether self);
void tether_drop(tether self);

void tether_eval(tether self, const char *js);
void tether_load(tether self, const char *html);
void tether_close(tether self);
