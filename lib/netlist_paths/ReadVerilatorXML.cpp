#include <fstream>
#include <iostream>
#include <map>

#include "netlist_paths/Debug.hpp"
#include "netlist_paths/Options.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

using namespace netlist_paths;

void ReadVerilatorXML::dispatchVisitor(XMLNode *node) {
  // Handle node by type.
  switch (resolveNode(node->name())) {
  case AstNode::ALWAYS:        visitAlways(node);    break;
  case AstNode::ALWAYS_PUBLIC: visitAlways(node);    break;
  case AstNode::ASSIGN:        visitAssign(node);    break;
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
  case AstNode::VAR_SCOPE:     visitVarScope(node);  break;
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
  for (XMLNode *child = node->first_node();
       child; child = child->next_sibling()) {
    dispatchVisitor(child);
  }
}

void ReadVerilatorXML::newScope(XMLNode *node) {
  DEBUG(std::cout << "New scope\n");
  scopeParents.push(std::move(currentScope));
  currentScope = std::make_unique<ScopeNode>(node);
  iterateChildren(node);
  currentScope = std::move(scopeParents.top());
  scopeParents.pop();
}

void ReadVerilatorXML::newVarScope(XMLNode *node) {
  assert(numChildren(node) == 0 && "varscope has children");
  // Add this <varscope> to the current scope.
  auto vertex = netlist->addVertex(VertexType::VAR);
  currentScope->addVarScope(node, vertex);
}

void ReadVerilatorXML::newStatement(XMLNode *node, VertexType vertexType) {
  DEBUG(std::cout << "New statement: " << getVertexTypeStr(vertexType) << "\n");
  // A statment must have a scope for variable references to occur in.
  if (currentScope) {
    logicParents.push(std::move(currentLogic));
    // Create a vertex for this logic.
    auto vertex = netlist->addVertex(vertexType);
    currentLogic = std::make_unique<LogicNode>(node, *currentScope, vertex);
    // Create an edge from the parent logic to this one.
    if (logicParents.top()) {
      auto vertexParent = logicParents.top()->getVertex();
      netlist->addEdge(vertexParent, vertex);
      DEBUG(std::cout << "Edge from parent logic to "
                      << getVertexTypeStr(vertexType) << "\n");
    }
    if (vertexType == VertexType::ASSIGN ||
        vertexType == VertexType::ASSIGN_ALIAS ||
        vertexType == VertexType::ASSIGN_DLY ||
        vertexType == VertexType::ASSIGN_W) {
      // Handle assignments to distinguish L and R values.
      assert(numChildren(node) == 2 &&
             "assign statement has more than 2 children");
      dispatchVisitor(node->first_node()); // R-value.
      isLValue = true;
      dispatchVisitor(node->last_node()); // L-value.
      isLValue = false;
    } else {
      iterateChildren(node);
    }
    currentLogic = std::move(logicParents.top());
    logicParents.pop();
  }
}

void ReadVerilatorXML::newVarRef(XMLNode *node) {
  if (currentScope) {
    if (!currentLogic) {
      auto name = std::string(node->first_attribute("name")->value());
      throw Exception(std::string("var ")+name+" not under a logic block");
    }
    auto varName = node->first_attribute("name")->value();
    auto varScopeVertex = currentScope->lookupVarVertex(varName);
    if (varScopeVertex == boost::graph_traits<Graph>::null_vertex()) {
      throw Exception(std::string("var ")+varName+" does not have a VAR_SCOPE");
    }
    //auto varDTypeID = std::stoul(node->first_attribute("dtype_id")->value());
    //auto varFileLine = node->first_attribute("fl")->value();
    //auto varLocation = node->first_attribute("loc")->value();
    if (isLValue) {
      // Assignment to var
      if (isDelayedAssign) {
        // Var is reg l-value.
        // TODO: set varscope to REG
        netlist->addEdge(currentLogic->getVertex(), varScopeVertex);
        DEBUG(std::cout << "Edge from logic to reg '" << varName << "'\n");
      } else {
        // Var is wire l-value.
        netlist->addEdge(currentLogic->getVertex(), varScopeVertex);
        DEBUG(std::cout << "Edge from logic to var '" << varName << "'\n");
      }
    } else {
      // Var is wire r-value.
      netlist->addEdge(varScopeVertex, currentLogic->getVertex());
      DEBUG(std::cout << "Edge from var '" << varName << "' to logic\n");
    }
    iterateChildren(node);
  }
}

//===----------------------------------------------------------------------===//
// Visitor methods.
//===----------------------------------------------------------------------===//

void ReadVerilatorXML::visitNode(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitModule(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitScope(XMLNode *node) {
  newScope(node);
}

void ReadVerilatorXML::visitTypeTable(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitAssign(XMLNode *node) {
  newStatement(node, VertexType::ASSIGN);
}

void ReadVerilatorXML::visitAssignDly(XMLNode *node) {
  isDelayedAssign = true;
  newStatement(node, VertexType::ASSIGN_DLY);
  isDelayedAssign = false;
}

void ReadVerilatorXML::visitAlways(XMLNode *node) {
  newStatement(node, VertexType::ALWAYS);
}

void ReadVerilatorXML::visitInitial(XMLNode *node) {
  newStatement(node, VertexType::INITIAL);
}

void ReadVerilatorXML::visitSenItem(XMLNode *node) {
  if (currentLogic) {
    iterateChildren(node);
  } else {
    newStatement(node, VertexType::SEN_ITEM);
  }
}

void ReadVerilatorXML::visitSenGate(XMLNode *node) {
  newStatement(node, VertexType::SEN_GATE);
}

void ReadVerilatorXML::visitCFunc(XMLNode *node) {
  newStatement(node, VertexType::C_FUNC);
}

void ReadVerilatorXML::visitVar(XMLNode *node) {
  iterateChildren(node);
}

void ReadVerilatorXML::visitVarScope(XMLNode *node) {
  newVarScope(node);
}

void ReadVerilatorXML::visitVarRef(XMLNode *node) {
  newVarRef(node);
}

Netlist& ReadVerilatorXML::readXML(const std::string &filename) {
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
    auto fileId = fileNode->first_attribute("id")->value();
    auto filename = fileNode->first_attribute("filename")->value();
    auto language = fileNode->first_attribute("language")->value();
    netlist->addFile(File(fileId, filename, language));
  }
  // Netlist section.
  XMLNode *netlistNode = rootNode->first_node("netlist");
  assert(numChildren(netlistNode) == 2 &&
         "expected module and typetable children");
  // Module (single instance).
  XMLNode *topModuleNode = netlistNode->first_node("module");
  visitModule(topModuleNode);
  assert(std::string(topModuleNode->first_attribute("name")->value()) == "TOP" &&
         "top module name does not equal TOP");
  // Typetable.
  XMLNode *typeTableNode = netlistNode->first_node("typetable");
  visitTypeTable(typeTableNode);
  return *netlist;
}
