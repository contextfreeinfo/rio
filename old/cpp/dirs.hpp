#pragma once

#include "util.hpp"
#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
// Seems slower: # define WIN32_LEAN_AND_MEAN
# include <shlobj.h>
# include <windows.h>
// #elif __APPLE__ // TODO What for Apple?
#else
  // Presume all else are xdg compliant for now.
#endif

namespace fs = std::experimental::filesystem;

namespace rio {

#ifdef _WIN32

auto u8_to_wstr(std::string_view u8) -> std::wstring {
  // Convert to utf16.
  std::wstring wstr;
  // Find utf8 size.
  // TODO Use std c++ functions? Endianness for Windows?
  auto size = MultiByteToWideChar(CP_UTF8, 0, u8.data(), -1, NULL, 0);
  if (!size) {
    throw std::runtime_error("bad utf16 size");
  }
  // Make the size large enough, not just the reserved space.
  // C++11 and later require implicit null char after string content, so this
  // is safe.
  // Go -1 because they count the null char, which we get following.
  wstr.assign(static_cast<size_t>(size - 1), ' ');
  auto new_size = MultiByteToWideChar(
    CP_UTF8, 0, u8.data(), -1, wstr.data(), size
  );
  if (new_size != size) {
    throw std::runtime_error("bad utf16 size");
  }
  return wstr;
}

auto wstr_to_u8(PWSTR wide, bool free = false) -> std::string {
  // Convert to utf8.
  std::string u8;
  if (!wide) {
    // Treat null as empty.
    return u8;
  }
  // Find utf8 size.
  // TODO Use std c++ functions?
  auto size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, 0, 0, NULL, NULL);
  if (!size) {
    if (free) CoTaskMemFree(wide);
    throw std::runtime_error("bad utf8 size");
  }
  // Make the size large enough, not just the reserved space.
  // C++11 and later require implicit null char after string content, so this
  // is safe.
  u8.assign(static_cast<size_t>(size), ' ');
  auto new_size =
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, u8.data(), size, NULL, NULL);
  // Free if requested before leaving.
  if (free) CoTaskMemFree(wide);
  // And still fail if things died.
  if (new_size != size) {
    throw std::runtime_error("bad utf8 size");
  }
  return u8;
}

auto get_cache_dir() -> fs::path {
  // Get the path as wstr.
  PWSTR wide;
  auto result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &wide);
  if (result != S_OK) {
    CoTaskMemFree(wide);
    throw std::runtime_error("no cache dir");
  }
  return wstr_to_u8(wide, true);
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
