#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include "tether.h"

@interface RKWindow : NSWindow
@end

@implementation RKWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

@interface RKHandler : NSObject <WKScriptMessageHandler>
@end

@implementation RKHandler {
    tether_fn handler;
}

- (id)initWithHandler:(tether_fn)fun {
    handler = fun;
    return self;
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
    (void)userContentController;

    id body = [message body];
    if (![body isKindOfClass:[NSString class]]) return;
    ((void (*)(void *data, const char *ptr))handler.call)(handler.data, [body UTF8String]);
}

- (void)observeValueForKeyPath:(NSString *)keyPath 
                      ofObject:(id)object 
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change 
                       context:(void *)context {
    (void)keyPath;
    (void)change;
    (void)context;

    [[object window] setTitle:[object title]];
}

- (void)dealloc {
    //TODO: This doesn't run, and I have NO idea why.
    handler.drop(handler.data);
}
@end

struct tether {
    CFTypeRef window;
    CFTypeRef webview;
};

void tether_start(tether_fn cb) {
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

    ((void (*)(void *data))cb.call)(cb.data);
    cb.drop(cb.data);

    [app activateIgnoringOtherApps:YES];
    [app run];
}

void tether_dispatch(tether_fn cb) {
    dispatch_async(dispatch_get_main_queue(), ^{
        ((void (*)(void *data))cb.call)(cb.data);
        cb.drop(cb.data);
    });
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

    // Create and attach the handler.
    RKHandler *handler = [[RKHandler alloc] initWithHandler:opts.handler];
    [manager addScriptMessageHandler:handler name:@"__tether"];
    [webview addObserver:handler forKeyPath:@"title" options:0 context:nil];
    //TODO: We want to be more granular with the context menus, but WebKit doesn't support that yet.
    [manager addUserScript:[[WKUserScript alloc] initWithSource:@"window.tether = function (s) { window.webkit.messageHandlers.__tether.postMessage(s); };\
                                                                  document.addEventListener('contextmenu', function (e) { e.preventDefault(); return false; });"
                                                  injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                               forMainFrameOnly:YES]];

    // Attach the web view to the window.
    [window setContentView:webview];

    // Show the window.
    [window makeKeyAndOrderFront:nil];

    // Finish up.
    self->window = (__bridge_retained void *)window;
    self->webview = (__bridge_retained void *)webview;
    return self;
}

tether tether_clone(tether self) {
    tether new = malloc(sizeof *new);
    assert(new);
    new->window = CFRetain(self->window);
    new->webview = CFRetain(self->webview);
    return new;
}

void tether_drop(tether self) {
    CFRelease(self->window);
    CFRelease(self->webview);
    free(self);
}

void tether_eval(tether self, const char *js) {
    WKWebView *wv = (__bridge WKWebView *)self->webview;
    [wv evaluateJavaScript:[NSString stringWithUTF8String:js] completionHandler:nil];
}

void tether_load(tether self, const char *html) {
    WKWebView *wv = (__bridge WKWebView *)self->webview;
    [wv loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void tether_close(tether self) {
    NSWindow *w = (__bridge NSWindow *)self->window;
    [w close];
}
