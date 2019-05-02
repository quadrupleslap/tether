use tether::Window;

const HTML: &'static str = "
    <p>
        Hello and, again, welcome to the Aperture Science computer-aided
        enrichment center. We hope your brief detention in the relaxation vault
        has been a pleasant one.
    </p>
    <button onclick=\"tether('Bzzz.')\">Click here!</button>
";

fn start() {
    let window = Window::with_handler(Handler);
    window.title("Hello, world!");
    window.load(HTML);
}

struct Handler;

impl tether::Handler for Handler {
    fn handle(&mut self, msg: &str) {
        println!("{}", msg);
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
