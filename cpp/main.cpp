#include "tokenizer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc, char** argv) {
  // TODO Tokenize something.
  try {
    if (argc < 2) {
      throw std::runtime_error("need 2 args");
    }
    std::ifstream file{argv[1]};
    std::stringstream buffer;
    buffer << file.rdbuf();
    // Tokenize.
    std::string content = buffer.str();
    rio::Tokenizer tokenizer{content};
    auto tokens = tokenizer.collect();
    for (auto& token: tokens) {
      std::cout << token << std::endl;
    }
  } catch (std::exception& error) {
    std::cout << "Error: " << error.what() << std::endl;
  }
}
