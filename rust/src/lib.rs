mod escape;
mod handler;
mod options;
mod window;

pub use self::escape::*;
pub use self::handler::*;
pub use self::options::*;
pub use self::window::*;

pub fn builder() -> Options<()> {
    Options::new()
}

pub fn start<F: FnOnce()>(_f: F) -> ! {
    unimplemented!()
}

pub fn dispatch<F: FnOnce() + Send + 'static>(_f: F) {
    unimplemented!()
}
