#include <fstream>
#include <iostream>
#include <map>

#include "netlist_paths/Debug.hpp"
#include "netlist_paths/Options.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

using namespace netlist_paths;

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

//struct LogicNode {
//  XMLNode *scope;
//  XMLNode *node;
//  LogicNode(XMLNode *scope, XMLNode *node) : scope(scope), node(node) {}
//};

/// Convert a string name into an AstNode type.
AstNode resolveNode(const char *name) {
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

void ReadVerilatorXML::dispatchVisitor(XMLNode *node) {
  // Handle node by type.
  switch (resolveNode(node->name())) {
  case AstNode::ALWAYS:        visitAlways(node);    break;
  case AstNode::ALWAYS_PUBLIC: visitAlways(node);    break;
  case AstNode::ASSIGN_ALIAS:  visitAssign(node);    break;
  case AstNode::ASSIGN_DLY:    visitAssignDly(node); break;
  case AstNode::ASSIGN_W:      visitAssign(node);    break;
  case AstNode::C_FUNC:        visitCFunc(node);     break;
  case AstNode::CONT_ASSIGN:   visitAssign(node);    break;
  case AstNode::INITIAL:       visitInitial(node);   break;
  case AstNode::SEN_ITEM:      visitSenItem(node);   break;
  case AstNode::SEN_GATE:      visitSenGate(node);   break;
  case AstNode::SCOPE:         visitScope(node);     break;
  case AstNode::TOP_SCOPE:     visitScope(node);     break;
  case AstNode::VAR:           visitVar(node);       break;
  case AstNode::VAR_REF:       visitVarRef(node);    break;
  case AstNode::VAR_SCOPE:     visitScope(node);     break;
  default:                     visitNode(node);      break;
  }
}

std::size_t ReadVerilatorXML::numChildren(XMLNode *node) {
  std::size_t count = 0;
  for (XMLNode *child = node->first_node();
       child; child = child->next_sibling()) {
    count++;
  }
  return count;
}

void ReadVerilatorXML::iterateChildren(XMLNode *node) {
  std::cout << node->name() << "\n";
  for (XMLNode *child = node->first_node();
       child; child = child->next_sibling()) {
    dispatchVisitor(child);
  }
}

void ReadVerilatorXML::newStatement(XMLNode *node) {
  // A statment must have a scope for variable references to occur in.
  if (currentScope) {
    logicParents.push(currentLogic);
    currentLogic = node;
    if (logicParents.top() != nullptr) {
      // Add edge to parent logic.
      std::cout << "new edge to+from logic\n";
    }
    iterateChildren(node);
    currentLogic = logicParents.top();
    logicParents.pop();
  }
}

void ReadVerilatorXML::newAssignStatement(XMLNode *node) {
  // A statment must have a scope for variable references to occur in.
  if (currentScope) {
    logicParents.push(currentLogic);
    currentLogic = node;
    if (logicParents.top() != nullptr) {
      // Add edge to parent logic.
      std::cout << "new edge to+from logic\n";
    }
    assert(numChildren(node) == 2 &&
           "assign statement has more than 2 children");
    dispatchVisitor(node->first_node()); // R-value.
    isLValue = true;
    dispatchVisitor(node->last_node()); // L-value.
    isLValue = false;
    currentLogic = logicParents.top();
    logicParents.pop();
  }
}

void ReadVerilatorXML::visitNode(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitModule(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitScope(XMLNode *node) {
  std::cout << "New scope\n";
  scopeParents.push(currentScope);
  currentScope = node;
  iterateChildren(node);
  currentScope = scopeParents.top();
  scopeParents.pop();
}

void ReadVerilatorXML::visitTypeTable(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitAssign(XMLNode *node) {
  newAssignStatement(node);
}

void ReadVerilatorXML::visitAssignDly(XMLNode *node) {
  isDelayedAssign = true;
  newAssignStatement(node);
  isDelayedAssign = false;
}

void ReadVerilatorXML::visitAlways(XMLNode *node) {
  newStatement(node);
}

void ReadVerilatorXML::visitInitial(XMLNode *node) {
  newStatement(node);
}

void ReadVerilatorXML::visitSenItem(XMLNode *node) {
  if (currentLogic) {
    iterateChildren(node);
  } else {
    newStatement(node);
  }
}

void ReadVerilatorXML::visitSenGate(XMLNode *node) {
  newStatement(node);
}

void ReadVerilatorXML::visitCFunc(XMLNode *node) {
  newAssignStatement(node);
}

void ReadVerilatorXML::visitVar(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitVarRef(XMLNode *node) {
  if (currentScope) {
    if (!currentLogic) {
      throw Exception(std::string("var ")+node->name()+" not under a logic block");
    }
    if (isLValue) {
      // Assignment to var
      if (isDelayedAssign) {
        // Var is reg
        std::cout << "new edge from logic to reg " << node->name() << "\n";
      } else {
        // Var is wire
        std::cout << "new edge from logic to var" << node->name() << "\n";;
      }
    } else {
      // Var is wire r-value.
      std::cout << "new edge from var " << node->name() << " to logic\n";
    }
    iterateChildren(node);
  }
}

Netlist ReadVerilatorXML::readXML(const std::string &filename) {
  INFO(std::cout << "Parsing input XML file\n");
  std::fstream inputFile(filename);
  if (!inputFile.is_open()) {
    throw Exception("could not open file");
  }
  // Parse the buffered XML.
  rapidxml::xml_document<> doc;
  std::vector<char> buffer((std::istreambuf_iterator<char>(inputFile)),
                            std::istreambuf_iterator<char>());
  buffer.push_back('\0');
  doc.parse<0>(&buffer[0]);
  // Find our root node
  XMLNode *rootNode = doc.first_node("verilator_xml");
  // Files section
  XMLNode *filesNode = rootNode->first_node("files");
  for (XMLNode *fileNode = filesNode->first_node("file");
       fileNode; fileNode = fileNode->next_sibling()) {
    std::cout << "file " << fileNode->first_attribute("id")->value()
              << " " << fileNode->first_attribute("filename")->value()
              << " " << fileNode->first_attribute("language")->value() << "\n";
  }
  //XMLNode *moduleFilesNode = rootNode->first_node("module_files");
  //XMLNode *cellsNode = rootNode->first_node("cells");
  XMLNode *netlistNode = rootNode->first_node("netlist");
  XMLNode *topModuleNode = netlistNode->first_node("module");
  assert(std::string(topModuleNode->first_attribute("name")->value()) == "TOP" &&
         "top module name does not equal TOP");
  XMLNode *typeTableNode = netlistNode->first_node("typetable");
  visitModule(topModuleNode);
  visitTypeTable(typeTableNode);
  return Netlist();
}
