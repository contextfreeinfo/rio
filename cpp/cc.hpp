#pragma once

#include "dirs.hpp"
#include "sub.hpp"

#ifdef _WIN32
# include "Setup.Configuration.h"
#endif

namespace rio {

#ifdef _WIN32

struct CoBstr {
  BSTR bstr;
  ~CoBstr() {
    if (bstr) SysFreeString(bstr);
  }
  auto operator&() -> BSTR* {
    return &bstr;
  }
};

template<typename Interface>
struct CoInstance {
  Interface* instance = nullptr;
  ~CoInstance() {
    if (instance) {
      // std::cout << "releasing an instance" << std::endl;
      instance->Release();
    }
  }
  auto operator->() -> Interface* {
    return instance;
  }
  auto operator&() -> Interface** {
    return &instance;
  }
  operator bool() {
    return instance;
  }
  operator void**() {
    return reinterpret_cast<void**>(&instance);
  }
};

auto find_msvc() -> void {
  // Based on Setup.Configuration.ZeroDeps.VC.cpp.
  // Not using atl since it seems not always available by default (such as for
  // my setup), and I want easy building.
  if (CoInitializeEx(NULL, COINIT_MULTITHREADED)) {
    throw std::runtime_error("no com");
  }
  CoInstance<ISetupConfiguration2> setup_config;
  if (CoCreateInstance(
    __uuidof(SetupConfiguration), nullptr, CLSCTX_INPROC_SERVER,
    IID_ISetupConfiguration2, setup_config
  )) {
    // TODO Check for older visual studio versions in the registry.
    throw std::runtime_error("no setup config");
  }
  CoInstance<ISetupHelper> setup_helper;
  if (setup_config->QueryInterface(IID_ISetupHelper, setup_helper)) {
    throw std::runtime_error("no setup helper");
  }
  CoInstance<IEnumSetupInstances> enum_instances;
  if (setup_config->EnumAllInstances(&enum_instances)) {
    throw std::runtime_error("enum instances failed");
  }
  while (true) {
    CoInstance<ISetupInstance> instance;
    if (!enum_instances->Next(1, &instance, nullptr) && instance) {
      // std::cout << "Got an instance!" << std::endl;
      CoInstance<ISetupInstance2> instance2;
      if (instance->QueryInterface(IID_ISetupInstance2, instance2)) {
        throw std::runtime_error("instance2 failed");
      }
      std::cout << "Got instance2" << std::endl;
      CoBstr wpath;
      if (instance2->GetInstallationPath(&wpath)) {
        throw std::runtime_error("instance path failed");
      }
      auto path = wstr_to_u8(wpath.bstr);
      std::cout << "path: " << path << std::endl;
    } else {
      break;
    }
  }
  // HKEY hKey;
  // auto result = RegOpenKeyExW(
  //   HKEY_LOCAL_MACHINE,
  //   LR"(SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7)",
  //   0, KEY_READ, &hKey
  // );
  // if (result) {
  //   throw std::runtime_error("failed to read registry");
  // }
  std::cout << "Yo!!" << std::endl;
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
