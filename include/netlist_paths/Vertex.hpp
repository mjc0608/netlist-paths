#ifndef NETLIST_PATHS_GRAPH_HPP
#define NETLIST_PATHS_GRAPH_HPP

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "netlist_paths/Exception.hpp"

namespace netlist_paths {

enum class VertexType {
  LOGIC,
  ASSIGN,
  ASSIGN_ALIAS,
  ASSIGN_DLY,
  ASSIGN_W,
  ALWAYS,
  INITIAL,
  REG_SRC,
  REG_DST,
  SEN_GATE,
  SEN_ITEM,
  VAR,
  WIRE,
  PORT,
  C_FUNC,
  INVALID
};

enum class VertexDirection {
  NONE,
  INPUT,
  OUTPUT,
  INOUT
};

class File {
  std::string filename;
  std::string language;
public:
  File(const std::string &filename,
       const std::string &language) :
      filename(filename), language(language) {}
  const std::string &getFilename() { return filename; }
};

class Location {
  std::shared_ptr<File> file;
  unsigned startLine;
  unsigned startCol;
  unsigned endLine;
  unsigned endCol;
public:
  Location() : file(nullptr) {}
  Location(std::shared_ptr<File> file,
           unsigned startLine,
           unsigned startCol,
           unsigned endLine,
           unsigned endCol) :
      file(file),
      startLine(startLine),
      endLine(endLine),
      endCol(endCol) {}
  const std::string getFilename() const { return file->getFilename(); }
};

class DType {
public:
  DType() {}
};

struct VertexProperties {
  unsigned long long id;
  VertexType type;
  VertexDirection direction;
  Location location;
  std::shared_ptr<DType> dtype;
  std::string name;
  bool isParam;
  std::string paramValue;
  bool isTop;
  bool deleted;
};

// Vertex helper fuctions.

inline bool determineIsTop(const std::string &name) {
  // Check there are no Vlvbound nodes.
  assert(name.find("__Vlvbound") == std::string::npos);
  // module.name or name is top level, but module.submodule.name is not.
  std::vector<std::string> tokens;
  boost::split(tokens, name, boost::is_any_of("."));
  return tokens.size() < 3;
}

inline std::string expandName(const std::string &topName,
                              const std::string &name) {
  // Add prefix to top-level names that are missing it.
  if (name.rfind(topName + ".", 0) == std::string::npos) {
    return std::string() + topName + "." + name;
  }
  return name;
}

inline bool isSrcReg(const VertexProperties &p) {
  return !p.deleted &&
         p.type == VertexType::REG_SRC;
}

inline bool isLogic(const VertexProperties &p) {
  return p.type == VertexType::LOGIC ||
         p.type == VertexType::ASSIGN ||
         p.type == VertexType::ASSIGN_ALIAS ||
         p.type == VertexType::ASSIGN_DLY ||
         p.type == VertexType::ASSIGN_W ||
         p.type == VertexType::ALWAYS ||
         p.type == VertexType::INITIAL ||
         p.type == VertexType::SEN_GATE ||
         p.type == VertexType::SEN_ITEM;
}

inline bool isReg(const VertexProperties &p) {
  return !p.deleted &&
         (p.type == VertexType::REG_DST ||
          p.type == VertexType::REG_SRC);
}

inline bool isStartPoint(const VertexProperties &p) {
  return !p.deleted &&
         (p.type == VertexType::REG_SRC ||
          (p.direction == VertexDirection::INPUT && p.isTop) ||
          (p.direction == VertexDirection::INOUT && p.isTop));
}

inline bool isEndPoint(const VertexProperties &p) {
  return !p.deleted &&
         (p.type == VertexType::REG_DST ||
          (p.direction == VertexDirection::OUTPUT && p.isTop) ||
          (p.direction == VertexDirection::INOUT && p.isTop));
}

inline bool isMidPoint(const VertexProperties &p) {
  return !p.deleted &&
         (p.type == VertexType::VAR ||
          p.type == VertexType::WIRE ||
          p.type == VertexType::PORT);
}

inline bool canIgnore(const VertexProperties &p) {
  // Ignore variables Verilator has introduced.
  return p.name.find("__Vdly") != std::string::npos ||
         p.name.find("__Vcell") != std::string::npos ||
         p.name.find("__Vconc") != std::string::npos;
}

inline VertexType getVertexType(const std::string &name) {
  static std::map<std::string, VertexType> mappings {
      { "LOGIC",        VertexType::LOGIC },
      { "ASSIGN",       VertexType::ASSIGN },
      { "ASSIGN_ALIAS", VertexType::ASSIGN_ALIAS },
      { "ASSIGN_DLY",   VertexType::ASSIGN_DLY },
      { "ASSIGN_W",     VertexType::ASSIGN_W },
      { "ALWAYS",       VertexType::ALWAYS },
      { "INITIAL",      VertexType::INITIAL },
      { "REG_SRC",      VertexType::REG_SRC },
      { "REG_DST",      VertexType::REG_DST },
      { "SEN_GATE",     VertexType::SEN_GATE },
      { "SEN_ITEM",     VertexType::SEN_ITEM },
      { "VAR",          VertexType::VAR },
      { "WIRE",         VertexType::WIRE },
      { "PORT",         VertexType::PORT },
      { "C_FUNC",       VertexType::C_FUNC },
  };
  auto it = mappings.find(name);
  return (it != mappings.end()) ? it->second : VertexType::INVALID;
}

inline const char *getVertexTypeStr(VertexType type) {
  switch (type) {
    case VertexType::LOGIC:        return "LOGIC";
    case VertexType::ASSIGN:       return "ASSIGN";
    case VertexType::ASSIGN_ALIAS: return "ASSIGN_ALIAS";
    case VertexType::ASSIGN_DLY:   return "ASSIGN_DLY";
    case VertexType::ASSIGN_W:     return "ASSIGN_W";
    case VertexType::ALWAYS:       return "ALWAYS";
    case VertexType::INITIAL:      return "INITIAL";
    case VertexType::REG_SRC:      return "REG_SRC";
    case VertexType::REG_DST:      return "REG_DST";
    case VertexType::SEN_GATE:     return "SEN_GATE";
    case VertexType::SEN_ITEM:     return "SEN_ITEM";
    case VertexType::VAR:          return "VAR";
    case VertexType::WIRE:         return "WIRE";
    case VertexType::PORT:         return "PORT";
    case VertexType::C_FUNC:       return "C_FUNC";
    case VertexType::INVALID:      return "INVALID";
    default:                       return "UNKNOWN";
  }
}

inline VertexDirection getVertexDirection(const std::string &direction) {
  static std::map<std::string, VertexDirection> mappings {
      { "input",  VertexDirection::INPUT },
      { "output", VertexDirection::OUTPUT },
      { "inout",  VertexDirection::INOUT },
  };
  auto it = mappings.find(direction);
  return (it != mappings.end()) ? it->second : VertexDirection::NONE;
}

inline const char *getVertexDirectionStr(VertexDirection direction) {
  switch (direction) {
    case VertexDirection::NONE:   return "NONE";
    case VertexDirection::INPUT:  return "INPUT";
    case VertexDirection::OUTPUT: return "OUTPUT";
    case VertexDirection::INOUT:  return "INOUT";
    default:                      return "UNKNOWN";
  }
}

} // End netlist_paths namespace.

#endif // NETLIST_PATHS_GRAPH_HPP
