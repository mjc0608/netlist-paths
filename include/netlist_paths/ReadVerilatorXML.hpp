#ifndef NETLIST_PATHS_READ_VERILATOR_XML_HPP
#define NETLIST_PATHS_READ_VERILATOR_XML_HPP

#include <algorithm>
#include <memory>
#include <stack>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <rapidxml-1.13/rapidxml.hpp>

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
  C_FUNC,
  CONT_ASSIGN,
  INITIAL,
  MODULE,
  SEN_ITEM,
  SEN_GATE,
  SCOPE,
  TOP_SCOPE,
  TYPE_TABLE,
  VAR,
  VAR_REF,
  VAR_SCOPE,
  INVALID
};

/// Convert a string name into an AstNode type.
static AstNode resolveNode(const char *name) {
  static std::map<std::string, AstNode> mappings {
      { "always",       AstNode::ALWAYS },
      { "alwayspublic", AstNode::ALWAYS_PUBLIC },
      { "assignalias",  AstNode::ASSIGN_ALIAS },
      { "assigndly",    AstNode::ASSIGN_DLY },
      { "assignw",      AstNode::ASSIGN_W },
      { "assign",       AstNode::ASSIGN },
      { "basicdtype",   AstNode::BASIC_DTYPE },
      { "cfunc",        AstNode::C_FUNC },
      { "contassign",   AstNode::CONT_ASSIGN },
      { "initial",      AstNode::INITIAL },
      { "module",       AstNode::MODULE },
      { "senitem",      AstNode::SEN_ITEM },
      { "sengate",      AstNode::SEN_GATE },
      { "scope",        AstNode::SCOPE },
      { "topscope",     AstNode::TOP_SCOPE },
      { "typetable",    AstNode::TYPE_TABLE },
      { "var",          AstNode::VAR },
      { "varref",       AstNode::VAR_REF },
      { "varscope",     AstNode::VAR_SCOPE },
  };
  auto it = mappings.find(name);
  return (it != mappings.end()) ? it->second : AstNode::INVALID;
}

class VarScopeNode {
  XMLNode *node;
  std::string name;
  VertexDesc vertex;
public:
  VarScopeNode(XMLNode *node, VertexDesc vertex) :
      node(node), name(node->first_attribute("name")->value()), vertex(vertex) {}
  XMLNode *getNode() { return node; }
  std::string &getName() { return name; }
  VertexDesc getVertex() { return vertex; }
};

class ScopeNode {
  XMLNode *node;
  std::vector<std::unique_ptr<VarScopeNode>> vars;

public:
  ScopeNode(XMLNode *node) : node(node) {}
  /// Add a VAR_SCOPE.
  void addVarScope(XMLNode *varScopeNode, VertexDesc vertex) {
    assert(resolveNode(varScopeNode->name()) == AstNode::VAR_SCOPE &&
           "invalid node, expecting VAR_SCOPE");
    vars.push_back(std::make_unique<VarScopeNode>(varScopeNode, vertex));
    DEBUG(auto name = varScopeNode->first_attribute("name")->value();
          std::cout << "Add var '" << name << "' to scope\n");
  }
  VertexDesc lookupVarVertex(const std::string &name) {
    // Check the var ref is a suffix of a VAR_SCOPE.
    // (This is simplistic and should be improved.)
    auto equals = [&name](const std::unique_ptr<VarScopeNode> &node) {
        return boost::algorithm::ends_with(node->getName(), name); };
    auto it = std::find_if(std::begin(vars), std::end(vars), equals);
    return (it != std::end(vars)) ? (*it)->getVertex()
                                  : boost::graph_traits<Graph>::null_vertex();
  }
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
  std::stack<std::unique_ptr<LogicNode>> logicParents;
  std::stack<std::unique_ptr<ScopeNode>> scopeParents;
  std::unique_ptr<LogicNode> currentLogic;
  std::unique_ptr<ScopeNode> currentScope;
  bool isDelayedAssign;
  bool isLValue;

  std::size_t numChildren(XMLNode *node);
  void dispatchVisitor(XMLNode *node);
  void iterateChildren(XMLNode *node);
  void newScope(XMLNode *node);
  void newVarScope(XMLNode *node);
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
