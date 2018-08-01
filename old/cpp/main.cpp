#include "args.hxx"
#include "rio.hpp"
#include <stdexcept>

int main(int argc, char** argv) {
  args::ArgumentParser parser{"Rio language compiler and executor."};
  // TODO Instead some kind of mode thing?
  args::Flag generate{
    parser, "generate", "Generate without building", {"generate"},
  };
  args::HelpFlag help{parser, "help", "Show help", {'h', "help"}};
  args::Flag show_tree{parser, "tree", "Show parse trees", {"tree"}};
  args::Flag verbose{
    parser, "verbose", "Show additional info", {'v', "verbose"},
  };
  // TODO We need those after the main script name to be args to the script,
  // TODO when in run mode.
  // TODO Could just require "--", maybe even recommended before main name.
  // TODO But sort of uncool.
  args::Positional<std::string> script_name{
    parser, "MAIN", "The path to the main script", args::Options::Required,
  };
  try {
    parser.ParseCLI(argc, argv);
    rio::Session session;
    session.generate_only = args::get(generate);
    session.main_path = args::get(script_name);
    session.show_tree = args::get(show_tree);
    session.verbose = args::get(verbose);
    session.run();
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
