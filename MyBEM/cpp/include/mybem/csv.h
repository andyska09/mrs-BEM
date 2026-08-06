#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace mybem {

/* Reads a numeric CSV with a single header line into one contiguous block. */
class CsvReader {
 public:
  explicit CsvReader(const std::string& path);
  ~CsvReader();

  size_t rows() const { return rows_; }
  size_t cols() const { return cols_; }
  const double* row(size_t i) const { return data_ + i * cols_; }

 private:
  double* data_ = nullptr;
  size_t rows_ = 0;
  size_t cols_ = 0;
};

/* %.12g, not the original's %lf: 6 decimals leaves ~3 significant digits on a
 * 1e-3 Nm torque. */
class CsvWriter {
 public:
  CsvWriter(const std::string& path, size_t cols);
  ~CsvWriter();
  void add(const double* values);

 private:
  FILE* file_ = nullptr;
  size_t cols_;
};

}  // namespace mybem
