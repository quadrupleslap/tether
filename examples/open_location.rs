use std::env;
use tether::Window;
static DEFAULT_URL: &str = "http://localhost:3000/";

fn start() {

    let args = env::args().collect::<Vec<_>>();
    let url = match args.len() {
        1 => DEFAULT_URL,
        2 => &args[1],
        _ => {
            eprintln!("{} [location (default: {})]", args[0], DEFAULT_URL);
            std::process::exit(1);
        }
    };

    let window = Window::with_handler(Handler(0));
    window.title("Hello, world!");
    window.load(format!(
        r#"
            <p>
                Loading {} ...
            </p>
            <script>
                window.location.href = "{}";
            </script>
        "#,
        url, url
    ));
}

struct Handler(pub usize);

impl tether::Handler for Handler {
    fn handle(&mut self, _window: Window, msg: &str) {
        println!("{}", msg);

        //     self.0 += 1;
        //     window.eval(format!(
        //         "
        //             document.getElementById('click-count').textContent = {};
        //         ",
        //         self.0,
        //     ));
    }
}

impl Drop for Handler {
    fn drop(&mut self) {
        println!("Goodbye!");
        tether::exit();
    }
}

fn main() {
    unsafe { tether::start(start) }
}
