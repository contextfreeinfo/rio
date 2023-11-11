use std::{
    fs,
    path::{Path, PathBuf},
    process::Command,
    time::Instant,
};

use clap::Parser;

#[derive(clap::Parser)]
struct Cli {
    #[arg(long)]
    profile: Option<String>,
}

fn main() {
    let cli = Cli::parse();
    install_tools();
    let profile = cli.profile.unwrap_or("release".into());
    build_and_run(&profile);
}

fn build_and_run(profile: &str) {
    // Build
    println!("building profile {profile} ...");
    let profile_args = match profile {
        "debug" => vec![],
        "release" => vec![format!("--{profile}")],
        _ => ["--profile", profile].iter().map(|s| (*s).into()).collect(),
    };
    Command::new(CARGO)
        .arg("build")
        .args(profile_args)
        .status()
        .unwrap();
    let rio = Path::new("target").join(profile).join("rio");
    // Report
    let Ok(metadata) = fs::metadata(&rio) else {
        panic!()
    };
    println!(
        "built rio: {} bytes or {:.3} MB",
        metadata.len(),
        metadata.len() as f64 / (1 << 20) as f64
    );
    // Run
    for example in ["hi", "wild"] {
        run_example(&rio, example);
    }
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
    install_if_missing(WASM_TOOLS);
    // install_if_missing("wasmi_cli");
}

fn run_example(rio: &PathBuf, example: &str) {
    let examples = "examples";
    let examples_out = format!("{examples}/out");
    let example_path = format!("{examples}/{example}.rio");
    // Wasm
    let start = Instant::now();
    Command::new(rio)
        .args(["build", &example_path])
        .args(["--dump", "trees"])
        .args(["--outdir", &examples_out])
        .env("RUST_BACKTRACE", "1")
        .output()
        .unwrap();
    println!("built {example_path} in {:.1?}", start.elapsed());
    // Wat
    Command::new(WASM_TOOLS)
        .args(["print", &format!("{examples_out}/{example}.wasm")])
        .args(["--output", &format!("{examples_out}/{example}.wat")])
        .status()
        .unwrap();
    // TODO Run
}

const CARGO: &str = "cargo";
const WASM_TOOLS: &str = "wasm-tools";
