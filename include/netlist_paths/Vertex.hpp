#ifndef NETLIST_PATHS_VERTEX_HPP
#define NETLIST_PATHS_VERTEX_HPP

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "netlist_paths/Location.hpp"
#include "netlist_paths/DTypes.hpp"

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

struct Vertex {
  //unsigned long long id;
  VertexType type;
  VertexDirection direction;
  Location location;
  std::shared_ptr<DType> dtype;
  std::string name;
  bool isParam;
  std::string paramValue;
  bool isTop;
  bool deleted;
  Vertex() {}
  /// Logic vertex.
  Vertex(VertexType type,
         Location location) :
      type(type),
      direction(VertexDirection::NONE),
      location(location),
      isParam(false),
      isTop(false),
      deleted(false) {}
  /// Var vertex.
  Vertex(VertexType type,
         VertexDirection direction,
         Location location,
         std::shared_ptr<DType> dtype,
         const std::string &name,
         bool isParam,
         const std::string &paramValue) :
      type(type),
      direction(direction),
      location(location),
      dtype(dtype),
      name(name),
      isParam(isParam),
      paramValue(paramValue),
      isTop(determineIsTop(name)),
      deleted(false) {}
  /// Less than comparison
  bool compareLessThan(const Vertex &b) {
    if (name        < b.name)      return true;
    if (b.name      < name)        return false;
    if (type        < b.type)      return true;
    if (b.type      < type)        return false;
    if (direction   < b.direction) return true;
    if (b.direction < direction)   return false;
    if (deleted     < b.deleted)   return true;
    if (b.deleted   < deleted)     return false;
    return false;
  }
  /// Equality comparison
  bool compareEqual(const Vertex &b) {
    return type       == b.type &&
           direction  == b.direction &&
           location   == b.location &&
           dtype      == b.dtype &&
           name       == b.name &&
           isParam    == b.isParam &&
           paramValue == b.paramValue &&
           isTop      == b.isTop &&
           deleted    == b.deleted;
  }
  inline bool isSrcReg() const {
    return !deleted &&
           type == VertexType::REG_SRC;
  }
  inline bool isLogic() const {
    return type == VertexType::LOGIC ||
           type == VertexType::ASSIGN ||
           type == VertexType::ASSIGN_ALIAS ||
           type == VertexType::ASSIGN_DLY ||
           type == VertexType::ASSIGN_W ||
           type == VertexType::ALWAYS ||
           type == VertexType::INITIAL ||
           type == VertexType::SEN_GATE ||
           type == VertexType::SEN_ITEM;
  }
  inline bool isReg() const {
    return !deleted &&
           (type == VertexType::REG_DST ||
            type == VertexType::REG_SRC);
  }
  inline bool isStartPoint() const {
    return !deleted &&
           (type == VertexType::REG_SRC ||
            (direction == VertexDirection::INPUT && isTop) ||
            (direction == VertexDirection::INOUT && isTop));
  }
  inline bool isEndPoint() const {
    return !deleted &&
           (type == VertexType::REG_DST ||
            (direction == VertexDirection::OUTPUT && isTop) ||
            (direction == VertexDirection::INOUT && isTop));
  }
  inline bool isMidPoint() const {
    return !deleted &&
           (type == VertexType::VAR ||
            type == VertexType::WIRE ||
            type == VertexType::PORT);
  }
  inline bool canIgnore() const {
    // Ignore variables Verilator has introduced.
    return name.find("__Vdly") != std::string::npos ||
           name.find("__Vcell") != std::string::npos ||
           name.find("__Vconc") != std::string::npos;
  }
  void setDeleted() { deleted = true; }
  static bool determineIsTop(const std::string &name) {
    // Check there are no Vlvbound nodes.
    assert(name.find("__Vlvbound") == std::string::npos);
    // module.name or name is top level, but module.submodule.name is not.
    std::vector<std::string> tokens;
    boost::split(tokens, name, boost::is_any_of("."));
    return tokens.size() < 3;
  }
};

//===----------------------------------------------------------------------===//
// Vertex helper fuctions.
//===----------------------------------------------------------------------===//

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

#endif // NETLIST_PATHS_VERTEX_HPP
