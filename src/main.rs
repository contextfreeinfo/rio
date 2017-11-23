extern crate app_dirs;
extern crate cbuild;
extern crate rio;

use app_dirs::*;
use cbuild::prelude::*;
use rio::process;
use std::fs::File;
use std::io::prelude::*;
use std::process::*;
use std::env;

const APP_INFO: AppInfo = AppInfo {author: "TJPalmer", name: "Rio"};

fn main() {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  // TODO Actually build c program from source.
  process(name).expect("Failed to parse");
  // TODO How to invent cache subdirs?
  // TODO Hash full path for scripts without project files?
  // TODO Hash by project file if there is one?
  let out_dir = app_dir(AppDataType::UserCache, &APP_INFO, "cache").unwrap();
  let name = "hello";
  let c_name = format!("{}.c", name);
  let c_path = out_dir.join(c_name);
  {
    let mut file = File::create(c_path.clone()).unwrap();
    file.write_all(r###"
      #include <stdio.h>
      int main() {
        printf("Hi there!\n");
      }
    "###.as_bytes()).unwrap();
  }
  // println!("{}", out_dir.to_str().unwrap());
  // Build exe.
  // TODO Remember host and target from own build as defaults?
  // TODO Determine both live probably, actually ...
  // x86_64 vs i686
  // TODO Return full executable path name, since that includes ".exe" on
  // TODO windows.
  Build::new().
    host("i686-pc-windows-msvc").
    kind(BuildKind::Executable).
    opt_level(0).
    out_dir(out_dir.clone()).
    target("i686-pc-windows-msvc").
    // verbose(true).
    file(c_path.clone()).
    compile("hello");
  // Run exe.
  Command::new(out_dir.join("hello")).status().expect("failed to run exe");
}
