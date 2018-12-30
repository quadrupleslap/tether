#include <stdbool.h>
#include <stddef.h>

//TODO: Thoroughly check all the platforms for memory leaks.
//TODO: Add keyboard shortcuts.
//TODO: Add context menus.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tether *tether;

typedef struct _tether_fn {
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
    tether_fn message, // (const char *) -> ()
              closed; // () -> ()
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

#ifdef __cplusplus
}
#endif
