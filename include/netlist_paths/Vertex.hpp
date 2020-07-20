#ifndef NETLIST_PATHS_VERTEX_HPP
#define NETLIST_PATHS_VERTEX_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include "netlist_paths/Location.hpp"
#include "netlist_paths/DTypes.hpp"

namespace netlist_paths {

//===----------------------------------------------------------------------===//
// Vertex type enums.
//===----------------------------------------------------------------------===//

// Corresponding type of vertex in the Verilator AST.
enum class VertexAstType {
  LOGIC,
  ASSIGN,
  ASSIGN_ALIAS,
  ASSIGN_DLY,
  ASSIGN_W,
  ALWAYS,
  INITIAL,
  REG,
  //REG_DST,
  SEN_GATE,
  SEN_ITEM,
  VAR,
  WIRE,
  PORT,
  C_FUNC,
  INVALID
};

// Vertex categorisation within the netlist graph.
enum class VertexGraphType {
  REG,
  //SRC_REG,
  LOGIC,
  START_POINT,
  END_POINT,
  MID_POINT,
  ANY
};

enum class VertexDirection {
  NONE,
  INPUT,
  OUTPUT,
  INOUT
};

//===----------------------------------------------------------------------===//
// Vertex type helper fuctions.
//===----------------------------------------------------------------------===//

inline VertexAstType getVertexAstType(const std::string &name) {
  static std::map<std::string, VertexAstType> mappings {
      { "LOGIC",        VertexAstType::LOGIC },
      { "ASSIGN",       VertexAstType::ASSIGN },
      { "ASSIGN_ALIAS", VertexAstType::ASSIGN_ALIAS },
      { "ASSIGN_DLY",   VertexAstType::ASSIGN_DLY },
      { "ASSIGN_W",     VertexAstType::ASSIGN_W },
      { "ALWAYS",       VertexAstType::ALWAYS },
      { "INITIAL",      VertexAstType::INITIAL },
      { "REG",          VertexAstType::REG },
      //{ "REG_DST",      VertexAstType::REG_DST },
      { "SEN_GATE",     VertexAstType::SEN_GATE },
      { "SEN_ITEM",     VertexAstType::SEN_ITEM },
      { "VAR",          VertexAstType::VAR },
      { "WIRE",         VertexAstType::WIRE },
      { "PORT",         VertexAstType::PORT },
      { "C_FUNC",       VertexAstType::C_FUNC },
  };
  auto it = mappings.find(name);
  return (it != mappings.end()) ? it->second : VertexAstType::INVALID;
}

inline const char *getVertexAstTypeStr(VertexAstType type) {
  switch (type) {
    case VertexAstType::LOGIC:        return "LOGIC";
    case VertexAstType::ASSIGN:       return "ASSIGN";
    case VertexAstType::ASSIGN_ALIAS: return "ASSIGN_ALIAS";
    case VertexAstType::ASSIGN_DLY:   return "ASSIGN_DLY";
    case VertexAstType::ASSIGN_W:     return "ASSIGN_W";
    case VertexAstType::ALWAYS:       return "ALWAYS";
    case VertexAstType::INITIAL:      return "INITIAL";
    case VertexAstType::REG:          return "REG";
    //case VertexAstType::REG_DST:      return "REG_DST";
    case VertexAstType::SEN_GATE:     return "SEN_GATE";
    case VertexAstType::SEN_ITEM:     return "SEN_ITEM";
    case VertexAstType::VAR:          return "VAR";
    case VertexAstType::WIRE:         return "WIRE";
    case VertexAstType::PORT:         return "PORT";
    case VertexAstType::C_FUNC:       return "C_FUNC";
    case VertexAstType::INVALID:      return "INVALID";
    default:                          return "UNKNOWN";
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

//===----------------------------------------------------------------------===//
// Vertex
//===----------------------------------------------------------------------===//

struct Vertex {
  //unsigned long long id;
  VertexAstType astType;
  VertexDirection direction;
  Location location;
  std::shared_ptr<DType> dtype;
  std::string name;
  bool isParam;
  std::string paramValue;
  bool isPublic;
  bool isTop;
  bool deleted;
  Vertex() {}
  /// Logic vertex.
  Vertex(VertexAstType type,
         Location location) :
      astType(type),
      direction(VertexDirection::NONE),
      location(location),
      isParam(false),
      isTop(false),
      isPublic(false),
      deleted(false) {}
  /// Var vertex.
  Vertex(VertexAstType type,
         VertexDirection direction,
         Location location,
         std::shared_ptr<DType> dtype,
         const std::string &name,
         bool isParam,
         const std::string &paramValue,
         bool isPublic) :
      astType(type),
      direction(direction),
      location(location),
      dtype(dtype),
      name(name),
      isParam(isParam),
      paramValue(paramValue),
      isTop(determineIsTop(name)),
      isPublic(isPublic),
      deleted(false) {}
  /// A Vertex is in the 'top' scope when has one or two hierarchical components.
  /// module.name or name is top level, but module.submodule.name is not.
  static bool determineIsTop(const std::string &name) {
    std::vector<std::string> tokens;
    boost::split(tokens, name, boost::is_any_of("."));
    return tokens.size() < 3;
  }
  /// Given a hierarchical name a.b.c, return the last component c.
  std::string getBasename() const {
    std::vector<std::string> tokens;
    boost::split(tokens, name, boost::is_any_of("."));
    return tokens.back();
  }
  /// Less than comparison
  bool compareLessThan(const Vertex &b) const {
    if (name        < b.name)      return true;
    if (b.name      < name)        return false;
    if (astType     < b.astType)   return true;
    if (b.astType   < astType)     return false;
    if (direction   < b.direction) return true;
    if (b.direction < direction)   return false;
    if (deleted     < b.deleted)   return true;
    if (b.deleted   < deleted)     return false;
    return false;
  }
  /// Equality comparison
  bool compareEqual(const Vertex &b) const {
    return astType    == b.astType &&
           direction  == b.direction &&
           location   == b.location &&
           dtype      == b.dtype &&
           name       == b.name &&
           isParam    == b.isParam &&
           paramValue == b.paramValue &&
           isTop      == b.isTop &&
           deleted    == b.deleted;
  }
  bool isGraphType(VertexGraphType type) const {
    switch (type) {
      case VertexGraphType::REG:         return isReg();
      //case VertexGraphType::SRC_REG:     return isSrcReg();
      case VertexGraphType::LOGIC:       return isLogic();
      case VertexGraphType::START_POINT: return isStartPoint();
      case VertexGraphType::END_POINT:   return isEndPoint();
      case VertexGraphType::MID_POINT:   return isMidPoint();
      default:                           return false;
    }
  }
  //inline bool isSrcReg() const {
  //  return !deleted &&
  //         astType == VertexAstType::REG_SRC;
  //}
  //inline bool isDstReg() const {
  //  return !deleted &&
  //         astType == VertexAstType::REG_DST;
  //}
  inline bool isLogic() const {
    return astType == VertexAstType::LOGIC ||
           astType == VertexAstType::ASSIGN ||
           astType == VertexAstType::ASSIGN_ALIAS ||
           astType == VertexAstType::ASSIGN_DLY ||
           astType == VertexAstType::ASSIGN_W ||
           astType == VertexAstType::ALWAYS ||
           astType == VertexAstType::INITIAL ||
           astType == VertexAstType::SEN_GATE ||
           astType == VertexAstType::SEN_ITEM;
  }
  inline bool isParameter() const {
    return isParam;
  }
  inline bool isReg() const {
    return !deleted &&
           astType == VertexAstType::REG;
  }
  inline bool isPort() const {
    return !deleted &&
           isTop &&
           (direction == VertexDirection::INPUT ||
            direction == VertexDirection::OUTPUT ||
            direction == VertexDirection::INOUT);
  }
  inline bool isStartPoint() const {
    return !deleted &&
           (astType == VertexAstType::REG ||
            (direction == VertexDirection::INPUT && isTop) ||
            (direction == VertexDirection::INOUT && isTop));
  }
  inline bool isEndPoint() const {
    return !deleted &&
           (astType == VertexAstType::REG ||
            (direction == VertexDirection::OUTPUT && isTop) ||
            (direction == VertexDirection::INOUT && isTop));
  }
  inline bool isMidPoint() const {
    return !deleted &&
           (astType == VertexAstType::VAR ||
            astType == VertexAstType::WIRE ||
            astType == VertexAstType::PORT);
  }
  inline bool canIgnore() const {
    // Ignore variables Verilator has introduced.
    return name.find("__Vdly") != std::string::npos ||
           name.find("__Vcell") != std::string::npos ||
           name.find("__Vconc") != std::string::npos;
  }
  /// Visible vertices are displayed in the name dump.
  inline bool isVisible() const {
    return !isLogic() &&
           //!isSrcReg() &&
           !canIgnore() &&
           !isDeleted();
  }
  /// Return a description of this vertex.
  std::string toString() const {
    // TODO: expand on this for different vertex types.
    return std::string(getVertexAstTypeStr(astType));
  }
  bool isDeleted() const { return deleted; }
  void setDeleted() { deleted = true; }
  //void setSrcReg() { astType = VertexAstType::REG_SRC; }
  const std::string &getName() const { return name; }
};

} // End netlist_paths namespace.

#endif // NETLIST_PATHS_VERTEX_HPP
