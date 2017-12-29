#include "args.hxx"
#include "rio.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main(int argc, char** argv) {
  args::ArgumentParser parser{"Rio language compiler and executor."};
  args::HelpFlag help{parser, "help", "Show help", {'h', "help"}};
  // TODO Show std tree arg.
  args::Flag show_tree{parser, "tree", "Show parse trees", {"tree"}};
  args::Flag verbose{
    parser, "verbose", "Show additional info", {'v', "verbose"},
  };
  args::Positional<std::string> script_name{
    parser, "MAIN", "The path to the main script", args::Options::Required,
  };
  try {
    parser.ParseCLI(argc, argv);
    // Read source.
    std::ifstream file{args::get(script_name)};
    std::stringstream buffer;
    buffer << file.rdbuf();
    // Parse.
    std::string content = buffer.str();
    auto std_script = rio::std_init_c();
    rio::Context std_context{std_script.root};
    rio::Script main{content, &std_context};
    // Diagnostics.
    if (show_tree) {
      if (verbose) {
        std::cout << "--- std ---" << std::endl;
        std::cout << std_script.root.format() << std::endl;
        std::cout << std::endl;
      }
      std::cout << "--- main ---" << std::endl;
      std::cout << main.root.format() << std::endl;
    }
    // Generate.
    rio::CGenerator generator{std::cout};
    generator.generate(main);
  } catch (args::Help&) {
    std::cout << parser;
  } catch (args::Error& error) {
    std::cout << error.what() << std::endl;
    std::cout << parser;
    return 1;
  } catch (std::exception& error) {
    std::cout << "Error: " << error.what() << std::endl;
    return 1;
  }
}
