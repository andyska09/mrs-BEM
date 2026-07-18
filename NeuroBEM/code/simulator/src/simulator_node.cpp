#include <map>
#include <string>

#include "config.h"
#include "simulator.h"

/* Main function of the node. Depending on the number of arguments, one of the
 * constructors of the Simulator() class is called */
int main(int argc, char** argv) {
  if (argc == 3 || argc == 4) {
    std::string infile = argv[1];
    std::string outfile = argv[2];
    std::cout << "Converting " << infile << " to " << outfile << std::endl;
    std::map<std::string, double> config;
    if (argc == 4) config = loadConfig(argv[3]);
    Simulator sim(infile.c_str(), outfile.c_str(), config);
  } else {
    Simulator sim;
  }
  return 0;
}
