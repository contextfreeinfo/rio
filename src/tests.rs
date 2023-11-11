#![cfg(test)]

use std::{fs, path::Path, process::Command};

#[test]
fn run_things() {
    install_tools();
    let profile = "release";
    let profile = "release-lto";
    build_and_run(profile);
}

fn build_and_run(profile: &str) {
    // Build
    println!("building profile {profile}");
    let profile_args = match profile {
        "debug" | "release" => vec![format!("--{profile}")],
        _ => vec!["--profile", profile]
            .iter()
            .map(|s| (*s).into())
            .collect(),
    };
    Command::new(CARGO)
        .arg("build")
        .args(profile_args)
        .status()
        .unwrap();
    let rio = Path::new("target").join(profile).join("rio");
    // Report
    let Ok(metadata) = fs::metadata(rio) else {
        panic!()
    };
    println!(
        "built rio: {} bytes or {:.3} MB",
        metadata.len(),
        metadata.len() as f64 / (1 << 20) as f64
    );
    // Run
    // TODO
}

fn install_if_missing(command: &str) {
    let Ok(_) = Command::new(command).arg("-h").output() else {
        println!("{command} not found; installing ...");
        Command::new(CARGO)
            .args(["install", command])
            .status()
            .unwrap();
        return;
    };
}

fn install_tools() {
    install_if_missing("wasm-tools");
    // install_if_missing("wasmi_cli");
}

const CARGO: &str = "cargo";
