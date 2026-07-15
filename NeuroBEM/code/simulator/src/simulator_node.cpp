#include <string>
#include <vector>

#include "simulator.h"

/* Main function of the node. Depending on the number of arguments, one of the
 * constructors of the Simulator() class is called */
int main(int argc, char** argv) {
  if (argc == 3 || argc == 6) {
    std::string infile = argv[1];
    std::string outfile = argv[2];
    std::cout << "Converting " << infile << " to " << outfile << std::endl;
    if (argc == 6) {
      double aero[3] = {atof(argv[3]), atof(argv[4]), atof(argv[5])};
      Simulator sim(infile.c_str(), outfile.c_str(), aero);
    } else {
      Simulator sim(infile.c_str(), outfile.c_str());
    }
  } else {
    Simulator sim;
  }
  return 0;
}
