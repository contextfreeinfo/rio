#pragma once

#include <future>
#include <initializer_list>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>
#include <unistd.h>

namespace rio {

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
    // TODO Check return code!
    return out_future.get() + err_future.get();
  }

  private:

  Pipe err;
  Pipe in;
  Pipe out;

  auto start() -> void {
    pid_t pid = fork();
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

}
