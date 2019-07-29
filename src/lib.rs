#![deny(missing_docs)]

//! Windows that are web views.

use log::error;
use std::cell::{Cell, RefCell};
use std::ffi::{c_void, CStr, CString};
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::{panic, process};

thread_local! {
    static MAIN_THREAD: Cell<bool> = Cell::new(false);
}

static INITIALIZED: AtomicBool = AtomicBool::new(false);

/// An event handler; you probably want to implement one.
///
/// - When the webpage calls `window.tether`, the message is passed to `handle`.
/// - The handler is dropped when the window is closed.
pub trait Handler: 'static {
    /// The webpage called `window.tether` with the given string.
    fn handle(&mut self, window: Window, message: &str) {
        let _ = (window, message);
    }
}

impl<F: FnMut(Window, &str) + 'static> Handler for F {
    fn handle(&mut self, window: Window, message: &str) {
        (self)(window, message)
    }
}

#[derive(Clone)]
/// A window, which may or may not be open.
pub struct Window {
    data: Rc<RefCell<Option<raw::tether>>>,
}

type Data = (Window, Box<dyn Handler>);

impl Window {
    /// Make a new window with the given options.
    pub fn new(opts: Options) -> Self {
        assert_main();

        let this = Window {
            data: Rc::new(RefCell::new(None)),
        };

        let handler = opts.handler.unwrap_or(Box::new(|_, _: &_| {}));

        let opts = raw::tether_options {
            initial_width: opts.initial_width,
            initial_height: opts.initial_height,
            minimum_width: opts.minimum_width,
            minimum_height: opts.minimum_height,

            borderless: opts.borderless,
            debug: opts.debug,

            data: Box::<Data>::into_raw(Box::new((this.clone(), handler))) as _,
            closed: Some(closed),
            message: Some(message),
        };

        let raw = unsafe { raw::tether_new(opts) };
        this.data.replace(Some(raw));

        unsafe extern fn closed(data: *mut c_void) {
            abort_on_panic(|| {
                let _ = Box::<Data>::from_raw(data as _);
            });
        }

        unsafe extern fn message(data: *mut c_void, message: *const i8) {
            abort_on_panic(|| {
                let data = data as *mut Data;

                match CStr::from_ptr(message).to_str() {
                    Ok(message) => {
                        (*data).1.handle((*data).0.clone(), message);
                    }
                    Err(e) => {
                        error!("{}", e);
                    }
                }
            });
        }

        this
    }

    /// Make a new window with the default options and the given handler.
    pub fn with_handler(handler: impl Handler) -> Self {
        Self::new(Options {
            handler: Some(Box::new(handler)),
            ..Default::default()
        })
    }

    /// Evaluate the given JavaScript asynchronously.
    pub fn eval<I: Into<String>>(&self, s: I) {
        if let Some(data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_eval(data, s.as_ptr());
            }
        }
    }

    /// Load the given HTML asynchronously.
    pub fn load<I: Into<String>>(&self, s: I) {
        if let Some(data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_load(data, s.as_ptr());
            }
        }
    }

    /// Set this window's title to the given string.
    pub fn title<I: Into<String>>(&self, s: I) {
        if let Some(data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_title(data, s.as_ptr());
            }
        }
    }

    /// Focus this window above the other windows.
    pub fn focus(&self) {
        if let Some(data) = *self.data.borrow_mut() {
            unsafe {
                raw::tether_focus(data);
            }
        }
    }

    /// Close this window.
    pub fn close(&self) {
        if let Some(data) = *self.data.borrow_mut() {
            unsafe {
                raw::tether_close(data);
            }
        }
    }
}

impl Default for Window {
    fn default() -> Self {
        Self::new(Default::default())
    }
}

/// The window options.
///
/// Note that these are mostly *suggestions* rather than *requirements*.
pub struct Options {
    /// The initial window width in pixels.
    pub initial_width: usize,
    /// The initial window height in pixels.
    pub initial_height: usize,
    /// The minimum window width in pixels.
    pub minimum_width: usize,
    /// The minimum window height in pixels.
    pub minimum_height: usize,

    /// Whether to draw the title bar and stuff like that.
    pub borderless: bool,
    /// I'm not entirely sure what enabling this does.
    pub debug: bool,

    /// The window's handler.
    pub handler: Option<Box<dyn Handler>>,
}

impl Default for Options {
    fn default() -> Self {
        Self {
            initial_width: 640,
            initial_height: 480,
            minimum_width: 480,
            minimum_height: 360,

            borderless: false,
            debug: false,

            handler: None,
        }
    }
}

/// Initialize things; call this first.
///
/// By calling this function, you're promising us that you haven't called it
/// before, and that this is the main thread. The provided callback should
/// contain your "real" main function.
pub unsafe fn start(cb: fn()) {
    static mut INIT: Option<fn()> = None;
    INIT = Some(cb);

    unsafe extern fn init() {
        abort_on_panic(|| {
            MAIN_THREAD.with(|initialized| {
                initialized.set(true);
            });

            INITIALIZED.store(true, Ordering::Relaxed);

            INIT.unwrap()();
        });
    }

    raw::tether_start(Some(init));
}

/// Terminate the application as gracefully as possible.
pub fn exit() {
    assert_main();

    unsafe {
        raw::tether_exit();
    }
}

/// Run the given function on the main thread.
pub fn dispatch<F: FnOnce() + Send>(f: F) {
    assert_initialized();

    unsafe {
        raw::tether_dispatch(
            Box::<F>::into_raw(Box::new(f)) as _,
            Some(execute::<F>),
        );
    }

    unsafe extern fn execute<F: FnOnce() + Send>(data: *mut c_void) {
        abort_on_panic(|| {
            Box::<F>::from_raw(data as _)();
        });
    }
}

fn abort_on_panic<F: FnOnce() + panic::UnwindSafe>(f: F) {
    if panic::catch_unwind(f).is_err() {
        process::abort();
    }
}

/// Make sure that we're initialized.
fn assert_initialized() {
    assert!(INITIALIZED.load(Ordering::Relaxed));
}

/// Make sure that we're initialized and on the main thread.
fn assert_main() {
    MAIN_THREAD.with(|initialized| {
        assert!(initialized.get());
    });
}

fn string_to_cstring<I: Into<String>>(s: I) -> CString {
    CString::new(s.into()).unwrap()
}

mod raw {
    #![allow(dead_code, nonstandard_style)]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
