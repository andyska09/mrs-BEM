#pragma once

#include <stdio.h>

#include <string>
#include <vector>

/* Simple class that allows to easily write csv files. When opening the file,
 * either a header or the number of columns must be known.
 * To write a line, use the add() function.
 */
class csvWriter {
 public:
  csvWriter(const char* filename, const std::vector<std::string> header);
  csvWriter(const char* filename, const size_t numCol);
  ~csvWriter();
  bool add(const std::vector<double> data);
  void close();

 private:
  FILE* csvFile;
  size_t numCol = 0;
  size_t numRow = 0;
  const char* filename;
};
