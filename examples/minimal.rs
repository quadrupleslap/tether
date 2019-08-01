use tether::{Options, Window};

const HTML: &str = r#"
<!doctype html>
    <html>
        <body style="text-align: center;">
            <h1>This is a minimal example of Tether!</h1>
        </body>
    </html>
"#;

fn start() {
    let options = Options {
        initial_width: 960,
        initial_height: 720,
        minimum_width: 640,
        minimum_height: 480,
        icon_name: "examples/resources/rust-logo.png",
        borderless: false,
        debug: true,
        handler: None,
    };
    let window = Window::new(options);
    window.title("Tether Application");
    window.load(HTML);
}

fn main() {
    unsafe {
        tether::start(start)
    }
}