#include <assert.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "tether.h"

// ===============
// RANDOM NONSENSE
// ===============

struct _tether {
    GtkWindow *window;
    WebKitWebView *webview;
};

static int dispatch(void *ctx) {
    tether_fn cb = *(tether_fn *)ctx;
    ((void (*)(void *data))cb.call)(cb.data);
    return G_SOURCE_REMOVE;
}

static tether_fn *box(tether_fn fun) {
    tether_fn *boxed = malloc(sizeof *boxed);
    assert(boxed);
    *boxed = fun;
    return boxed;
}

static void starter_dropped(void *ctx) {
    tether_fn cb = *(tether_fn *)ctx;
    cb.drop(cb.data);
    free(ctx);
}

static void handler_dropped(void *ctx, GClosure *closure) {
    (void)closure;

    tether_fn cb = *(tether_fn *)ctx;
    cb.drop(cb.data);
    free(ctx);
}

static void title_changed(GObject *webview, GParamSpec *spec, void *ctx) {
    (void)spec;
    (void)ctx;

    const char *title = webkit_web_view_get_title(WEBKIT_WEB_VIEW(webview));
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(webview));
    if (GTK_IS_WINDOW(window)) gtk_window_set_title(GTK_WINDOW(window), title);
}

static void message_received(WebKitUserContentManager *manager, WebKitJavascriptResult *result, void *ctx) {
    (void)manager;

    tether_fn cb = *(tether_fn *)ctx;
    JSCValue *val = webkit_javascript_result_get_js_value(result);
    char *message = jsc_value_to_string(val);
    ((void (*)(void *data, const char *ptr))cb.call)(cb.data, message);
    g_free(message);
}

static void window_destroyed(GtkWidget* widget, void *ctx) {
    (void)widget;

    tether_fn cb = *(tether_fn *)ctx;
    ((void (*)(void *data))cb.call)(cb.data);
}

gboolean context_menu(WebKitWebView *wv, WebKitContextMenu *cm, GdkEvent *e, WebKitHitTestResult *htr, void *ctx) {
    (void)wv;
    (void)e;
    (void)htr;
    (void)ctx;

    guint i = webkit_context_menu_get_n_items(cm);
    while (i --> 0) {
        WebKitContextMenuItem *item = webkit_context_menu_get_item_at_position(cm, i);
        switch (webkit_context_menu_item_get_stock_action(item)) {
            case WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION:
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_TO_CLIPBOARD:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY_IMAGE_URL_TO_CLIPBOARD:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY:
            case WEBKIT_CONTEXT_MENU_ACTION_CUT:
            case WEBKIT_CONTEXT_MENU_ACTION_PASTE:
            case WEBKIT_CONTEXT_MENU_ACTION_DELETE:
            case WEBKIT_CONTEXT_MENU_ACTION_SELECT_ALL:
            case WEBKIT_CONTEXT_MENU_ACTION_INPUT_METHODS:
            case WEBKIT_CONTEXT_MENU_ACTION_UNICODE:
            case WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS:
            case WEBKIT_CONTEXT_MENU_ACTION_NO_GUESSES_FOUND:
            case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_SPELLING:
            case WEBKIT_CONTEXT_MENU_ACTION_LEARN_SPELLING:
            case WEBKIT_CONTEXT_MENU_ACTION_IGNORE_GRAMMAR:
            case WEBKIT_CONTEXT_MENU_ACTION_FONT_MENU:
            case WEBKIT_CONTEXT_MENU_ACTION_BOLD:
            case WEBKIT_CONTEXT_MENU_ACTION_ITALIC:
            case WEBKIT_CONTEXT_MENU_ACTION_UNDERLINE:
            case WEBKIT_CONTEXT_MENU_ACTION_OUTLINE:
            case WEBKIT_CONTEXT_MENU_ACTION_INSPECT_ELEMENT:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY_VIDEO_LINK_TO_CLIPBOARD:
            case WEBKIT_CONTEXT_MENU_ACTION_COPY_AUDIO_LINK_TO_CLIPBOARD:
            case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_CONTROLS:
            case WEBKIT_CONTEXT_MENU_ACTION_TOGGLE_MEDIA_LOOP:
            case WEBKIT_CONTEXT_MENU_ACTION_ENTER_VIDEO_FULLSCREEN:
            case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PLAY:
            case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_PAUSE:
            case WEBKIT_CONTEXT_MENU_ACTION_MEDIA_MUTE:
            case WEBKIT_CONTEXT_MENU_ACTION_CUSTOM:
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW:
            case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_LINK_TO_DISK:
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW:
            case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_IMAGE_TO_DISK:
            case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_VIDEO_TO_DISK:
            case WEBKIT_CONTEXT_MENU_ACTION_DOWNLOAD_AUDIO_TO_DISK:
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW:
            case WEBKIT_CONTEXT_MENU_ACTION_GO_BACK:
            case WEBKIT_CONTEXT_MENU_ACTION_GO_FORWARD:
            case WEBKIT_CONTEXT_MENU_ACTION_STOP:
            case WEBKIT_CONTEXT_MENU_ACTION_RELOAD:
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW:
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW:
                webkit_context_menu_remove(cm, item);
                break;
        }
    }

    return FALSE;
}

