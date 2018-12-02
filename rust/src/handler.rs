use crate::Window;

pub trait Handler: 'static {
    fn message(&mut self, win: Window, msg: &str);
}

impl<F: FnMut(Window, &str) + 'static> Handler for F {
    fn message(&mut self, win: Window, msg: &str) {
        self(win, msg)
    }
}

impl Handler for () {
    fn message(&mut self, _: Window, _: &str) {}
}
