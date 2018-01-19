#pragma once

#include "cc.hpp"
#include "dirs.hpp"
#include "generate.hpp"
#include "picosha2.h"
#include "resolve.hpp"
#include "std.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace rio {

struct Session {

  bool generate_only = false;

  fs::path build_path;

  fs::path main_path;

  bool show_tree = false;

  bool verbose = false;

  auto run() -> int {
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
    // TODO Include session here for counting defined ids.
    Script main{content, &std_context};
    // Diagnostics.
    if (show_tree) {
      Map<Node*, USize> def_indices;
      USize def_index = 0;
      for (auto script: std::vector<Script*>{&std_script, &main}) {
        for (auto def: script->defs) {
          def_indices[def] = def_index++;
        }
      }
      if (verbose) {
        std::cout << "--- std ---" << std::endl;
        std::cout << std_script.root.format(def_indices) << std::endl;
        std::cout << std::endl;
      }
      std::cout << "--- main ---" << std::endl;
      std::cout << main.root.format(def_indices) << std::endl;
      return 0;
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
    // Output path.
    // TODO Check by hash (or even date/size?) if already good.
    auto gen_path = build_path / main_path.stem();
    gen_path += ".c";
    if (verbose) {
      std::cout << "out file: " << gen_path << std::endl;
    }
    // Produce output, in constrained scope for flushing.
    CGenerator generator;
    {
      std::ofstream out{gen_path};
      // Generate.
      GenState gen_state{out};
      generator.generate(gen_state, main);
    }
    if (generate_only) {
      // std::cout << "generate done; ending" << std::endl;
      return 0;
    }
    // Compile c.
    // TODO Check by hash (or even date/size?) if already good.
    compile_c(gen_path.string(), verbose);
    // Run it in current dir.
    auto exe_name = (gen_path.parent_path() / gen_path.stem()).string();
    // TODO How does this work with winmain vs console on Windows?
    // TODO Also, probably need something other than system eventually.
    auto result = std::system(exe_name.data());
    return result;
  }

};

}
