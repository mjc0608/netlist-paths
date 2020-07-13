#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <regex>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/tokenizer.hpp>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/Exception.hpp"
#include "netlist_paths/Options.hpp"
#include "netlist_paths/Debug.hpp"

namespace fs = boost::filesystem;
using namespace netlist_paths;

class DfsVisitor : public boost::default_dfs_visitor {
private:
  ParentMap &parentMap;
  bool allPaths;
public:
  DfsVisitor(ParentMap &parentMap, bool allPaths) :
      parentMap(parentMap), allPaths(allPaths) {}
  // Visit only the edges of the DFS graph.
  template<typename Edge, typename Graph>
  void tree_edge(Edge edge, const Graph &graph) const {
    if (!allPaths) {
      VertexDesc src, dst;
      src = boost::source(edge, graph);
      dst = boost::target(edge, graph);
      parentMap[dst].push_back(src);
    }
    return;
  }
  // Visit all edges of a all vertices.
  template<typename Edge, typename Graph>
  void examine_edge(Edge edge, const Graph &graph) const {
    if (allPaths) {
      VertexDesc src, dst;
      src = boost::source(edge, graph);
      dst = boost::target(edge, graph);
      parentMap[dst].push_back(src);
    }
    return;
  }
};

/// Remove duplicate vertices from the graph by sorting them comparing each
/// vertex to its neighbours.
void Netlist::mergeDuplicateVertices() {
  std::vector<VertexDesc> vs;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (!graph[v].isLogic()) {
      vs.push_back(v);
    }
  }
  auto compare = [this](const VertexDesc a, const VertexDesc b) {
                   return graph[a].compareLessThan(graph[b]); };
  std::sort(std::begin(vs), std::end(vs), compare);
  VertexDesc current = vs[0];
  unsigned count = 0;
  for (size_t i=1; i<vs.size(); i++) {
    if (graph[vs[i]].compareEqual(graph[current])) {
      DEBUG(std::cout << "DUPLICATE VERTEX " << graph[vs[i]].name << "\n");
      BGL_FORALL_ADJ(vs[i], v, graph, Graph) {
        boost::add_edge(current, v, graph);
        boost::remove_edge(vs[i], v, graph);
      }
      // We mark duplicate vertices as deleted since it is expensive to remove
      // them from the graph as vertices are stored in a vecS. Using a listS
      // is less performant.
      graph[vs[i]].setDeleted();
      ++count;
    } else {
      current = vs[i];
    }
  }
  INFO(std::cout << "Removed " << count << " duplicate vertices\n");
}

/// Perform some checks on the netlist and emit warnings if necessary.
void Netlist::checkGraph() const {
  BGL_FORALL_VERTICES(v, graph, Graph) {
    // Check there are no Vlvbound nodes.
    if (graph[v].name.find("__Vlvbound") == std::string::npos) {
      std::cout << "Warning: " << graph[v].name << " vertex in netlist\n";
    }
    // Source registers don't have in edges.
    if (graph[v].type == VertexType::REG_SRC) {
      if (boost::in_degree(v, graph) > 0)
         std::cout << "Warning: source reg " << graph[v].name
                   << " has in edges" << "\n";
    }
    // Destination registers don't have out edges.
    if (graph[v].type == VertexType::REG_DST) {
      if (boost::out_degree(v, graph) > 0)
        std::cout << "Warning: destination reg " << graph[v].name
                  << " has out edges"<<"\n";
    }
    // NOTE: vertices may be incorrectly marked as reg if a field of a
    // structure has a delayed assignment to a field of it.
  }
}

