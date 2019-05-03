# Tether

[![Cargo](https://img.shields.io/crates/v/tether.svg)](https://crates.io/crates/tether)
[![Docs.rs](https://docs.rs/tether/badge.svg)](https://docs.rs/tether)

Open a window that consists entirely of a single web view.

## Getting Started

1. Make sure you've installed Clang, Make, and your system's SDK.
    - macOS — Xcode
    - Linux — GTK and Webkit2GTK
    - Windows — Visual Studio
2. Add this library to your `Cargo.toml`.
3. Read over [an example](rust/examples/hello.rs).

### Warning

On Windows, you *need* to use Rust's MSVC build. The MinGW build will just
produce a bunch of weird errors.

## Supported Platforms

| Operating System | Library    | System Requirements |
| ---------------- | ---------- | ------------------- |
| Windows          | UWP        | Windows 10          |
| macOS            | WebKit     | macOS 10.10         |
| Linux            | Webkit2GTK | Webkit2GTK 2.8      |
