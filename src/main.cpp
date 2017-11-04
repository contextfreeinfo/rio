#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "tokenizer.h"

namespace rio {

void process(int argc, char** argv) {
  if (argc < 2) {
    throw std::runtime_error("source name required");
  }
  std::ifstream file{argv[1]};
  std::stringstream buffer;
  buffer << file.rdbuf();
  Tokenizer tokenizer;
  tokenizer.tokenize(buffer.str());
}

}

int main(int argc, char** argv) {
  try {
    std::cout << "Hello, world!" << std::endl;
    rio::process(argc, argv);
  }
  catch (std::exception& error) {
    std::cout << error.what() << std::endl;
  }
}
