#include <string>
#include <vector>

#include "simulator.h"

/* Main function of the node. Depending on the number of arguments, one of the
 * constructors of the Simulator() class is called */
int main(int argc, char** argv) {
  if (argc == 3) {
    std::string infile = argv[1];
    std::string outfile = argv[2];
    std::cout << "Converting " << infile << " to " << outfile << std::endl;
    Simulator sim(infile.c_str(), outfile.c_str());
  } else {
    Simulator sim;
  }
  return 0;
}