///// Dump unique names of vars/regs/wires in the netlist for searching.
//std::vector<VertexDesc> Netlist::getNames() const {
//  std::vector<VertexDesc> vs;
//  // Collect vertices.
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (!isLogic(graph[v]) &&
//        !isSrcReg(graph[v]) &&
//        !canIgnore(graph[v]) &&
//        !graph[v].deleted) {
//      vs.push_back(v);
//    }
//  }
//  // Sort them.
//  auto compare = [this](const VertexDesc a, const VertexDesc b) {
//                   return vertexCompare(a, b); };
//  std::sort(vs.begin(), vs.end(), compare);
//  return vs;
//}
//
//void Netlist::printNames(std::vector<VertexDesc> &names) const {
//  // Print the output.
//  int maxWidth = maxNameLength(names) + 1;
//  std::cout << std::left << std::setw(maxWidth) << "Name"
//            << std::left << std::setw(10)       << "Type"
//            << std::left << std::setw(10)       << "Direction"
//            << std::left << std::setw(10)       << "Width"
//                                                << "Location\n";
//  for (auto v : names) {
//    auto type = getVertexTypeStr(graph[v].type);
//    auto srcPath = netlist_paths::options.fullFileNames ? fs::path(graph[v].location.getFilename())
//                                                        : fs::path(graph[v].location.getFilename()).filename();
//    std::cout << std::left << std::setw(maxWidth) << graph[v].name
//              << std::left << std::setw(10)       << (std::string(type) == "REG_DST" ? "REG" : type)
//              << std::left << std::setw(10)       << getVertexDirectionStr(graph[v].direction)
//              //<< std::left << std::setw(10)       << graph[v].width
//                                                  << srcPath.string()
//              << "\n";
//  }
//}
//
///// Dump a Graphviz dotfile of the netlist graph for visualisation.
//void Netlist::dumpDotFile(const std::string &outputFilename) const {
//  std::ofstream outputFile(outputFilename);
//  if (!outputFile.is_open()) {
//    throw Exception(std::string("unable to open ")+outputFilename);
//  }
////  // Write graphviz format.
////  boost::write_graphviz_dp(outputFile, graph, dp, /*node_id=*/"id");
//  // Loop over all vertices and print properties.
//  outputFile << "digraph netlist {\n";
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    outputFile << v << " [id="<<graph[v].id
//       <<" name=\""<<graph[v].name << "\""
//       <<" type=\""<<getVertexTypeStr(graph[v].type) << "\""
//       <<"]\n";
//  }
//  // Loop over all edges.
//  BGL_FORALL_EDGES(e, graph, Graph) {
//    outputFile << boost::source(e, graph) << " -> "
//               << boost::target(e, graph) << ";\n";
//  }
//  outputFile << "}\n";
//  outputFile.close();
//  // Print command line to generate graph file.
//  INFO(std::cout << "dot -Tpdf " << outputFilename << " -o graph.pdf\n");
//}
//
///// Lookup a vertex using a regex pattern and function specifying a type.
//VertexDesc Netlist::getVertexDesc(const std::string &name,
//                                  bool matchVertex (const VertexProperties &p)) const {
//  // FIXME: create a list of candidate vertices, rather than iterating all vertices.
//  auto nameRegexStr(name);
//  // Ignoring '/' (when supplying a heirarchical ref).
//  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '_', '.');
//  // Or '_' (when supplying a flattened name).
//  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '/', '.');
//  std::regex nameRegex(nameRegexStr);
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (matchVertex(graph[v])) {
//      if (std::regex_search(graph[v].name, nameRegex)) {
//        return v;
//      }
//    }
//  }
//  return boost::graph_traits<Graph>::null_vertex();
//}
//
//// FIXME: Move exception logic into tool.
//VertexDesc Netlist::getStartVertexExcept(const std::string &name) const {
//  auto vertex = getStartVertex(name);
//  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
//    throw Exception(std::string("could not find vertex ")+name);
//  } else {
//    return vertex;
//  }
//}
//
//// FIXME: Move exception logic into tool.
//VertexDesc Netlist::getEndVertexExcept(const std::string &name) const {
//  auto vertex = getEndVertex(name);
//  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
//    throw Exception(std::string("could not find vertex ")+name);
//  } else {
//    return vertex;
//  }
//}
//
//// FIXME: Move exception logic into tool.
//VertexDesc Netlist::getMidVertexExcept(const std::string &name) const {
//  auto vertex = getMidVertex(name);
//  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
//    throw Exception(std::string("could not find vertex ")+name);
//  } else {
//    return vertex;
//  }
//}
//
//void Netlist::dumpPath(const std::vector<VertexDesc> &path) const {
//  for (auto v : path) {
//    if (!isLogic(graph[v])) {
//      std::cout << "  " << graph[v].name << "\n";
//    }
//  }
//}
//
///// Given the tree structure from a DFS, traverse the tree from leaf to root to
///// return a path.
//Path Netlist::determinePath(ParentMap &parentMap,
//                                 Path path,
//                                 VertexDesc startVertex,
//                                 VertexDesc endVertex) const {
//  path.push_back(endVertex);
//  if (endVertex == startVertex) {
//    return path;
//  }
//  if (parentMap[endVertex].size() == 0)
//    return std::vector<VertexDesc>();
//  assert(parentMap[endVertex].size() == 1);
//  auto nextVertex = parentMap[endVertex].front();
//  assert(std::find(std::begin(path),
//                   std::end(path),
//                   nextVertex) == std::end(path));
//  return determinePath(parentMap, path, startVertex, nextVertex);
//}
//
///// Determine all paths between a start and an end point.
///// This performs a DFS starting at the end point. It is not feasible for large
///// graphs since the number of simple paths grows exponentially.
//void Netlist::determineAllPaths(ParentMap &parentMap,
//                                     std::vector<Path> &result,
//                                     Path path,
//                                     VertexDesc startVertex,
//                                     VertexDesc endVertex) const {
//  path.push_back(endVertex);
//  if (endVertex == startVertex) {
//    INFO(std::cout << "FOUND PATH\n");
//    result.push_back(path);
//    return;
//  }
//  INFO(std::cout<<"length "<<path.size()<<" vertex "<<graph[endVertex].id<<"\n");
//  INFO(dumpPath(path));
//  INFO(std::cout<<(parentMap[endVertex].empty()?"DEAD END\n":""));
//  for (auto vertex : parentMap[endVertex]) {
//    if (std::find(std::begin(path), std::end(path), vertex) == std::end(path)) {
//      determineAllPaths(parentMap, result, path, startVertex, vertex);
//    } else {
//      INFO(std::cout << "CYCLE DETECTED\n");
//    }
//  }
//}
//
///// Determine the max length of a name.
//int Netlist::maxNameLength(const Path &path) const {
//  size_t maxLength = 0;
//  for (auto v : path) {
//    if (canIgnore(graph[v]))
//      continue;
//    maxLength = std::max(maxLength, graph[v].name.size());
//  }
//  return static_cast<int>(maxLength);
//}
//
///// Pretty print a path (some sequence of vertices).
//void Netlist::printPathReport(const Path &path) const {
//  int maxWidth = maxNameLength(path) + 1;
//  // Print each vertex on the path.
//  for (auto v : path) {
//    if (canIgnore(graph[v]))
//      continue;
//    auto srcPath = netlist_paths::options.fullFileNames ? fs::path(graph[v].location.getFilename())
//                                                        : fs::path(graph[v].location.getFilename()).filename();
//    if (!netlist_paths::options.reportLogic) {
//      if (!isLogic(graph[v])) {
//        std::cout << "  " << std::left
//                  << std::setw(maxWidth)
//                  << graph[v].name
//                  << srcPath.string() << "\n";
//      }
//    } else {
//      if (isLogic(graph[v])) {
//        std::cout << "  " << std::left
//                  << std::setw(maxWidth)
//                  << getVertexTypeStr(graph[v].type)
//                  << std::setw(VERTEX_TYPE_STR_MAX_LEN)
//                  << "LOGIC"
//                  << srcPath.string() << "\n";
//      } else {
//        std::cout << "  " << std::left
//                  << std::setw(maxWidth)
//                  << graph[v].name
//                  << std::setw(VERTEX_TYPE_STR_MAX_LEN)
//                  << getVertexTypeStr(graph[v].type)
//                  << srcPath.string() << "\n";
//      }
//    }
//  }
//}
//
///// Print a collection of paths.
//void Netlist::
//printPathReport(const std::vector<Path> &paths) const {
//  int pathCount = 0;
//  for (auto &path : paths) {
//    if (!path.empty()) {
//      std::cout << "Path " << ++pathCount << "\n";
//      printPathReport(path);
//      std::cout << "\n";
//    }
//  }
//  std::cout << "Found " << pathCount << " path(s)\n";
//}
//
///// Report all paths fanning out from a net/register/port.
//std::vector<Path> Netlist::
//getAllFanOut(VertexDesc startVertex) const {
//  INFO(std::cout << "Performing DFS from "
//                 << graph[startVertex].name << "\n");
//  ParentMap parentMap;
//  boost::depth_first_search(graph,
//      boost::visitor(DfsVisitor(parentMap, false))
//        .root_vertex(startVertex));
//  // Check for a path between startPoint and each register.
//  std::vector<Path> paths;
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (isEndPoint(graph[v])) {
//      auto path = determinePath(parentMap,
//                                Path(),
//                                startVertex,
//                                static_cast<VertexDesc>(graph[v].id));
//      if (!path.empty()) {
//        std::reverse(std::begin(path), std::end(path));
//        paths.push_back(path);
//      }
//    }
//  }
//  return paths;
//}
//
//std::vector<Path> Netlist::
//getAllFanOut(const std::string &startName) const {
//  auto startVertex = getStartVertexExcept(startName);
//  return getAllFanOut(startVertex);
//}
//
///// Report all paths fanning into a net/register/port.
//std::vector<Path> Netlist::
//getAllFanIn(VertexDesc endVertex) const {
//  auto reverseGraph = boost::make_reverse_graph(graph);
//  INFO(std::cout << "Performing DFS in reverse graph from "
//                 << graph[endVertex].name << "\n");
//  ParentMap parentMap;
//  boost::depth_first_search(reverseGraph,
//      boost::visitor(DfsVisitor(parentMap, false))
//        .root_vertex(endVertex));
//  // Check for a path between endPoint and each register.
//  std::vector<Path> paths;
//  BGL_FORALL_VERTICES(v, graph, Graph) {
//    if (isStartPoint(graph[v])) {
//      auto path = determinePath(parentMap,
//                                Path(),
//                                endVertex,
//                                static_cast<VertexDesc>(graph[v].id));
//      if (!path.empty()) {
//        paths.push_back(path);
//      }
//    }
//  }
//  return paths;
//}
//
//std::vector<Path> Netlist::
//getAllFanIn(const std::string &endName) const {
//  auto endVertex = getEndVertexExcept(endName);
//  return getAllFanIn(endVertex);
//}
//
///// Report a single path between a set of named points.
//Path Netlist::
//getAnyPointToPoint() const {
//  std::vector<VertexDesc> path;
//  // Construct the path between each adjacent waypoints.
//  for (std::size_t i = 0; i < waypoints.size()-1; ++i) {
//    auto startVertex = waypoints[i];
//    auto endVertex = waypoints[i+1];
//    INFO(std::cout << "Performing DFS from "
//                   << graph[startVertex].name << "\n");
//    ParentMap parentMap;
//    boost::depth_first_search(graph,
//        boost::visitor(DfsVisitor(parentMap, false))
//          .root_vertex(startVertex));
//    INFO(std::cout << "Determining a path to "
//                   << graph[endVertex].name << "\n");
//    auto subPath = determinePath(parentMap,
//                                 Path(),
//                                 startVertex,
//                                 endVertex);
//    if (subPath.empty()) {
//      // No path exists.
//      return Path();
//    }
//    std::reverse(std::begin(subPath), std::end(subPath));
//    path.insert(std::end(path), std::begin(subPath), std::end(subPath)-1);
//  }
//  path.push_back(waypoints.back());
//  return path;
//}
//
///// Report all paths between start and end points.
//std::vector<Path> Netlist::
//getAllPointToPoint() const {
//  assert(waypoints.size() > 2 && "invlalid waypoints");
//  INFO(std::cout << "Performing DFS\n");
//  ParentMap parentMap;
//  boost::depth_first_search(graph,
//      boost::visitor(DfsVisitor(parentMap, true))
//        .root_vertex(waypoints[0]));
//  INFO(std::cout << "Determining all paths\n");
//  std::vector<Path> paths;
//  determineAllPaths(parentMap,
//                    paths,
//                    Path(),
//                    waypoints[0],
//                    waypoints[1]);
//  for (auto &path : paths) {
//    std::reverse(std::begin(path), std::end(path));
//  }
//  return paths;
//}
//
///// Return the number of registers a start point fans out to.
//unsigned Netlist::
//getfanOutDegree(VertexDesc startVertex) {
//  const auto paths = getAllFanOut(startVertex);
//  unsigned count = 0;
//  for (auto &path : paths) {
////    auto endVertex = path.back();
////    count += graph[endVertex].width;
//    count += 1;
//  }
//  return count;
//}
//
//unsigned Netlist::
//getfanOutDegree(const std::string &startName) {
//  auto startVertex = getStartVertexExcept(startName);
//  return getfanOutDegree(startVertex);
//}
//
///// Return he number of registers that fan into an end point.
//unsigned Netlist::
//getFanInDegree(VertexDesc endVertex) {
//  const auto paths = getAllFanIn(endVertex);
//  unsigned count = 0;
//  for (auto &path : paths) {
////    auto startVertex = path.front();
////    count += graph[startVertex].width;
//    count += 1;
//  }
//  return count;
//}
//
//unsigned Netlist::
//getFanInDegree(const std::string &endName) {
//  auto endVertex = getEndVertexExcept(endName);
//  return getFanInDegree(endVertex);
//}
//
///// Check if a path exists between two points.
//bool Netlist::
//pathExists(const std::string &start, const std::string &end) {
//  clearWaypoints();
//  // Check that the start and end points exist.
//  auto startPoint = getStartVertex(start);
//  auto endPoint = getEndVertex(end);
//  if (startPoint == boost::graph_traits<Graph>::null_vertex() ||
//      endPoint == boost::graph_traits<Graph>::null_vertex()) {
//    return false;
//  }
//  // Check the path exists.
//  waypoints.push_back(startPoint);
//  waypoints.push_back(endPoint);
//  auto path = getAnyPointToPoint();
//  return !path.empty();
//}
