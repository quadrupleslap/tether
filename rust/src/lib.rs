#![deny(missing_docs)]

//! Windows that are web views.

use log::error;
use std::cell::{Cell, RefCell};
use std::ffi::{c_void, CStr, CString};
use std::rc::Rc;

thread_local! {
    static INITIALIZED: Cell<bool> = Cell::new(false);
}

/// An event handler; you probably want to implement one.
///
/// - When the webpage calls `window.tether`, the message is passed to `handle`.
/// - The handler is dropped when the window is closed.
pub trait Handler: 'static {
    /// The webpage called `window.tether` with the given string.
    fn handle(&mut self, message: &str);
}

impl<F: FnMut(&str) + 'static> Handler for F {
    fn handle(&mut self, message: &str) {
        (self)(message)
    }
}

#[derive(Clone)]
/// A window, which may or may not be open.
pub struct Window {
    data: Data,
}

type Data = Rc<RefCell<Option<(raw::tether, Box<dyn Handler>)>>>;

impl Window {
    /// Make a new window with the given options.
    pub fn new(opts: Options) -> Self {
        assert_initialized();

        let data = Rc::new(RefCell::new(None));

        let handler = opts.handler.unwrap_or(Box::new(|_: &_| {}));

        let opts = raw::tether_options {
            initial_width: opts.initial_width,
            initial_height: opts.initial_height,
            minimum_width: opts.minimum_width,
            minimum_height: opts.minimum_height,

            borderless: opts.borderless,
            debug: opts.debug,

            data: Rc::into_raw(data.clone()) as *mut c_void,
            closed: Some(closed),
            message: Some(message),
        };

        let raw = unsafe { raw::tether_new(opts) };
        data.replace(Some((raw, handler)));

        unsafe extern fn closed(data: *mut c_void) {
            let _: Data = Rc::from_raw(data as _);
        }

        unsafe extern fn message(data: *mut c_void, message: *const i8) {
            match CStr::from_ptr(message).to_str() {
                Ok(message) => {
                    let data: Data = Rc::from_raw(data as _);

                    if let Some(ref mut data) = *data.borrow_mut() {
                        data.1.handle(message);
                    }

                    Rc::into_raw(data);
                }
                Err(e) => {
                    error!("{}", e);
                }
            }
        }

        Self { data }
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
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_eval(data.0, s.as_ptr());
            }
        }
    }

    /// Load the given HTML asynchronously.
    pub fn load<I: Into<String>>(&self, s: I) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_load(data.0, s.as_ptr());
            }
        }
    }

    /// Set this window's title to the given string.
    pub fn title<I: Into<String>>(&self, s: I) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_title(data.0, s.as_ptr());
            }
        }
    }

    /// Focus this window above the other windows.
    pub fn focus(&self) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            unsafe {
                raw::tether_focus(data.0);
            }
        }
    }

    /// Close this window.
    pub fn close(&self) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            unsafe {
                raw::tether_close(data.0);
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
        INITIALIZED.with(|initialized| {
            initialized.set(true);
        });

        INIT.unwrap()();
    }

    raw::tether_start(Some(init));
}

/// Terminate the application as gracefully as possible.
pub fn exit() {
    assert_initialized();

    unsafe {
        raw::tether_exit();
    }
}

/// Run the given function on the main thread.
pub fn dispatch<F: FnOnce() + Send>(f: F) {
    assert_initialized();

    unsafe {
        raw::tether_dispatch(
            Box::into_raw(Box::new(f)) as _,
            Some(execute::<F>),
        );
    }

    unsafe extern fn execute<F: FnOnce() + Send>(data: *mut c_void) {
        Box::<F>::from_raw(data as _)();
    }
}

/// Make sure that we're initialized and on the main thread.
fn assert_initialized() {
    INITIALIZED.with(|initialized| {
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
