use std::{env, io};
use std::path::PathBuf;
use std::process::Command;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    let rust_path = env::current_dir()?;
    let tether_path = rust_path.join("../tether");

    env::set_current_dir(&tether_path)?;

    // Link any platform-specific dependencies.

    if cfg!(target_os = "linux") {
        pkg_config::Config::new()
            .atleast_version("3.14")
            .probe("gtk+-3.0")?;

        pkg_config::Config::new()
            .atleast_version("2.4")
            .probe("webkit2gtk-4.0")?;
    } else if cfg!(target_os = "windows") {
        //TODO: Windows dependencies.
    } else if cfg!(target_os = "macos") {
        //TODO: macOS dependencies.
    } else {
        panic!("unsupported platform");
    }

    // Build the library.

    Command::new("make").status()?;

    // Link the library.

    println!("cargo:rustc-link-search=native={}", tether_path.display());
    println!("cargo:rustc-link-lib=static=tether");

    // Generate the bindings to the library.

    bindgen::Builder::default()
        .header("tether.h")
        .generate()
        .map_err(|()| io::Error::new(io::ErrorKind::Other, "bindgen failed"))?
        .write_to_file(out_path.join("bindings.rs"))?;

    Ok(())
}
