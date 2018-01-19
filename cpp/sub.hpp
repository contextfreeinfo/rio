#pragma once

#include "dirs.hpp"
#include <future>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
// Seems slower: # define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/types.h>
# include <unistd.h>
#endif

namespace rio {

using StringMap = Map<std::string, std::string>;

#ifdef _WIN32

struct Pipe {

  HANDLE in;
  HANDLE out;

  Pipe() {
    SECURITY_ATTRIBUTES security;
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;
    security.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&in, &out, &security, 0)) {
      throw std::runtime_error("failed to open pipe");
    }
  }

  ~Pipe() {
    // Don't bother to check these for now.
    // Do they fail on double-close? Can we check if closed?
    CloseHandle(in);
    CloseHandle(out);
  }

};

struct Process {

  std::vector<std::string> args;

  fs::path dir;

  StringMap env;

  Process(const std::initializer_list<std::string>& args_): args(args_) {}

  auto check_output() -> std::string {
    // Kick off.
    start();
    // Read all into string.
    std::stringstream result; 
    constexpr size_t size = 8192;
    char buffer[size];
    DWORD count;
    while (true) {
      auto success = ReadFile(out.in, buffer, size, &count, NULL);
      if (!success) {
        if (GetLastError() != ERROR_BROKEN_PIPE) {
          throw std::runtime_error("failed to read from pipe");
        }
      }
      if (!count) {
        break;
      }
      result << std::string_view(buffer, count);
    }
    // Return result.
    return result.str();
  }

  private:

  Pipe in;
  Pipe out;

  auto make_command_line() -> std::wstring {
    // Build utf8 command line.
    std::string command_line;
    size_t index = 0;
    for (auto& arg: args) {
      if (index) {
        command_line += ' ';
      }
      std::string escaped;
      if (arg.find(' ') == std::string::npos) {
        // Without spaces, no need for quotes in Windows.
        // Backslashes are just backslashes.
        escaped = arg;
      } else {
        // Need to wrap in quotes.
        escaped += '"';
        // First see if there are quotes or backslashes already present.
        // This won't matter for file names, but I'm trying to be vaguely
        // general.
        if (arg.find_first_of("\\\"") == std::string::npos) {
          // Nope, so this is easy.
          escaped += arg;
        } else {
          // Yep, so escape by sane rules for now, although maybe should escape
          // by crazy CommandLineToArgvW rules instead, if that's what most
          // Windows software uses.
          for (auto c: arg) {
            if (c == '\\' || c == '"') {
              escaped += '\\';
            }
            escaped += c;
          }
        }
        escaped += '"';
      }
      command_line += escaped;
      index += 1;
    }
    // Convert to utf16.
    return u8_to_wstr(command_line);
  }

  auto start() -> void {
    // Prevent wrong inheritance of pipe ends.
    if (!SetHandleInformation(out.in, HANDLE_FLAG_INHERIT, 0)) {
      throw std::runtime_error("failed to configure pipe");
    }
    if (!SetHandleInformation(in.out, HANDLE_FLAG_INHERIT, 0)) {
      throw std::runtime_error("failed to configure pipe");
    }
    // Start child.
    start_child();
    // For the moment, we don't care about writing to the child.
    // Also close the inherited sides, or our reads won't terminate. See:
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365782.aspx
    in.~Pipe();
    CloseHandle(out.out);
  }

