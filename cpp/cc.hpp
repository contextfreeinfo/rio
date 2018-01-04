#pragma once

#include "dirs.hpp"
#include "sub.hpp"

namespace rio {

#ifdef _WIN32

auto compile_c(const fs::path& path, bool verbose = false) -> void {
  Process compile{
    // TODO Find cl. Set env!
    "cl", path.filename().string(),
    // "cat", path.filename().string(),
  };
  compile.dir = path.parent_path();
  // Debug.
  if (verbose) {
    std::cout << compile.dir << ":";
    for (auto& arg: compile.args) {
      std::cout << " " << arg;
    }
    std::cout << std::endl;
  }
  auto output = compile.check_output();
  // Debug.
  if (verbose) {
    std::cout << output << std::endl;
  }
}

#else

auto compile_c(const fs::path& path, bool verbose = false) -> void {
  Process compile{
    "cc", "-o", path.stem().string(), path.filename().string(),
    // "cat", path.filename().string(),
  };
  compile.dir = path.parent_path();
  // Debug.
  if (verbose) {
    std::cout << compile.dir << ":";
    for (auto& arg: compile.args) {
      std::cout << " " << arg;
    }
    std::cout << std::endl;
  }
  auto output = compile.check_output();
  // Debug.
  if (verbose) {
    std::cout << output << std::endl;
  }
}

#endif

}
