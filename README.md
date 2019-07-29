# Tether

[![Cargo](https://img.shields.io/crates/v/tether.svg)](https://crates.io/crates/tether)
[![Docs.rs](https://docs.rs/tether/badge.svg)](https://docs.rs/tether)

If you're…

- writing a C or Rust application,
- familiar with HTML, and
- want to painlessly add a cross-platform GUI,

then this is the library for you!

## Getting Started

1. Make sure you've installed Clang and your system's SDK.
    - macOS — Xcode
    - Linux — GTK and Webkit2GTK
    - Windows — Visual Studio
2. Add this library to your `Cargo.toml`.
3. Read [an example](examples/hello.rs) and [the documentation](https://docs.rs/tether).

### Warning

On Windows, you *need* to use Rust's MSVC build. The MinGW build will just
produce a bunch of weird errors.

## Supported Platforms

| Operating System | Library    | System Requirements |
| ---------------- | ---------- | ------------------- |
| Windows          | UWP        | Windows 10          |
| macOS            | WebKit     | macOS 10.10         |
| Linux            | Webkit2GTK | Webkit2GTK 2.22     |
