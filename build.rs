use std::{env, io};
use std::path::PathBuf;
use std::process::Command;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    let rust_path = env::current_dir()?;
    let native_path = rust_path.join("native");

    env::set_current_dir(&native_path)?;

    // Link any platform-specific dependencies.

    if cfg!(target_os = "linux") {
        pkg_config::Config::new()
            .atleast_version("3.14")
            .probe("gtk+-3.0")?;

        pkg_config::Config::new()
            .atleast_version("2.8")
            .probe("webkit2gtk-4.0")?;
    } else if cfg!(target_os = "windows") {
        println!("cargo:rustc-link-lib=dylib=ole32");
        println!("cargo:rustc-link-lib=dylib=user32");
        println!("cargo:rustc-link-lib=dylib=windowsapp");
    } else if cfg!(target_os = "macos") {
        println!("cargo:rustc-link-lib=framework=Cocoa");
        println!("cargo:rustc-link-lib=framework=WebKit");
    } else {
        panic!("unsupported platform");
    }

    // Build the library.

    Command::new("make").status()?;

    // Link the library.

    println!("cargo:rustc-link-search=native={}", native_path.display());
    println!("cargo:rustc-link-lib=static=tether");

    // Generate the bindings to the library.

    bindgen::Builder::default()
        .header("tether.h")
        .generate()
        .map_err(|()| io::Error::new(io::ErrorKind::Other, "bindgen failed"))?
        .write_to_file(out_path.join("bindings.rs"))?;

    Ok(())
}
