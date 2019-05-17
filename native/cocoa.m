#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include "tether.h"

struct _tether {
    CFTypeRef window;
    CFTypeRef webview;
};

@interface RKWindow : NSWindow
@end

@implementation RKWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

@interface RKDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
@end

@implementation RKDelegate {
    tether_options opts;
    tether handle;
}

- (id)initWithOptions:(tether_options)x
               tether:(tether)y {
    opts = x;
    handle = y;
    return self;
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)scriptMessage {
    (void)userContentController;

    if (!handle) return;

    id body = [scriptMessage body];
    if (![body isKindOfClass:[NSString class]]) return;
    opts.message(opts.data, [body UTF8String]);
}

- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;

    if (!handle) return;
    tether x = handle;
    handle = NULL;

    WKWebView *wv = (__bridge WKWebView *)x->webview;
    WKUserContentController *ucc = [[wv configuration] userContentController];
    [ucc removeScriptMessageHandlerForName:@"__tether"];

    opts.closed(opts.data);
    free(x);
}
@end

void tether_start(void (*func)(void)) {
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    // MAIN MENU
    NSProcessInfo *process = [NSProcessInfo processInfo];
    NSString *appname = [process processName];
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"MainMenu"],
           *submenu;
    NSMenuItem *item_app = [menu addItemWithTitle:appname action:nil keyEquivalent:@""],
               *item_edit = [menu addItemWithTitle:@"Edit" action:nil keyEquivalent:@""],
               *item_window = [menu addItemWithTitle:@"Window" action:nil keyEquivalent:@""],
               *item;
    // APP MENU
    submenu = [[NSMenu alloc] initWithTitle:appname];
    item = [submenu addItemWithTitle:[@"Hide " stringByAppendingString:appname]
                              action:@selector(hide:)
                       keyEquivalent:@"h"];
    item = [submenu addItemWithTitle:@"Hide Others"
                              action:@selector(hideOtherApplications:)
                       keyEquivalent:@"h"];
    [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
    item = [submenu addItemWithTitle:@"Show All"
                              action:@selector(unhideAllApplications:)
                       keyEquivalent:@""];
    [submenu addItem:[NSMenuItem separatorItem]];
    item = [submenu addItemWithTitle:[@"Quit " stringByAppendingString:appname]
                              action:@selector(terminate:)
                       keyEquivalent:@"q"];
    [menu setSubmenu:submenu forItem:item_app];
    // EDIT MENU
    submenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    item = [submenu addItemWithTitle:@"Undo"
                              action:@selector(undo:)
                       keyEquivalent:@"z"];
    item = [submenu addItemWithTitle:@"Redo"
                              action:@selector(undo:)
                       keyEquivalent:@"z"];
    [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagShift];
    [submenu addItem:[NSMenuItem separatorItem]];
    item = [submenu addItemWithTitle:@"Cut"
                              action:@selector(cut:)
                       keyEquivalent:@"x"];
    item = [submenu addItemWithTitle:@"Copy"
                              action:@selector(copy:)
                       keyEquivalent:@"c"];
    item = [submenu addItemWithTitle:@"Paste"
                              action:@selector(paste:)
                       keyEquivalent:@"v"];
    item = [submenu addItemWithTitle:@"Delete"
                              action:@selector(delete:)
                       keyEquivalent:@""];
    item = [submenu addItemWithTitle:@"Select All"
                              action:@selector(selectAll:)
                       keyEquivalent:@"a"];
    [menu setSubmenu:submenu forItem:item_edit];
    // WINDOW MENU
    submenu = [[NSMenu alloc] initWithTitle:@"Window"];
    item = [submenu addItemWithTitle:@"Close"
                              action:@selector(close)
                       keyEquivalent:@"w"];
    item = [submenu addItemWithTitle:@"Minimize"
                              action:@selector(miniaturize:)
                       keyEquivalent:@"m"];
    item = [submenu addItemWithTitle:@"Toggle Full Screen"
                              action:@selector(toggleFullScreen:)
                       keyEquivalent:@"f"];
    [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagControl];
    [menu setSubmenu:submenu forItem:item_window];
    [app setWindowsMenu:submenu];
    // MAIN MENU
    [app setMainMenu:menu];

    func();
    [app activateIgnoringOtherApps:YES];
    [app run];
}

void tether_dispatch(void *data, void (*func)(void *data)) {
    dispatch_async(dispatch_get_main_queue(), ^{ func(data); });
}

void tether_exit(void) {
    [NSApp terminate:nil];
}

tether tether_new(tether_options opts) {
    tether self = malloc(sizeof *self);
    assert(self);

    // Create the window.
    NSRect initial_size = NSMakeRect(0, 0, opts.initial_width, opts.initial_height);
    NSWindowStyleMask style = 0;
    if (!opts.borderless) style |= NSWindowStyleMaskTitled
                                |  NSWindowStyleMaskClosable
                                |  NSWindowStyleMaskMiniaturizable
                                |  NSWindowStyleMaskResizable;
    NSWindow *window =
        [[RKWindow alloc] initWithContentRect:initial_size
                                    styleMask:style
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    [window setContentMinSize:NSMakeSize(opts.minimum_width, opts.minimum_height)];
    [window setReleasedWhenClosed:NO];
    [window center];

    // Create the web view.
    WKWebViewConfiguration *config = [WKWebViewConfiguration new];
    WKPreferences *prefs = [config preferences];
    WKUserContentController *manager = [config userContentController];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];
    if (opts.debug) [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    WKWebView *webview = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];

    // Create and attach the delegate.
    RKDelegate *delegate = [[RKDelegate alloc] initWithOptions:opts tether:self];
    [manager addScriptMessageHandler:delegate name:@"__tether"];
    [manager addUserScript:[[WKUserScript alloc] initWithSource:@"window.tether = function (s) { window.webkit.messageHandlers.__tether.postMessage(s); };"
                                                  injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                               forMainFrameOnly:YES]];
    if (!opts.debug) {
        //TODO: macOS currently doesn't show any context menu.
        [manager addUserScript:[[WKUserScript alloc] initWithSource:@"document.addEventListener('contextmenu', function (e) { e.preventDefault(); return false; });"
                                                      injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                   forMainFrameOnly:NO]];
    }
    [window setDelegate:delegate];

    // Show things.
    [window setContentView:webview];
    [window makeKeyAndOrderFront:nil];

    // Note that we don't use __bridge_retain, so these are essentially
    // extremely dangerous weak references.
    self->window = (__bridge void *)window;
    self->webview = (__bridge void *)webview;
    return self;
}

void tether_eval(tether self, const char *js) {
    WKWebView *wv = (__bridge WKWebView *)self->webview;
    [wv evaluateJavaScript:[NSString stringWithUTF8String:js] completionHandler:nil];
}

void tether_load(tether self, const char *html) {
    WKWebView *wv = (__bridge WKWebView *)self->webview;
    [wv loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void tether_title(tether self, const char *title) {
    NSWindow *w = (__bridge NSWindow *)self->window;
    [w setTitle:[NSString stringWithUTF8String:title]];
}

void tether_focus(tether self) {
    NSWindow *w = (__bridge NSWindow *)self->window;
    [w makeKeyAndOrderFront:nil];
}

void tether_close(tether self) {
    NSWindow *w = (__bridge NSWindow *)self->window;
    [w close];
}
