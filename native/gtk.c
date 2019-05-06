#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "tether.h"

// ===============
// RANDOM NONSENSE
// ===============

struct _tether {
    GtkWindow *window;
    WebKitWebView *webview;
    void *data;
    void (*closed)(void *data);
};

struct dispatch {
    void *data;
    void (*func)(void *data);
};

struct handler {
    void *data;
    void (*func)(void *data, const char *message);
};

static int dispatched(void *ctx) {
    struct dispatch *dispatch = (struct dispatch *)ctx;
    dispatch->func(dispatch->data);
    return G_SOURCE_REMOVE;
}

static void message_received(WebKitUserContentManager *manager, WebKitJavascriptResult *result, void *ctx) {
    (void)manager;
    struct handler *handler = (struct handler *)ctx;
    JSCValue *val = webkit_javascript_result_get_js_value(result);
    char *message = jsc_value_to_string(val);
    handler->func(handler->data, message);
    g_free(message);
}

static void handler_free(void *ctx, GClosure *closure) {
    (void)closure;
    free(ctx);
}

static void window_destroyed(GtkWidget* widget, void *ctx) {
    (void)widget;
    tether self = (tether)ctx;
    self->closed(self->data);
    free(self);
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

void tether_start(void (*func)(void)) {
    gtk_init(0, NULL);
    func();
    gtk_main();
}

void tether_dispatch(void *data, void (*func)(void *data)) {
    struct dispatch *dispatch = malloc(sizeof *dispatch);
    dispatch->data = data;
    dispatch->func = func;

    g_idle_add_full(
        G_PRIORITY_HIGH_IDLE,
        dispatched,
        dispatch,
        free
    );
}

void tether_exit(void) {
    gtk_main_quit();
}

tether tether_new(tether_options opts) {
    tether self = malloc(sizeof *self);
    self->data = opts.data;
    self->closed = opts.closed;

    // Create the window.
    GtkWindow *window = self->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_default_size(window, opts.initial_width, opts.initial_height);
    gtk_widget_set_size_request(GTK_WIDGET(window), opts.minimum_width, opts.minimum_height);
    if (opts.borderless) gtk_window_set_decorated(window, FALSE);
    gtk_window_set_position(window, GTK_WIN_POS_CENTER);

    // Free resources and notify the user when the window's been closed.
    g_signal_connect_data(
        window,
        "destroy",
        G_CALLBACK(window_destroyed),
        self,
        NULL,
        0
    );

    // Create the web view.
    WebKitWebView *webview = self->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitSettings *settings = webkit_web_view_get_settings(webview);
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(webview);
    if (opts.debug) webkit_settings_set_enable_developer_extras(settings, TRUE);

    // Listen for messages.
    struct handler *handler = malloc(sizeof *handler);
    handler->data = opts.data;
    handler->func = opts.message;

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
        handler,
        handler_free,
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

    return self;
}

void tether_eval(tether self, const char *js) {
    webkit_web_view_run_javascript(self->webview, js, NULL, NULL, NULL);
}

void tether_load(tether self, const char *html) {
    webkit_web_view_load_html(self->webview, html, NULL);
}

void tether_title(tether self, const char *title) {
    gtk_window_set_title(self->window, title);
}

void tether_focus(tether self) {
    gtk_window_present(self->window);
}

void tether_close(tether self) {
    gtk_window_close(self->window);
}
