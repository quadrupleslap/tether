use crate::{Handler, Options};
use std::marker::PhantomData;

pub struct Window {
    /// This is here just to make sure the window isn't `Send` or `Sync`.
    spooky: PhantomData<*mut ()>,
}

impl Window {
    pub fn new<H: Handler>(_opts: Options<H>) -> Self {
        Window {
            spooky: PhantomData,
        }
    }

    pub fn eval(&self, _js: &str) {
        unimplemented!()
    }

    pub fn load(&self, _html: &str) {
        unimplemented!()
    }

    pub fn close(&self) {
        unimplemented!()
    }
}

impl Clone for Window {
    fn clone(&self) -> Self {
        unimplemented!()
    }
}
