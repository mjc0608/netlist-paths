#ifndef NETLIST_PATHS_READ_VERILATOR_XML_HPP
#define NETLIST_PATHS_READ_VERILATOR_XML_HPP

#include <stack>
#include <rapidxml-1.13/rapidxml.hpp>

#include "netlist_paths/Netlist.hpp"

namespace netlist_paths {

using XMLNode = rapidxml::xml_node<>;

class ReadVerilatorXML {
private:
  rapidxml::xml_node<> *currentLogic;
  rapidxml::xml_node<> *currentScope;
  std::stack<XMLNode*> logicParents;
  std::stack<XMLNode*> scopeParents;
  bool isDelayedAssign;
  bool isLValue;

  std::size_t numChildren(XMLNode *node);
  void dispatchVisitor(XMLNode *node);
  void iterateChildren(XMLNode *node);
  void newStatement(XMLNode *node);
  void newAssignStatement(XMLNode *node);
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
  void visitVarRef(XMLNode *node);
  void visitVarScope(XMLNode *node);

public:
  ReadVerilatorXML() :
      currentLogic(nullptr),
      currentScope(nullptr),
      isDelayedAssign(false),
      isLValue(false) {}
  // Helper function to constuct a Netlist from Verilator XML output.
  Netlist readXML(const std::string &filename);
};

} // End netlist_paths namespace.

#endif // NETLIST_PATHS_READ_VERILATOR_XML_HPP
