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

class ScopeNode {
  XMLNode *node;
  std::vector<XMLNode*> varScopes;

public:
  ScopeNode(XMLNode *node) : node(node) {}
  ScopeNode(XMLNode *node, ScopeNode &parentScope) : node(node) {
    // Copy the variables from the parent scope into this one.
    varScopes.insert(std::begin(varScopes),
                     std::begin(parentScope.getVarScopes()),
                     std::end(parentScope.getVarScopes()));
  }
  /// Check a VAR_REF has a corresponding VAR_SCOPE declaration.
  bool hasVarScope(XMLNode *varRefNode) {
    assert(resolveNode(varRefNode->name()) == AstNode::VAR_REF &&
           "invalid node, expecting VAR_REF");
    // Check the var ref is a suffix of a VAR_SCOPE.
    // (This is simplistic and should be improved.)
    auto varRefName = varRefNode->first_attribute("name")->value();
    auto it = std::find_if(std::begin(varScopes),
                           std::end(varScopes),
                           [&](const XMLNode *n) {
                             return boost::algorithm::ends_with(n->first_attribute("name")->value(),
                                                                varRefName); } );
    return it != std::end(varScopes);
  }
  /// Add a VAR_SCOPE.
  void addVarScope(XMLNode *varScopeNode) {
    assert(resolveNode(varScopeNode->name()) == AstNode::VAR_SCOPE &&
           "invalid node, expecting VAR_SCOPE");
    varScopes.push_back(varScopeNode);
    //std::cout << "Add var " << varScopeNode->first_attribute("name")->value()<<" to scope\n";
  }
  const std::vector<XMLNode*> getVarScopes() const { return varScopes; }
};

class LogicNode {
  XMLNode *node;
  ScopeNode &scope;
  std::vector<std::pair<std::string, VertexDesc>> vertices;
  VertexDesc vertex;
public:
  LogicNode(XMLNode *node, ScopeNode &scope) : node(node), scope(scope) {}
  /// Record a vertex in the Netlist graph.
  void addVertex(const std::string &name, VertexDesc vertexDesc) {
    vertices.push_back(std::make_pair(name, vertexDesc));
    //std::cout << "Add logic vertex for var " << name << "\n";
    vertex = vertexDesc;
  }
  /// Return the vertex associated with the variable name.
  VertexDesc lookupVertex(const std::string &name) {
    auto it = std::find_if(std::begin(vertices),
                           std::end(vertices),
                           [&name](const std::pair<std::string, VertexDesc> &element) {
                             return boost::algorithm::ends_with(element.first,
                                                                name); } );
    if (it != std::end(vertices)) {
      return it->second;
    } else {
      return boost::graph_traits<Graph>::null_vertex();
    }
  }
  VertexDesc getVertex() { return vertex; }
  const std::vector<XMLNode*> getVarScopes() const { return scope.getVarScopes(); }
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
