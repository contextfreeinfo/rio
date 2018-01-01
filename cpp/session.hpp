#pragma once

#include "dirs.hpp"
#include "generate.hpp"
#include "picosha2.h"
#include "resolve.hpp"
#include "std.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace rio {

struct Session {

  fs::path build_path;

  fs::path main_path;

  bool show_tree = false;

  bool verbose = false;

  auto run() -> void {
    // Read source.
    std::stringstream buffer;
    {
      std::ifstream file{main_path};
      buffer << file.rdbuf();
    }
    // Parse.
    std::string content = buffer.str();
    auto std_script = std_init_c();
    Context std_context{std_script.root};
    Script main{content, &std_context};
    // Diagnostics.
    if (show_tree) {
      if (verbose) {
        std::cout << "--- std ---" << std::endl;
        std::cout << std_script.root.format() << std::endl;
        std::cout << std::endl;
      }
      std::cout << "--- main ---" << std::endl;
      std::cout << main.root.format() << std::endl;
    }
    // Paths.
    auto main_abs = fs::absolute(main_path);
    // std::cout << "script: " << main_abs << std::endl;
    auto main_abs_hash = picosha2::hash256_hex_string(main_abs.string());
    build_path = get_cache_dir() / "rio-lang" / "build" / main_abs_hash;
    if (!fs::exists(build_path)) {
      if (!fs::create_directories(build_path)) {
        throw std::runtime_error("failed to create build dir");
      }
    }
    // Output.
    auto gen_path = build_path / main_path.stem();
    gen_path += ".c";
    if (verbose) {
      std::cout << "out file: " << gen_path << std::endl;
    }
    std::ofstream out{gen_path};
    // Generate.
    GenState gen_state{out};
    CGenerator generator;
    generator.generate(gen_state, main);
  }

};

}
