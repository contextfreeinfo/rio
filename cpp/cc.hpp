#pragma once

#include "dirs.hpp"
#include "sub.hpp"

#ifdef _WIN32
// # include "Setup.Configuration.h"
#endif

namespace rio {

#ifdef _WIN32

auto find_msvc() -> void {
  HKEY hKey;
  auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7)", 0, KEY_READ, &hKey);
  if (result) {
    throw std::runtime_error("failed to read registry");
  }
  std::cout << "Yo!" << std::endl;
	// CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // ISetupInstance setup_config;
  // CoCreateInstance(__uuidof(SetupConfiguration), nullptr, CLSCTX_INPROC_SERVER, IID_ISetupInstance, &setup_config);
}

auto compile_c(const fs::path& path, bool verbose = false) -> void {
  // See also:
  // https://blogs.msdn.microsoft.com/vcblog/2017/03/06/finding-the-visual-c-compiler-tools-in-visual-studio-2017/
  // https://blogs.msdn.microsoft.com/heaths/2017/03/11/vswhere-now-searches-older-versions-of-visual-studio/
  // HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7
  find_msvc();
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
