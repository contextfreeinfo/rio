#pragma once

#include "dirs.hpp"
#include "sub.hpp"
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

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

auto find_msvs() -> std::string {
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
      // std::cout << "Got instance2" << std::endl;
      CoBstr wpath;
      if (instance2->GetInstallationPath(&wpath)) {
        throw std::runtime_error("instance path failed");
      }
      // TODO Check versions using setup_helper to make sure we have the most
      // TODO recent version, then return that.
      return wstr_to_u8(wpath.bstr);
    } else {
      break;
    }
  }
  // Alternative and/or getting started for older msvs versions.
  // HKEY hKey;
  // auto result = RegOpenKeyExW(
  //   HKEY_LOCAL_MACHINE,
  //   LR"(SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7)",
  //   0, KEY_READ, &hKey
  // );
  // if (result) {
  //   throw std::runtime_error("failed to read registry");
  // }
  return "";
}

auto find_cl() -> fs::path {
  fs::path msvs{find_msvs()};
  auto aux = msvs / "VC" / "Auxiliary" / "Build";
  auto version_file = aux / "Microsoft.VCToolsVersion.default.txt";
  std::ifstream version_in{version_file};
  std::string version;
  version_in >> version;
  std::cout << version << std::endl;
  auto cl =
    msvs / "VC" / "Tools" / "MSVC" / version / "bin" /
    // TODO Look up architecture!
    "Hostx64" / "x64" / "cl.exe";
  return cl;
}

auto find_vcvars_bat() -> fs::path {
  fs::path msvs{find_msvs()};
  auto aux = msvs / "VC" / "Auxiliary" / "Build";
  // std::cout << aux << std::endl;
  // See "https://superuser.com/questions/321988/".
  // TODO Just check for existence of two program files dirs?
  auto arch = Process{"wmic", "os", "get", "osarchitecture"}.check_output();
  arch = arch.find("64-bit") == std::string::npos ? "32" : "64";
  auto vcvars_bat = aux / (std::string{"vcvars"} + arch + ".bat");
  // std::cout << vcvars_bat << std::endl;
  return vcvars_bat;
}

auto make_cc_bat(const fs::path& work_dir) -> fs::path {
  // Going to parent path caches across all builds.
  // TODO Put under {rio-lang}/cc/{hash}.bat
  // TODO Use other info to know if the cache is stale:
  // TODO - msvs path
  // TODO - version from VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt
  // TODO - starting environment
  // TODO - rio config
  // TODO - hash all the above (after normalizing env sort order)
  // TODO - store vars there
  auto top_dir = work_dir.parent_path().parent_path();
  auto cc_bat = top_dir / "cc.bat";
  if (!fs::exists(cc_bat)) {
    // Need to rebuild it.
    auto vcvars_bat = find_vcvars_bat();
    auto finder = top_dir / "finder.bat";
    {
      std::ofstream finder_out{finder};
      // TODO What if there are quotes in the vcvars_bat?
      finder_out << "call \"" << vcvars_bat.string() << '"' << std::endl;
      finder_out << "set" << std::endl;
    }
    auto vcvars_str = Process{finder.string()}.check_output();
    std::stringstream vcvars_text;
    vcvars_text << vcvars_str;
    // Got the text. Write out a compile script.
    std::ofstream cc_out{cc_bat};
    for (std::string line; std::getline(vcvars_text, line);) {
      auto split = line.find('=');
      if (split == std::string::npos) continue;
      cc_out << "@set " << line << std::endl;
    }
    cc_out << "@cl %*" << std::endl;
  }
  return cc_bat;
}

auto compile_c(const fs::path& path, bool verbose = false) -> void {
  // See also:
  // https://blogs.msdn.microsoft.com/vcblog/2017/03/06/finding-the-visual-c-compiler-tools-in-visual-studio-2017/
  // https://blogs.msdn.microsoft.com/heaths/2017/03/11/vswhere-now-searches-older-versions-of-visual-studio/
  // HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7
  auto dir = path.parent_path();
  // auto cl = find_cl();
  auto cc = make_cc_bat(dir);
  Process compile{
    cc.string(), path.filename().string(),
    // "cl", path.filename().string(),
    // "cat", path.filename().string(),
    // "cmd", "/c", "echo %SYSTEMROOT%",
    // "echo"
  };
  compile.dir = dir;
  // compile.env = std::move(find_msvc_vars(dir));
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
