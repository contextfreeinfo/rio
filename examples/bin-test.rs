use std::{
    fs,
    path::{Path, PathBuf},
    process::Command,
    time::{Duration, Instant},
};

use clap::Parser;

#[derive(clap::Parser)]
struct Cli {
    #[arg(long)]
    profile: Option<String>,
    #[arg(long)]
    quick: bool,
}

fn main() {
    let cli = Cli::parse();
    install_tools();
    let profile = cli.profile.clone().unwrap_or("release".into());
    build_and_run(&profile, &cli);
}

fn build_and_run(profile: &str, cli: &Cli) {
    // Build
    println!("building profile {profile} ...");
    let profile_args = match profile {
        "debug" => vec![],
        "release" => vec![format!("--{profile}")],
        _ => ["--profile", profile].iter().map(|s| (*s).into()).collect(),
    };
    let start = Instant::now();
    Command::new(CARGO)
        .arg("build")
        .args(profile_args)
        .status()
        .unwrap();
    let mut rio = PathBuf::new().join("target").join(profile).join("rio");
    if cfg!(target_os = "windows") {
        rio.set_extension("exe");
    }
    // Report
    report_build(&rio, start.elapsed());
    // Run
    for example in ["branch", "hi", "struct", "wild"] {
        run_example(&rio, example, cli);
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

fn report_build<P: AsRef<Path>>(path: P, duration: Duration) {
    let path = path.as_ref();
    let Ok(metadata) = fs::metadata(path) else {
        panic!()
    };
    println!(
        "built {} in {:.1?}: {} bytes ({:.2} MB)",
        path.to_str().unwrap(),
        duration,
        metadata.len(),
        metadata.len() as f64 / (1 << 20) as f64
    );
}

fn run_example(rio: &PathBuf, example: &str, cli: &Cli) {
    let examples = "examples";
    let examples_out = format!("{examples}/out");
    let example_path = format!("{examples}/{example}.rio");
    // Wasm
    let start = Instant::now();
    let status = Command::new(rio)
        .args(["build", &example_path])
        .args(if cli.quick {
            vec![]
        } else {
            vec!["--dump", "trees"]
        })
        .args(["--outdir", &examples_out])
        .arg("--time")
        .env("RUST_BACKTRACE", "1")
        .status()
        .unwrap();
    if !status.success() {
        println!("build failed");
        return;
    }
    let wasm = format!("{examples_out}/{example}.wasm");
    report_build(Path::new(&wasm), start.elapsed());
    // Wat
    Command::new(WASM_TOOLS)
        .args(["print", &wasm])
        .args(["--output", &format!("{examples_out}/{example}.wat")])
        .status()
        .unwrap();
    // TODO Run
}

const CARGO: &str = "cargo";
const WASM_TOOLS: &str = "wasm-tools";
