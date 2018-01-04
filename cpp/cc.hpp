#pragma once

#include "dirs.hpp"
#include "sub.hpp"

namespace rio {

#ifdef _WIN32

auto compile_c(const fs::path& path) -> void {
  Process compile{
    // TODO
    // "cl", path.filename().string(),
    "cat", path.filename().string(),
  };
  compile.dir = path.parent_path();
  // Debug.
  if (true) {
    std::cout << compile.dir << ":";
    for (auto& arg: compile.args) {
      std::cout << " " << arg;
    }
    std::cout << std::endl;
  }
  auto output = compile.check_output();
  // Debug.
  std::cout << output << std::endl;
}

#else

auto compile_c(const fs::path& path) -> void {
  Process compile{
    "cc", "-o", path.stem().string(), path.filename().string(),
    // "cat", path.filename().string(),
  };
  compile.dir = path.parent_path();
  // Debug.
  if (false) {
    std::cout << compile.dir << ":";
    for (auto& arg: compile.args) {
      std::cout << " " << arg;
    }
    std::cout << std::endl;
  }
  auto output = compile.check_output();
  // Debug.
  std::cout << output << std::endl;
}

#endif

}
