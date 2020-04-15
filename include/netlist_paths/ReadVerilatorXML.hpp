#ifndef NETLIST_PATHS_READ_VERILATOR_XML_HPP
#define NETLIST_PATHS_READ_VERILATOR_XML_HPP

#include <algorithm>
#include <memory>
#include <stack>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <rapidxml-1.13/rapidxml.hpp>

#include "netlist_paths/Debug.hpp"
#include "netlist_paths/Netlist.hpp"

namespace netlist_paths {

using XMLNode = rapidxml::xml_node<>;

enum class AstNode {
  ALWAYS,
  ALWAYS_PUBLIC,
  ASSIGN,
  ASSIGN_ALIAS,
  ASSIGN_DLY,
  ASSIGN_W,
  BASIC_DTYPE,
  CONT_ASSIGN,
  C_FUNC,
  INITIAL,
  MODULE,
  PACKED_ARRAY_DTYPE,
  REF_DTYPE,
  SCOPE,
  SEN_GATE,
  SEN_ITEM,
  STRUCT_DTYPE,
  TOP_SCOPE,
  TYPE_TABLE,
  UNPACKED_ARRAY_DTYPE,
  VAR,
  VAR_REF,
  VAR_SCOPE,
  INVALID
};

/// Convert a string name into an AstNode type.
static AstNode resolveNode(const char *name) {
  static std::map<std::string, AstNode> mappings {
      { "always",             AstNode::ALWAYS },
      { "alwayspublic",       AstNode::ALWAYS_PUBLIC },
      { "assign",             AstNode::ASSIGN },
      { "assignalias",        AstNode::ASSIGN_ALIAS },
      { "assigndly",          AstNode::ASSIGN_DLY },
      { "assignw",            AstNode::ASSIGN_W },
      { "basicdtype",         AstNode::BASIC_DTYPE },
      { "cfunc",              AstNode::C_FUNC },
      { "contassign",         AstNode::CONT_ASSIGN },
      { "initial",            AstNode::INITIAL },
      { "module",             AstNode::MODULE },
      { "packedarraydtype",   AstNode::PACKED_ARRAY_DTYPE },
      { "refdtype",           AstNode::REF_DTYPE },
      { "scope",              AstNode::SCOPE },
      { "sengate",            AstNode::SEN_GATE },
      { "senitem",            AstNode::SEN_ITEM },
      { "structdtype",        AstNode::STRUCT_DTYPE },
      { "topscope",           AstNode::TOP_SCOPE },
      { "typetable",          AstNode::TYPE_TABLE },
      { "unpackedarraydtype", AstNode::UNPACKED_ARRAY_DTYPE },
      { "var",                AstNode::VAR },
      { "varref",             AstNode::VAR_REF },
      { "varscope",           AstNode::VAR_SCOPE },
  };
  auto it = mappings.find(name);
  return (it != mappings.end()) ? it->second : AstNode::INVALID;
}

class VarNode {
  std::string name;
  VertexDesc vertex;
public:
  VarNode(const std::string &name, VertexDesc vertex) :
      name(name), vertex(vertex) {}
  std::string &getName() { return name; }
  VertexDesc getVertex() { return vertex; }
};

class ScopeNode {
  XMLNode *node;
public:
  ScopeNode(XMLNode *node) : node(node) {}
  XMLNode *getNode() { return node; }
};

class LogicNode {
  XMLNode *node;
  ScopeNode &scope;
  VertexDesc vertex;
public:
  LogicNode(XMLNode *node, ScopeNode &scope, VertexDesc vertex) :
      node(node), scope(scope), vertex(vertex) {}
  XMLNode *getNode() { return node; }
  ScopeNode &getScope() { return scope; }
  VertexDesc getVertex() { return vertex; }
};

class ReadVerilatorXML {
private:
  std::unique_ptr<Netlist> netlist;
  std::vector<std::unique_ptr<VarNode>> vars;
  std::map<std::string, std::shared_ptr<File>> fileIdMappings;
  std::map<unsigned, std::shared_ptr<DType>> dtypeMappings;
  std::stack<std::unique_ptr<LogicNode>> logicParents;
  std::stack<std::unique_ptr<ScopeNode>> scopeParents;
  std::unique_ptr<LogicNode> currentLogic;
  std::unique_ptr<ScopeNode> currentScope;
  bool isDelayedAssign;
  bool isLValue;

  std::size_t numChildren(XMLNode *node);
  void dispatchVisitor(XMLNode *node);
  void iterateChildren(XMLNode *node);
  Location parseLocation(const std::string location);
  VertexDesc lookupVarVertex(const std::string &name);
  void newVar(XMLNode *node);
  void newScope(XMLNode *node);
  void newStatement(XMLNode *node, VertexType);
  void newVarRef(XMLNode *node);
  void visitNode(XMLNode *node);
  void visitModule(XMLNode *node);
  void visitTypeTable(XMLNode *node);
  void visitAlways(XMLNode *node);
  void visitAssign(XMLNode *node);
  void visitAssignDly(XMLNode *node);
  void visitInitial(XMLNode *node);
  void visitSenItem(XMLNode *node);
  void visitSenGate(XMLNode *node);
  void visitCFunc(XMLNode *node);
  void visitScope(XMLNode *node);
  void visitVar(XMLNode *node);
  void visitVarScope(XMLNode *node);
  void visitVarRef(XMLNode *node);
  void visitBasicDtype(XMLNode *node);
  void visitRefDtype(XMLNode *node);
  void visitPackedArrayDtype(XMLNode *node);
  void visitUnpackedArrayDtype(XMLNode *node);
  void visitStructDtype(XMLNode *node);

public:
  ReadVerilatorXML() :
      netlist(std::make_unique<Netlist>()),
      currentLogic(nullptr),
      currentScope(nullptr),
      isDelayedAssign(false),
      isLValue(false) {}
  // Helper function to constuct a Netlist from Verilator XML output.
  Netlist& readXML(const std::string &filename);
};

} // End netlist_paths namespace.

#endif // NETLIST_PATHS_READ_VERILATOR_XML_HPP
