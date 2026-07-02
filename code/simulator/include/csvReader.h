#pragma once

#include <stdio.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "timeit.h"

/* The csvReader class is a simple class that provides read-in functionalities
 * for csv files. The first line is interpreted as a header and therefore always
 * discarded.
 * The contents of the csv can be accessed sequentially by calling getLine
 * without a line number or by calling a specific line number. Furthermore, the
 * class provides access to the number of columns and rows.
 */
class csvReader {
 public:
  csvReader(const std::string filename);
  ~csvReader();
  void reset() { current = 0; };
  bool getLine(std::vector<double>* line);
  bool getLine(std::vector<double>*, const size_t num);
  size_t getNumberOfRows();
  size_t getNumberOfColumns();

 private:
  std::string filename;
  double* data = nullptr;
  size_t rowCount = 0;
  size_t colCount = 0;
  size_t current = 0;
  std::string header;
};
