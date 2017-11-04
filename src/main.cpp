#include <iostream>
#include "tokenizer.h"

namespace rio {

void process(int argc, char** argv) {
  Tokenizer tokenizer;
  tokenizer.tokenize("Hi!");
}

}

int main(int argc, char** argv) {
  rio::process(argc, argv);
  std::cout << "Hello, world!" << std::endl;
}
