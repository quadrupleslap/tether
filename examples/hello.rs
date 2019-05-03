use tether::Window;

const HTML: &'static str = "
    <p>
        Hello and, again, welcome to the Aperture Science computer-aided
        enrichment center. We hope your brief detention in the relaxation vault
        has been a pleasant one.
    </p>
    <button onclick=\"tether('Bzzz.')\">Click here!</button>
    <p>
        The button has been clicked <span id=\"click-count\">0</span> times.
    </p>
";

fn start() {
    let window = Window::with_handler(Handler(0));
    window.title("Hello, world!");
    window.load(HTML);
}

struct Handler(pub usize);

impl tether::Handler for Handler {
    fn handle(&mut self, window: Window, msg: &str) {
        println!("{}", msg);

        self.0 += 1;
        window.eval(format!(
            "
                document.getElementById('click-count').textContent = {};
            ",
            self.0,
        ));
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
