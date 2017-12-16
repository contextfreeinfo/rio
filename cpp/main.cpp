#include "resolve.hpp"
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
    // Read source.
    std::ifstream file{argv[1]};
    std::stringstream buffer;
    buffer << file.rdbuf();
    // Parse.
    std::string content = buffer.str();
    auto root = rio::parse(content);
    // Write tree.
    std::cout << root.format() << std::endl;
  } catch (std::exception& error) {
    std::cout << "Error: " << error.what() << std::endl;
  }
}
