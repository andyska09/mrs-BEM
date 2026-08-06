#include "mybem/csv.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace mybem {

CsvReader::CsvReader(const std::string& path) {
  std::ifstream probe(path);
  if (!probe) {
    printf("Cannot read %s\n", path.c_str());
    exit(1);
  }
  std::string header, line;
  std::getline(probe, header);
  cols_ = std::count(header.begin(), header.end(), ',') + 1;
  while (std::getline(probe, line))
    if (!line.empty()) ++rows_;
  probe.close();

  data_ = new double[rows_ * cols_];
  FILE* file = fopen(path.c_str(), "r");
  if (!file) {
    printf("Cannot read %s\n", path.c_str());
    exit(1);
  }
  if (fscanf(file, "%*[^\n]\n") < 0) {
    printf("%s: empty file\n", path.c_str());
    exit(1);
  }
  for (size_t i = 0; i < rows_; ++i)
    for (size_t j = 0; j < cols_; ++j)
      if (fscanf(file, j + 1 < cols_ ? "%lf," : "%lf",
                 &data_[i * cols_ + j]) != 1) {
        printf("%s: parse error at row %zu, column %zu\n", path.c_str(), i + 1,
               j + 1);
        exit(1);
      }
  fclose(file);
}

CsvReader::~CsvReader() { delete[] data_; }

CsvWriter::CsvWriter(const std::string& path, size_t cols) : cols_(cols) {
  file_ = fopen(path.c_str(), "w");
  if (!file_) {
    printf("Cannot write %s\n", path.c_str());
    exit(1);
  }
}

CsvWriter::~CsvWriter() {
  if (file_) fclose(file_);
}

void CsvWriter::add(const double* values) {
  for (size_t i = 0; i + 1 < cols_; ++i) fprintf(file_, "%.12g,", values[i]);
  fprintf(file_, "%.12g\n", values[cols_ - 1]);
}

}  // namespace mybem
