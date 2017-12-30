#pragma once

#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
  #include <shlobj.h>
  // #include <windows.h>
// #elif __APPLE__ // TODO What for Apple?
#else
  // Presume all else are xdg compliant for now.
#endif

namespace fs = std::experimental::filesystem;

namespace rio {

#ifdef _WIN32

  auto get_cache_dir() -> fs::path {
    // Get the path as wstr.
    PWSTR wide;
    auto result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &wide);
    if (result != S_OK) {
      CoTaskMemFree(wide);
      throw std::runtime_error("no cache dir");
    }
    // Find utf8 size.
    // TODO Use std c++ functions?
    auto size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, NULL, NULL);
    if (!size) {
      CoTaskMemFree(wide);
      throw std::runtime_error("bad utf8 size");
    }
    // Convert to utf8, freeing wide after.
    std::string bytes;
    // Make the size large enough, not just the reserved space.
    // C++11 and later require implicit null char after string content, so this
    // is safe.
    bytes.assign(static_cast<size_t>(size), ' ');
    auto new_size =
      WideCharToMultiByte(CP_UTF8, 0, wide, -1, bytes.data(), size, NULL, NULL);
    // Free now that we're done.
    CoTaskMemFree(wide);
    // And still fail if things died.
    if (new_size != size) {
      throw std::runtime_error("bad utf8 size");
    }
    // Good to go.
    return bytes;
  }

// #elif __APPLE__ // TODO What for Apple?

#else

  // Presume all else are xdg compliant for now.
  auto get_cache_dir() -> fs::path {
    fs::path dir;
    auto value = std::getenv("XDG_CACHE_HOME");
    if (value) {
      dir = value;
    } else {
      value = std::getenv("HOME");
      if (!value) throw std::runtime_error("no cache dir");
      dir = value;
      dir /= ".cache";
    }
    return dir;
  }

#endif

}
