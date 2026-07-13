#include "csvReader.h"

csvReader::csvReader(const std::string filename) {
  this->filename = filename;

  std::string line;
  std::ifstream csvFile(filename);
  if (csvFile.is_open()) {
    std::getline(csvFile, header);
    colCount = std::count(header.begin(), header.end(), ',') + 1;
    while (std::getline(csvFile, line)) ++rowCount;
    printf("Opened file %s\n", filename.c_str());
  } else {
    printf("File %s could not be read\n", filename.c_str());
    abort();
    return;
  }
  csvFile.close();

  // Now open the file to actually read the values. Note the C++ way with
  // ifstream is faster for counting lines, but less ideal for reading values.
  data = new double[rowCount * colCount];
  int res;
  FILE* file;
  if ((file = fopen(filename.c_str(), "r"))) {
    res = fscanf(file, "%*[^\n]\n");  // Discard first line
    for (size_t i = 0; i < rowCount; ++i) {
      for (size_t j = 0; j < colCount - 1; ++j) {
        res = fscanf(file, "%lf,", &data[i * colCount + j]);
        if (res != 1) printf("CSV parsing error occured\n");
      }
      res = std::fscanf(file, "%lf", &data[(i + 1) * colCount - 1]);
      if (res != 1) printf("CSV parsing error occured\n");
    }
  }
  printf("Closed file %s\n", filename.c_str());
}

csvReader::~csvReader() { delete[] data; }

bool csvReader::getLine(std::vector<double>* line) {
  bool res = getLine(line, current);
  if (res) ++current;
  return res;
}

bool csvReader::getLine(std::vector<double>* line, const size_t num) {
  if (num > rowCount - 1) return false;
  if (line->size() < colCount) line->resize(colCount);
  memcpy(line->data(), &data[num * colCount], colCount * sizeof(double));
  return true;
}

size_t csvReader::getNumberOfRows() { return rowCount; }

size_t csvReader::getNumberOfColumns() { return colCount; }