// ==============
// EXPORTED STUFF
// ==============

void tether_start(tether_fn cb) {
    gtk_init(0, NULL);
    ((void (*)(void *data))cb.call)(cb.data);
    cb.drop(cb.data);
    gtk_main();
}

void tether_dispatch(tether_fn cb) {
    g_idle_add_full(
        G_PRIORITY_HIGH_IDLE,
        dispatch,
        box(cb),
        starter_dropped
    );
}

void tether_exit(void) {
    gtk_main_quit();
}

tether tether_new(tether_options opts) {
    tether self = malloc(sizeof *self);
    assert(self);

    // Create the window.
    GtkWindow *window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_default_size(window, opts.initial_width, opts.initial_height);
    gtk_widget_set_size_request(GTK_WIDGET(window), opts.minimum_width, opts.minimum_height);
    gtk_window_set_decorated(window, FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Tell the user when the window's been closed.
    g_signal_connect_data(
        window,
        "destroy",
        G_CALLBACK(window_destroyed),
        box(opts.closed),
        handler_dropped,
        0
    );

    // Create the web view.
    WebKitWebView *webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitSettings *settings = webkit_web_view_get_settings(webview);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(webview);
    if (opts.debug) webkit_settings_set_enable_developer_extras(settings, TRUE);

    // Update the window's title to match the document's title.
    g_signal_connect(webview, "notify::title", G_CALLBACK(title_changed), NULL);

    // Listen for messages.
    WebKitUserScript *script = webkit_user_script_new(
        "window.tether = function (s) { window.webkit.messageHandlers.__tether.postMessage(s); }",
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL,
        NULL
    );
    g_signal_connect_data(
        manager,
        "script-message-received::__tether",
        G_CALLBACK(message_received),
        box(opts.message),
        handler_dropped,
        0
    );
    webkit_user_content_manager_register_script_message_handler(manager, "__tether");
    webkit_user_content_manager_add_script(manager, script);

    // Remove navigation items from the context menu.
    g_signal_connect(webview, "context-menu", G_CALLBACK(context_menu), NULL);

    // Attach the web view to the window.
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webview));
    gtk_widget_grab_focus(GTK_WIDGET(webview));

    // Show the window.
    gtk_widget_show_all(GTK_WIDGET(window));

    // Finish up.
    self->window = g_object_ref(window);
    self->webview = g_object_ref(webview);
    return self;
}

tether tether_clone(tether self) {
    tether new = malloc(sizeof *new);
    assert(new);
    new->window = g_object_ref(self->window);
    new->webview = g_object_ref(self->webview);
    return new;
}

void tether_drop(tether self) {
    g_object_unref(self->window);
    g_object_unref(self->webview);
    free(self);
}

void tether_eval(tether self, const char *js) {
    webkit_web_view_run_javascript(self->webview, js, NULL, NULL, NULL);
}

void tether_load(tether self, const char *html) {
    webkit_web_view_load_html(self->webview, html, NULL);
}

void tether_close(tether self) {
    gtk_window_close(self->window);
}
