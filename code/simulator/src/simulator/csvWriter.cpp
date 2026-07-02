#include "csvWriter.h"

csvWriter::csvWriter(const char* filename,
                     const std::vector<std::string> header) {
  this->filename = filename;
  numCol = header.size();
  if (numCol <= 0) {
    printf("Invalid Number of Columns: %zu\n", numCol);
    abort();
  }
  csvFile = fopen(filename, "w");
  printf("Opened file %s\n", filename);
}

csvWriter::csvWriter(const char* filename, const size_t numCol) {
  this->filename = filename;
  if (numCol <= 0) {
    printf("Invalid Number of Columns: %zu\n", numCol);
    abort();
  }
  this->numCol = numCol;
  csvFile = fopen(filename, "w");
  printf("Opened file %s\n", filename);
}

csvWriter::~csvWriter() {}

bool csvWriter::add(const std::vector<double> data) {
  if (data.size() != numCol) {
    printf("Wrong number (%zu) of entries supplied. Must be %zu\n", data.size(),
           numCol);
    return false;
  }

  for (size_t i = 0; i < numCol - 1; ++i) {
    fprintf(csvFile, "%lf,", data[i]);
  }
  fprintf(csvFile, "%lf\n", data[numCol - 1]);

  return true;
}

void csvWriter::close() {
  printf("Closed file %s\n", filename);
  fclose(csvFile);
}
