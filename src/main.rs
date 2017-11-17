extern crate app_dirs;
extern crate cc;
extern crate rio;

use app_dirs::*;
use cc::Build;
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
  Build::new().
    host("x86_64-pc-windows-msvc").
    opt_level(0).
    out_dir(out_dir).
    target("x86_64-pc-windows-msvc").
    file("notes/hello.c").
    compile("hello");
}
