#pragma once

#include <map>
#include <string>
#include <vector>

namespace mybem {

using Leaf = std::map<std::string, std::string>;

/* Parsed model YAML. Deliberately covers only the schema MyBEM emits:
 * top-level scalars, one `models:` list of maps, and named sub-maps. Anything
 * else is an error rather than a silent skip. */
struct YamlDoc {
  Leaf root;
  std::map<std::string, Leaf> sections;
  std::vector<Leaf> models;

  std::string get(const std::string& key, const std::string& def = "") const;
};

YamlDoc readYaml(const std::string& path);

/* Emits the schema readYaml accepts, so a load/save round-trip is lossless. */
class YamlWriter {
 public:
  explicit YamlWriter(const std::string& path);
  ~YamlWriter();

  void scalar(const std::string& key, const std::string& value);
  void section(const std::string& key, const Leaf& values);
  void beginList(const std::string& key);
  void listItem(const Leaf& values, const std::string& first_key);

 private:
  std::string path_;
  std::string buf_;
};

std::string fmt(double v);

}  // namespace mybem
