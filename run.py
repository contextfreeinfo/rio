#!/usr/bin/env python

import pathlib as p
import subprocess as sp
import time as t


def main() -> None:
    install_tools()
    profile = "release"
    # profile = "release-lto"
    build_and_run(profile)


def build_and_run(profile: str) -> None:
    # Build
    print(f"building profile {profile} ...")
    if profile in {"debug", "release"}:
        profile_args = [f"--{profile}"]
    else:
        profile_args = ["--profile", profile]
    sp.check_call([cargo, "build", *profile_args])
    rio = p.Path("target", profile, "rio")
    # Report
    size = rio.stat().st_size
    print(f"built {rio}: {size:,} bytes or {size / (1 << 20):.3f} MB")
    # Run
    for example in ["hi", "wild"]:
        run_example(rio, example)


def install(command: str) -> None:
    sp.check_call([cargo, "install", command])


def install_if_missing(command: str) -> None:
    if not is_found(command):
        print(f"{command} not found; installing ...")
        install(command)


def install_tools() -> None:
    install_if_missing(wasm_tools)
    install_if_missing(wasmi_cli)


def is_found(command: str) -> bool:
    try:
        sp.check_call([command, "-h"], stdout=sp.DEVNULL, stderr=sp.DEVNULL)
        return True
    except FileNotFoundError:
        return False


def run_example(rio: p.Path, example: str) -> None:
    # Wasm
    start_time = t.time()
    examples = "examples"
    examples_out = f"{examples}/out"
    example_path = f"{examples}/{example}.rio"
    sp.check_call(
        args=[
            rio,
            "run",
            example_path,
            *["--dump", "trees"],
            *["--outdir", examples_out],
        ],
        env={"RUST_BACKTRACE": "1"},
    )
    elapsed = t.time() - start_time
    print(f"built {example_path} in {elapsed:.4f} seconds")
    # Wat
    sp.check_call(
        args=[
            wasm_tools,
            "print",
            f"{examples_out}/{example}.wasm",
            "--output",
            f"{examples_out}/{example}.wat",
        ]
    )
    # TODO Run


cargo = "cargo"
wasm_tools = "wasm-tools"
wasmi_cli = "wasmi_cli"


if __name__ == "__main__":
    main()
