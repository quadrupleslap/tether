use std::cell::RefCell;
use std::thread;
use std::time::Duration;
use tether::Window;

const HTML: &'static str = "
    <style>
        body {
            background-color: black;
            color: green;
            font-family: monospace;
        }
    </style>

    :)
";

thread_local! {
    static WINDOW: RefCell<Option<Window>> = RefCell::new(None);
}

fn start() {
    let window = Window::with_handler(Handler);
    window.title("Hello, world!");
    window.load(HTML);

    WINDOW.with(|w| {
        *w.borrow_mut() = Some(window);
    });

    thread::spawn(ticker);
}

fn ticker() {
    for i in 0.. {
        tether::dispatch(|| {
            WINDOW.with(|window| {
                window
                    .borrow()
                    .as_ref()
                    .unwrap()
                    .eval(format!(
                        "document.body.textContent = '{} ({})';",
                        if i % 2 == 1 { "Tick" } else { "Tock" },
                        i,
                    ));
            });
        });

        thread::sleep(Duration::from_secs(1));
    }
}

struct Handler;

impl tether::Handler for Handler {}

impl Drop for Handler {
    fn drop(&mut self) {
        tether::exit();
    }
}

fn main() {
    unsafe { tether::start(start) }
}
