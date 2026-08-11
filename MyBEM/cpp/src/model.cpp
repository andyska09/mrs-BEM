#include "mybem/model.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "mybem/yaml.h"

namespace mybem {
namespace {

[[noreturn]] void fail(const std::string& msg) {
  printf("model: %s\n", msg.c_str());
  exit(1);
}

uint64_t fnv1a(const std::string& s) {
  uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}

std::string fileId(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return "missing";
  std::ostringstream ss;
  ss << in.rdbuf();
  char buf[24];
  snprintf(buf, sizeof(buf), "%016llx",
           static_cast<unsigned long long>(fnv1a(ss.str())));
  return buf;
}

bool asDouble(const std::string& s, double& out) {
  try {
    size_t used = 0;
    out = std::stod(s, &used);
    return used == s.size();
  } catch (...) {
    return false;
  }
}

Params defaultsOf(const std::vector<TunableParam>& t) {
  Params p;
  for (const TunableParam& e : t) p[e.key] = e.def;
  return p;
}

}  // namespace

Model Model::load(const std::string& path) {
  const YamlDoc doc = readYaml(path);
  Model m;
  m.name_ = doc.get("name");
  m.drone_ = doc.get("drone");

  auto af = doc.sections.find("airframe");
  if (af != doc.sections.end()) {
    Params p = m.airframe_.params();
    for (const auto& kv : af->second) {
      double v;
      if (!asDouble(kv.second, v))
        fail("airframe." + kv.first + ": expected a number");
      if (!p.count(kv.first)) fail("unknown key airframe." + kv.first);
      p[kv.first] = v;
    }
    m.airframe_.load(p);
  }

  if (doc.models.empty()) fail("no 'models:' list in " + path);

  std::map<std::string, int> seen;
  for (const Leaf& item : doc.models) {
    auto type = item.find("type");
    if (type == item.end()) fail("a models entry has no 'type'");

    ComponentPtr c = createComponent(type->second);
    if (!c) fail("unknown component type '" + type->second + "'");

    Params p = defaultsOf(c->tunables());
    Options o = c->options();

    for (const auto& kv : item) {
      if (kv.first == "type") continue;
      double v;
      if (asDouble(kv.second, v)) {
        if (!p.count(kv.first))
          fail("unknown key " + type->second + "." + kv.first);
        p[kv.first] = v;
      } else {
        if (!o.count(kv.first))
          fail("unknown option " + type->second + "." + kv.first);
        o[kv.first] = kv.second;
      }
    }

    for (const std::string& key : c->pathOptions()) {
      auto opt = o.find(key);
      if (opt == o.end() || opt->second.empty()) continue;
      std::filesystem::path v(opt->second);
      if (v.is_relative()) v = std::filesystem::path(path).parent_path() / v;
      opt->second = std::filesystem::weakly_canonical(v).string();
    }
    c->load(p, o);

    const int n = seen[type->second]++;
    m.prefixes_.push_back(n == 0 ? type->second
                                 : type->second + "_" + std::to_string(n + 1));
    m.components_.push_back(std::move(c));
  }
  return m;
}

std::string Model::render(bool content_ids) const {
  YamlWriter w;
  if (!name_.empty()) w.scalar("name", name_);
  if (!drone_.empty()) w.scalar("drone", drone_);

  w.beginList("models");
  for (const ComponentPtr& c : components_) {
    Leaf item;
    item["type"] = c->type();
    for (const auto& kv : c->params()) item[kv.first] = fmt(kv.second);
    Options o = c->options();
    if (content_ids)
      for (const std::string& key : c->pathOptions()) {
        auto opt = o.find(key);
        if (opt != o.end()) opt->second = fileId(opt->second);
      }
    for (const auto& kv : o) item[kv.first] = kv.second;
    w.listItem(item, "type");
  }

  Leaf af;
  for (const auto& kv : airframe_.params()) af[kv.first] = fmt(kv.second);
  w.section("airframe", af);
  return w.str();
}

std::string Model::text() const { return render(false); }

void Model::save(const std::string& path) const {
  std::ofstream out(path);
  if (!out) {
    printf("Cannot write %s\n", path.c_str());
    return;
  }
  out << text();
}

std::string Model::hash() const {
  const uint64_t h = fnv1a(render(true));
  char buf[8];
  snprintf(buf, sizeof(buf), "%06llx",
           static_cast<unsigned long long>(h & 0xffffff));
  return buf;
}

Wrench Model::evaluate(const State& s) {
  Wrench w;
  for (const ComponentPtr& c : components_) c->add(s, airframe_, w);
  return w;
}

std::vector<TunableParam> Model::tunables() const {
  std::vector<TunableParam> out;
  for (size_t i = 0; i < components_.size(); ++i)
    for (TunableParam t : components_[i]->tunables()) {
      t.key = prefixes_[i] + "." + t.key;
      out.push_back(t);
    }
  for (TunableParam t : airframe_.tunables()) {
    t.key = "airframe." + t.key;
    out.push_back(t);
  }
  return out;
}

Params Model::values() const {
  Params out;
  for (size_t i = 0; i < components_.size(); ++i)
    for (const auto& kv : components_[i]->params())
      out[prefixes_[i] + "." + kv.first] = kv.second;
  for (const auto& kv : airframe_.params()) out["airframe." + kv.first] = kv.second;
  return out;
}

void Model::set(const Params& overrides) {
  for (size_t i = 0; i < components_.size(); ++i) {
    const std::string pre = prefixes_[i] + ".";
    Params p = components_[i]->params();
    bool touched = false;
    for (const auto& kv : overrides)
      if (kv.first.rfind(pre, 0) == 0) {
        const std::string key = kv.first.substr(pre.size());
        if (!p.count(key)) fail("unknown tunable " + kv.first);
        p[key] = kv.second;
        touched = true;
      }
    if (touched) components_[i]->load(p, components_[i]->options());
  }

  Params a = airframe_.params();
  for (const auto& kv : overrides)
    if (kv.first.rfind("airframe.", 0) == 0) {
      const std::string key = kv.first.substr(9);
      if (!a.count(key)) fail("unknown tunable " + kv.first);
      a[key] = kv.second;
    }
  airframe_.load(a);
}

}  // namespace mybem
