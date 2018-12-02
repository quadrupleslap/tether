use crate::{Handler, Window};

pub struct Options<H> {
    initial_size: Option<(usize, usize)>,
    minimum_size: Option<(usize, usize)>,
    fullscreen: bool,
    maximized: bool,
    borderless: bool,
    debug: bool,
    handler: H,
}

impl Options<()> {
    pub fn new() -> Self {
        Self {
            initial_size: None,
            minimum_size: None,
            fullscreen: false,
            maximized: false,
            borderless: false,
            debug: false,
            handler: (),
        }
    }
}

impl<H> Options<H> {
    pub fn initial_size(mut self, width: usize, height: usize) -> Self {
        self.initial_size = Some((width, height));
        self
    }

    pub fn minimum_size(mut self, width: usize, height: usize) -> Self {
        self.minimum_size = Some((width, height));
        self
    }

    pub fn fullscreen(mut self) -> Self {
        self.fullscreen = true;
        self
    }

    pub fn maximized(mut self) -> Self {
        self.maximized = true;
        self
    }

    pub fn borderless(mut self) -> Self {
        self.borderless = true;
        self
    }

    pub fn debug(mut self) -> Self {
        self.debug = true;
        self
    }

    pub fn handler<J>(self, handler: J) -> Options<J> {
        Options {
            initial_size: self.initial_size,
            minimum_size: self.minimum_size,
            fullscreen: self.fullscreen,
            maximized: self.maximized,
            borderless: self.borderless,
            debug: self.debug,
            handler,
        }
    }
}

impl<H: Handler> Options<H> {
    pub fn build(self) -> Window {
        Window::new(self)
    }
}
