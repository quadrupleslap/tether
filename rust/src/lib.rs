use log::error;
use std::cell::{Cell, RefCell};
use std::ffi::{c_void, CStr, CString};
use std::rc::Rc;

thread_local! {
    static INITIALIZED: Cell<bool> = Cell::new(false);
}

pub trait Handler: 'static {
    fn handle(&mut self, message: &str);
}

impl<F: FnMut(&str) + 'static> Handler for F {
    fn handle(&mut self, message: &str) {
        (self)(message)
    }
}

#[derive(Clone)]
pub struct Window {
    data: Data,
}

type Data = Rc<RefCell<Option<(raw::tether, Box<dyn Handler>)>>>;

impl Window {
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

    pub fn with_handler(handler: impl Handler) -> Self {
        Self::new(Options {
            handler: Some(Box::new(handler)),
            ..Default::default()
        })
    }

    pub fn eval<I: Into<String>>(&self, s: I) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_eval(data.0, s.as_ptr());
            }
        }
    }

    pub fn load<I: Into<String>>(&self, s: I) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_load(data.0, s.as_ptr());
            }
        }
    }

    pub fn title<I: Into<String>>(&self, s: I) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            let s = string_to_cstring(s);
            unsafe {
                raw::tether_title(data.0, s.as_ptr());
            }
        }
    }

    pub fn focus(&self) {
        if let Some(ref mut data) = *self.data.borrow_mut() {
            unsafe {
                raw::tether_focus(data.0);
            }
        }
    }

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

pub struct Options {
    pub initial_width: usize,
    pub initial_height: usize,
    pub minimum_width: usize,
    pub minimum_height: usize,

    pub borderless: bool,
    pub debug: bool,

    pub handler: Option<Box<dyn Handler>>,
}

impl Options {
    pub fn build(self) -> Window {
        Window::new(self)
    }
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

pub fn exit() {
    assert_initialized();

    unsafe {
        raw::tether_exit();
    }
}

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