  auto start_child() -> void {
    // Prep startup.
    STARTUPINFOW startup_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFOW));
    startup_info.cb = sizeof(STARTUPINFOW);
    startup_info.hStdError = out.out;
    startup_info.hStdInput = in.in;
    startup_info.hStdOutput = out.out;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    // Command line.
    auto command_line = make_command_line();
    // Prep to receive process info.
    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
    // Environment.
    std::wstring wenv;
    if (!env.empty()) {
      // std::stringstream u8env;
      for (auto& [name, value]: env) {
        // u8env << name << "=" << value << '\0';
        // if (name != "PATH") continue;
        wenv += u8_to_wstr(name);
        // std::wcout << wenv << ": " << wenv.size() << std::endl;
        // for (auto c: wenv) {
        //   std::wcout << static_cast<int>(c) << std::endl;
        // }
        wenv += L'=';
        wenv += u8_to_wstr(value);
        // wenv += L"\r\n";
        wenv += L'\0';
        // std::cout << name << "=" << value << std::endl;
      }
      // wenv = std::move(u8_to_wstr(u8env.str()));
    }
    // std::wcout << wenv << std::endl;
    // std::cout << wenv.size() << std::endl;
    // wenv += L'\0';
    // std::wcout << wenv << std::endl;
    // std::cout << wenv.size() << std::endl;
    // Kick it off.
    auto wdir = u8_to_wstr(fs::absolute(dir).string());
    auto success = CreateProcessW(
      NULL, command_line.data(), NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT,
      wenv.empty() ? NULL : wenv.data(),
      // NULL,
      // LR"(PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.12.25827\bin\HostX64\x64)" L"\0",
      wdir.empty() ? NULL : wdir.data(),
      &startup_info, &process_info
    );
    if (!success) {
      std::cout << GetLastError() << std::endl;
      throw std::runtime_error("failed to exec child");
    }
    // We're only monitoring by pipes, so we don't need these.
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
  }

};

#else // Posix

struct Pipe {

  Pipe() {
    if (pipe(fds)) {
      throw std::runtime_error("failed to open pipe");
    }
  }

  ~Pipe() {
    close(fds[0]);
    close(fds[1]);
  }

  auto in() -> int {
    return fds[0];
  }

  auto out() -> int {
    return fds[1];
  }

  private:

  int fds[2];

};

struct Process {

  std::vector<std::string> args;

  fs::path dir;

  Process(const std::initializer_list<std::string>& args_): args(args_) {}

  auto check_output() -> std::string {
    // Start child.
    start();
    // Prep read.
    auto read_all = [](int fd) -> std::string {
      std::stringstream result; 
      constexpr size_t size = 8192;
      char buffer[size];
      while (true) {
        auto count = read(fd, buffer, size);
        if (count < 0) {
          throw std::runtime_error("failed to read from pipe");
        }
        if (!count) {
          break;
        }
        result << std::string_view(buffer, count);
      }
      return result.str();
    };
    // Read from err and out in parallel, so we don't hang buffers.
    auto err_future = std::async(std::launch::async, [&]{
      return read_all(this->err.in());
    });
    auto out_future = std::async(std::launch::async, [&]{
      return read_all(this->out.in());
    });
    // Just join together for now.
    // Better might be to interleave, but I'd have to coordinate across threads
    // for that or something.
    // TODO Check return code!
    return out_future.get() + err_future.get();
  }

  private:

  Pipe err;
  Pipe in;
  Pipe out;

  auto start() -> void {
    auto pid = fork();
    if (!pid) {
      run_child();
    }
    // Parent.
    // Close pipe ends that we don't need.
    close(err.out());
    close(in.in());
    close(out.out());
    // And for the moment, we don't care about writing to the child, either.
    close(in.out());
  }

  auto run_child() -> void {
    if (
      dup2(err.out(), STDERR_FILENO) == -1 ||
      dup2(in.in(), STDIN_FILENO) == -1 ||
      dup2(out.out(), STDOUT_FILENO) == -1
    ) {
      throw std::runtime_error("failed to dup pipes in child");
    }
    // Close now, so destruct early.
    // TODO Reorg such that this isn't needed?
    err.~Pipe();
    in.~Pipe();
    out.~Pipe();
    // Change dir.
    if (!dir.empty()) {
      if (chdir(dir.string().data())) {
        throw std::runtime_error("failed to chdir");
      }
    }
    // Prep args for exec.
    std::vector<char*> exec_args;
    for (auto& arg: args) {
      exec_args.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_args.push_back(nullptr);
    // Exec.
    if (execvp(exec_args[0], exec_args.data()) == -1) {
      throw std::runtime_error("failed to exec child");
    }
  }

};

#endif // Posix

}
