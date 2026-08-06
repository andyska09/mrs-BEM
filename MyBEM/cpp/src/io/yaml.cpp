#include "mybem/yaml.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace mybem {
namespace {

[[noreturn]] void fail(const std::string& path, size_t line,
                       const std::string& msg) {
  printf("%s:%zu: %s\n", path.c_str(), line, msg.c_str());
  exit(1);
}

std::string trim(const std::string& s) {
  const size_t a = s.find_first_not_of(" \t\r");
  if (a == std::string::npos) return "";
  return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

size_t indentOf(const std::string& s) {
  return s.find_first_not_of(' ') == std::string::npos
             ? 0
             : s.find_first_not_of(' ');
}

}  // namespace

std::string YamlDoc::get(const std::string& key, const std::string& def) const {
  auto it = root.find(key);
  return it == root.end() ? def : it->second;
}

YamlDoc readYaml(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    printf("Cannot open %s\n", path.c_str());
    exit(1);
  }

  YamlDoc doc;
  enum { ROOT, SECTION, LIST } mode = ROOT;
  std::string section_name;
  std::string raw;
  size_t lineno = 0;

  while (std::getline(file, raw)) {
    ++lineno;
    const size_t hash = raw.find('#');
    if (hash != std::string::npos) raw = raw.substr(0, hash);
    if (trim(raw).empty()) continue;

    const size_t indent = indentOf(raw);
    std::string line = trim(raw);
    const bool item = line.rfind("- ", 0) == 0;
    if (item) line = trim(line.substr(2));

    if (indent == 0 && !item) {
      const size_t colon = line.find(':');
      if (colon == std::string::npos) fail(path, lineno, "expected 'key:'");
      const std::string key = trim(line.substr(0, colon));
      const std::string value = trim(line.substr(colon + 1));
      if (!value.empty()) {
        doc.root[key] = value;
        mode = ROOT;
      } else if (key == "models") {
        mode = LIST;
      } else {
        mode = SECTION;
        section_name = key;
        doc.sections[key];
      }
      continue;
    }

    const size_t colon = line.find(':');
    if (colon == std::string::npos) fail(path, lineno, "expected 'key: value'");
    const std::string key = trim(line.substr(0, colon));
    const std::string value = trim(line.substr(colon + 1));

    if (mode == LIST) {
      if (item) doc.models.emplace_back();
      if (doc.models.empty()) fail(path, lineno, "list entry before '- '");
      doc.models.back()[key] = value;
    } else if (mode == SECTION) {
      doc.sections[section_name][key] = value;
    } else {
      fail(path, lineno, "indented entry outside a section");
    }
  }
  return doc;
}

std::string fmt(double v) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.12g", v);
  return buf;
}

YamlWriter::YamlWriter(const std::string& path) : path_(path) {}

YamlWriter::~YamlWriter() {
  std::ofstream out(path_);
  if (!out) {
    printf("Cannot write %s\n", path_.c_str());
    return;
  }
  out << buf_;
}

void YamlWriter::scalar(const std::string& key, const std::string& value) {
  buf_ += key + ": " + value + "\n";
}

void YamlWriter::section(const std::string& key, const Leaf& values) {
  buf_ += key + ":\n";
  for (const auto& kv : values) buf_ += "  " + kv.first + ": " + kv.second + "\n";
}

void YamlWriter::beginList(const std::string& key) { buf_ += key + ":\n"; }

void YamlWriter::listItem(const Leaf& values, const std::string& first_key) {
  auto first = values.find(first_key);
  if (first == values.end()) return;
  buf_ += "  - " + first_key + ": " + first->second + "\n";
  for (const auto& kv : values)
    if (kv.first != first_key)
      buf_ += "    " + kv.first + ": " + kv.second + "\n";
}

}  // namespace mybem
