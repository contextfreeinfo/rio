extern crate app_dirs;
extern crate cbuild;
extern crate rio;

use app_dirs::*;
use cbuild::Build;
use cbuild::BuildKind;
use rio::process;
use std::env;

const APP_INFO: AppInfo = AppInfo {author: "TJPalmer", name: "Rio"};

fn main() {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  process(name).expect("Failed to parse");
  // TODO How to invent cache subdirs?
  // TODO Hash full path for scripts without project files?
  // TODO Hash by project file if there is one?
  let out_dir = app_dir(AppDataType::UserCache, &APP_INFO, "cache").unwrap();
  println!("{}", out_dir.to_str().unwrap());
  // TODO Remember host and target from own build as defaults?
  // TODO Determine both live probably, actually ...
  // x86_64 vs i686
  Build::new().
    host("i686-pc-windows-msvc").
    kind(BuildKind::Executable).
    opt_level(0).
    out_dir(out_dir).
    target("i686-pc-windows-msvc").
    // verbose(true).
    file("notes/hello.c").
    compile("hello");
}
