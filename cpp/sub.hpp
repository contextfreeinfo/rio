#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>
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
    run();
    return "";
  }

  private:

  Pipe err;
  Pipe in;
  Pipe out;

  auto run() -> void {
    pid_t pid = fork();
    if (!pid) {
      run_child();
    }
  }

  auto run_child() -> void {
    //
  }

};

}
