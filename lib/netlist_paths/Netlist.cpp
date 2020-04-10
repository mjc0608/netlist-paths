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

namespace boost {
  // Provide template specializations of lexical casts for Boost to convert
  // strings to/from enums in the dynamic property map.
  template<>
  netlist_paths::VertexType lexical_cast(const std::string &s) {
    return netlist_paths::getVertexType(s);
  }
  template<>
  netlist_paths::VertexDirection lexical_cast(const std::string &s) {
    return netlist_paths::getVertexDirection(s);
  }
  template<>
  std::string lexical_cast(const netlist_paths::VertexType &t) {
    return netlist_paths::getVertexTypeStr(t);
  }
  template<>
  std::string lexical_cast(const netlist_paths::VertexDirection &t) {
    return netlist_paths::getVertexDirectionStr(t);
  }
} // End boost namespace.

Netlist::Netlist() : dp(boost::ignore_other_properties) {
  // Initialise dynamic propery maps for the graph.
  dp.property("id",      boost::get(&VertexProperties::id,      graph));
  dp.property("type",    boost::get(&VertexProperties::type,    graph));
  dp.property("dir",     boost::get(&VertexProperties::dir,     graph));
  dp.property("width",   boost::get(&VertexProperties::width,   graph));
  dp.property("name",    boost::get(&VertexProperties::name,    graph));
  dp.property("loc",     boost::get(&VertexProperties::loc,     graph));
  dp.property("isTop",   boost::get(&VertexProperties::isTop,   graph));
  dp.property("deleted", boost::get(&VertexProperties::deleted, graph));
}

bool Netlist::vertexCompare(const VertexDesc a,
                            const VertexDesc b) const {
  if (graph[a].deleted < graph[b].deleted) return true;
  if (graph[b].deleted < graph[a].deleted) return false;
  if (graph[a].name    < graph[b].name)    return true;
  if (graph[b].name    < graph[a].name)    return false;
  if (graph[a].type    < graph[b].type)    return true;
  if (graph[b].type    < graph[a].type)    return false;
  if (graph[a].dir     < graph[b].dir)     return true;
  if (graph[b].dir     < graph[a].dir)     return false;
  if (graph[a].width   < graph[b].width)   return true;
  if (graph[b].width   < graph[a].width)   return false;
  if (graph[a].loc     < graph[b].loc)     return true;
  if (graph[b].loc     < graph[a].loc)     return false;
  return false;
}

bool Netlist::vertexEqual(const VertexDesc a,
                          const VertexDesc b) const {
  return graph[a].name    == graph[b].name &&
         graph[a].type    == graph[b].type &&
         graph[a].dir     == graph[b].dir &&
         graph[a].width   == graph[b].width &&
         graph[a].loc     == graph[b].loc &&
         graph[a].deleted == graph[b].deleted;
}

///// Simplistic but fast implementation of the GraphViz file parser.
//bool Netlist::parseGraphViz(std::istream &in) {
//  std::string line;
//  while (std::getline(in, line)) {
//    auto openBracePos = line.find_first_of('[');
//    auto closeBracePos = line.find_last_of(']');
//    // Declaration
//    if (line.find("digraph") != std::string::npos &&
//        boost::num_vertices(graph) == 0) {
//      std::vector<std::string> tokens;
//      boost::trim_if(line, boost::is_any_of(" \t"));
//      boost::split(tokens, line,
//                   boost::is_any_of(" \t;"),
//                   boost::token_compress_on);
//      topName.assign(tokens[1]);
//    // Edges
//    } else if (line.find("->") != std::string::npos) {
//      std::vector<std::string> tokens;
//      boost::trim_if(line, boost::is_any_of(" \t"));
//      boost::split(tokens, line,
//                   boost::is_any_of(" \t;"),
//                   boost::token_compress_on);
//      auto src = static_cast<VertexDesc>(std::stoull(tokens[0].substr(1)));
//      auto dst = static_cast<VertexDesc>(std::stoull(tokens[2].substr(1)));
//      boost::add_edge(src, dst, graph);
//    // Vertices
//    } else if (openBracePos != std::string::npos &&
//               closeBracePos != std::string::npos) {
//      auto v = boost::add_vertex(graph);
//      auto nPos = line.find_first_of('n');
//      std::string vertex = line.substr(nPos+1,
//                                       openBracePos-nPos-1);
//      auto vertexNumber = std::stoull(vertex);
//      // Sanity check the format/ordering of the file.
//      assert(boost::num_vertices(graph) == vertexNumber+1);
//      assert(boost::num_edges(graph) == 0);
//      std::string attributes = line.substr(openBracePos+1,
//                                           closeBracePos-openBracePos-1);
//      // Attributes.
//      using tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;
//      tokenizer tokens(attributes,
//                       boost::escaped_list_separator<char>("\\", ",", "\""));
//      for (auto &token : tokens) {
//        // Get Key.
//        auto equalsPos = token.find_first_of('=');
//        auto key = token.substr(0, equalsPos);
//        boost::trim_if(key, boost::is_any_of(" \t"));
//        // Get value.
//        auto quoteFirstPos = token.find_first_of('"');
//        auto quoteLastPos = token.find_last_of('"');
//        std::string value;
//        if (quoteFirstPos != std::string::npos &&
//            quoteLastPos != std::string::npos) {
//          value = token.substr(quoteFirstPos+1, quoteLastPos-quoteFirstPos-1);
//        } else {
//          value = token.substr(equalsPos+1);
//        }
//        // Set the graph attribute.
//        if (key == "id") {
//          auto idNumber = std::stoull(value);
//          assert(vertexNumber == idNumber);
//          graph[v].id = idNumber;
//        } else if (key == "type") {
//          graph[v].type = boost::lexical_cast<VertexType>(value);
//        } else if (key == "dir") {
//          graph[v].dir = boost::lexical_cast<VertexDirection>(value);
//        } else if (key == "width") {
//          graph[v].width = std::stoul(value);
//        } else if (key == "name") {
//          // Top level ports can appear with and without heirarchical paths,
//          // canonicalise their path to merge these vertices as duplicates.
//          graph[v].name.assign(expandName(topName, value));
//        } else if (key == "loc") {
//          graph[v].loc.assign(value);
//        }
//      }
//    }
//  }
//  return true;
//}

/// Parse a graph input file and return a list of Vertices and a list of Edges.
void Netlist::parseFile(const std::string &filename) {
  INFO(std::cout << "Parsing input file\n");
  //std::fstream infile(filename);
  //if (!infile.is_open()) {
  //  throw Exception("could not open file");
  //}
  //if (netlist_paths::options.boostParser) {
  //  // FIXME: this does not set topName from the digraph declaration.
  //  if (!boost::read_graphviz(infile, graph, dp))
  //    throw Exception(std::string("reading graph file: ")+filename);
  //} else {
  //  if (!parseGraphViz(infile))
  //    throw Exception(std::string("reading graph file: ")+filename);
  //}
  //if (!parseVerilatorXML(infile)) {
  //  throw Exception(std::string("reading XML netlist file: ")+filename);
  //}
  //// Initialse other attributes.
  //BGL_FORALL_VERTICES(v, graph, Graph) {
  //  graph[v].deleted = false;
  //  graph[v].isTop = netlist_paths::determineIsTop(graph[v].name);
  //}
  INFO(std::cout << "Netlist contains " << boost::num_vertices(graph)
                 << " vertices and " << boost::num_edges(graph)
                 << " edges\n");
}

/// Remove duplicate vertices from the graph by sorting them comparing each
/// vertex to its neighbours.
void Netlist::mergeDuplicateVertices() {
  std::vector<VertexDesc> vs;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (!isLogic(graph[v]))
      vs.push_back(v);
  }
  auto compare = [this](const VertexDesc a, const VertexDesc b) {
                   return vertexCompare(a, b); };
  std::sort(std::begin(vs), std::end(vs), compare);
  VertexDesc current = vs[0];
  unsigned count = 0;
  for (size_t i=1; i<vs.size(); i++) {
    if (vertexEqual(vs[i], current)) {
      //std::cout << "DUPLICATE VERTEX " << graph[vs[i]].name << "\n";
      BGL_FORALL_ADJ(vs[i], v, graph, Graph) {
        boost::add_edge(current, v, graph);
        boost::remove_edge(vs[i], v, graph);
      }
      // We mark duplicate vertices as deleted since it is expensive to remove
      // them from the graph as vertices are stored in a vecS. Using a listS
      // is less performant.
      graph[vs[i]].deleted = true;
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
    // Source registers don't have in edges.
    if (graph[v].type == VertexType::REG_SRC) {
      if (boost::in_degree(v, graph) > 0)
         std::cout << "Warning: source reg " << graph[v].name
                   << " (" << graph[v].id << ") has in edges" << "\n";
    }
    // Destination registers don't have out edges.
    if (graph[v].type == VertexType::REG_DST) {
      if (boost::out_degree(v, graph) > 0)
        std::cout << "Warning: destination reg " << graph[v].name
                  << " (" << graph[v].id << ") has out edges"<<"\n";
    }
    // NOTE: vertices may be incorrectly marked as reg if a field of a
    // structure has a delayed assignment to a field of it.
  }
}

/// Dump unique names of vars/regs/wires in the netlist for searching.
std::vector<VertexDesc> Netlist::getNames() const {
  std::vector<VertexDesc> vs;
  // Collect vertices.
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (!isLogic(graph[v]) &&
        !isSrcReg(graph[v]) &&
        !canIgnore(graph[v]) &&
        !graph[v].deleted) {
      vs.push_back(v);
    }
  }
  // Sort them.
  auto compare = [this](const VertexDesc a, const VertexDesc b) {
                   return vertexCompare(a, b); };
  std::sort(vs.begin(), vs.end(), compare);
  return vs;
}

void Netlist::printNames(std::vector<VertexDesc> &names) const {
  // Print the output.
  int maxWidth = maxNameLength(names) + 1;
  std::cout << std::left << std::setw(maxWidth) << "Name"
            << std::left << std::setw(10)       << "Type"
            << std::left << std::setw(10)       << "Direction"
            << std::left << std::setw(10)       << "Width"
                                                << "Location\n";
  for (auto v : names) {
    auto type = getVertexTypeStr(graph[v].type);
    auto srcPath = netlist_paths::options.fullFileNames ? fs::path(graph[v].loc)
                                                        : fs::path(graph[v].loc).filename();
    std::cout << std::left << std::setw(maxWidth) << graph[v].name
              << std::left << std::setw(10)       << (std::string(type) == "REG_DST" ? "REG" : type)
              << std::left << std::setw(10)       << getVertexDirectionStr(graph[v].dir)
              << std::left << std::setw(10)       << graph[v].width
                                                  << srcPath.string()
              << "\n";
  }
}

/// Dump a Graphviz dotfile of the netlist graph for visualisation.
void Netlist::dumpDotFile(const std::string &outputFilename) const {
  std::ofstream outputFile(outputFilename);
  if (!outputFile.is_open()) {
    throw Exception(std::string("unable to open ")+outputFilename);
  }
  // Write graphviz format.
  boost::write_graphviz_dp(outputFile, graph, dp, /*node_id=*/"name");
  outputFile.close();
  // Print command line to generate graph file.
  INFO(std::cout << "dot -Tpdf " << outputFilename << " -o graph.pdf\n");
}

/// Lookup a vertex using a regex pattern and function specifying a type.
VertexDesc Netlist::getVertexDesc(const std::string &name,
                                  bool matchVertex (const VertexProperties &p)) const {
  // FIXME: create a list of candidate vertices, rather than iterating all vertices.
  auto nameRegexStr(name);
  // Ignoring '/' (when supplying a heirarchical ref).
  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '_', '.');
  // Or '_' (when supplying a flattened name).
  std::replace(nameRegexStr.begin(), nameRegexStr.end(), '/', '.');
  std::regex nameRegex(nameRegexStr);
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (matchVertex(graph[v])) {
      if (std::regex_search(graph[v].name, nameRegex)) {
        return v;
      }
    }
  }
  return boost::graph_traits<Graph>::null_vertex();
}

// FIXME: Move exception logic into tool.
VertexDesc Netlist::getStartVertexExcept(const std::string &name) const {
  auto vertex = getStartVertex(name);
  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
    throw Exception(std::string("could not find vertex ")+name);
  } else {
    return vertex;
  }
}

// FIXME: Move exception logic into tool.
VertexDesc Netlist::getEndVertexExcept(const std::string &name) const {
  auto vertex = getEndVertex(name);
  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
    throw Exception(std::string("could not find vertex ")+name);
  } else {
    return vertex;
  }
}

// FIXME: Move exception logic into tool.
VertexDesc Netlist::getMidVertexExcept(const std::string &name) const {
  auto vertex = getMidVertex(name);
  if (vertex == boost::graph_traits<Graph>::null_vertex()) {
    throw Exception(std::string("could not find vertex ")+name);
  } else {
    return vertex;
  }
}

void Netlist::dumpPath(const std::vector<VertexDesc> &path) const {
  for (auto v : path) {
    if (!isLogic(graph[v])) {
      std::cout << "  " << graph[v].name << "\n";
    }
  }
}

/// Given the tree structure from a DFS, traverse the tree from leaf to root to
/// return a path.
Path Netlist::determinePath(ParentMap &parentMap,
                                 Path path,
                                 VertexDesc startVertex,
                                 VertexDesc endVertex) const {
  path.push_back(endVertex);
  if (endVertex == startVertex) {
    return path;
  }
  if (parentMap[endVertex].size() == 0)
    return std::vector<VertexDesc>();
  assert(parentMap[endVertex].size() == 1);
  auto nextVertex = parentMap[endVertex].front();
  assert(std::find(std::begin(path),
                   std::end(path),
                   nextVertex) == std::end(path));
  return determinePath(parentMap, path, startVertex, nextVertex);
}

/// Determine all paths between a start and an end point.
/// This performs a DFS starting at the end point. It is not feasible for large
/// graphs since the number of simple paths grows exponentially.
void Netlist::determineAllPaths(ParentMap &parentMap,
                                     std::vector<Path> &result,
                                     Path path,
                                     VertexDesc startVertex,
                                     VertexDesc endVertex) const {
  path.push_back(endVertex);
  if (endVertex == startVertex) {
    INFO(std::cout << "FOUND PATH\n");
    result.push_back(path);
    return;
  }
  INFO(std::cout<<"length "<<path.size()<<" vertex "<<graph[endVertex].id<<"\n");
  INFO(dumpPath(path));
  INFO(std::cout<<(parentMap[endVertex].empty()?"DEAD END\n":""));
  for (auto vertex : parentMap[endVertex]) {
    if (std::find(std::begin(path), std::end(path), vertex) == std::end(path)) {
      determineAllPaths(parentMap, result, path, startVertex, vertex);
    } else {
      INFO(std::cout << "CYCLE DETECTED\n");
    }
  }
}

/// Determine the max length of a name.
int Netlist::maxNameLength(const Path &path) const {
  size_t maxLength = 0;
  for (auto v : path) {
    if (canIgnore(graph[v]))
      continue;
    maxLength = std::max(maxLength, graph[v].name.size());
  }
  return static_cast<int>(maxLength);
}

/// Pretty print a path (some sequence of vertices).
void Netlist::printPathReport(const Path &path) const {
  int maxWidth = maxNameLength(path) + 1;
  // Print each vertex on the path.
  for (auto v : path) {
    if (canIgnore(graph[v]))
      continue;
    auto srcPath = netlist_paths::options.fullFileNames ? fs::path(graph[v].loc)
                                                        : fs::path(graph[v].loc).filename();
    if (!netlist_paths::options.reportLogic) {
      if (!isLogic(graph[v])) {
        std::cout << "  " << std::left
                  << std::setw(maxWidth)
                  << graph[v].name
                  << srcPath.string() << "\n";
      }
    } else {
      if (isLogic(graph[v])) {
        std::cout << "  " << std::left
                  << std::setw(maxWidth)
                  << getVertexTypeStr(graph[v].type)
                  << std::setw(VERTEX_TYPE_STR_MAX_LEN)
                  << "LOGIC"
                  << srcPath.string() << "\n";
      } else {
        std::cout << "  " << std::left
                  << std::setw(maxWidth)
                  << graph[v].name
                  << std::setw(VERTEX_TYPE_STR_MAX_LEN)
                  << getVertexTypeStr(graph[v].type)
                  << srcPath.string() << "\n";
      }
    }
  }
}

/// Print a collection of paths.
void Netlist::
printPathReport(const std::vector<Path> &paths) const {
  int pathCount = 0;
  for (auto &path : paths) {
    if (!path.empty()) {
      std::cout << "Path " << ++pathCount << "\n";
      printPathReport(path);
      std::cout << "\n";
    }
  }
  std::cout << "Found " << pathCount << " path(s)\n";
}

/// Report all paths fanning out from a net/register/port.
std::vector<Path> Netlist::
getAllFanOut(VertexDesc startVertex) const {
  INFO(std::cout << "Performing DFS from "
                 << graph[startVertex].name << "\n");
  ParentMap parentMap;
  boost::depth_first_search(graph,
      boost::visitor(DfsVisitor(parentMap, false))
        .root_vertex(startVertex));
  // Check for a path between startPoint and each register.
  std::vector<Path> paths;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (isEndPoint(graph[v])) {
      auto path = determinePath(parentMap,
                                Path(),
                                startVertex,
                                static_cast<VertexDesc>(graph[v].id));
      if (!path.empty()) {
        std::reverse(std::begin(path), std::end(path));
        paths.push_back(path);
      }
    }
  }
  return paths;
}

std::vector<Path> Netlist::
getAllFanOut(const std::string &startName) const {
  auto startVertex = getStartVertexExcept(startName);
  return getAllFanOut(startVertex);
}

/// Report all paths fanning into a net/register/port.
std::vector<Path> Netlist::
getAllFanIn(VertexDesc endVertex) const {
  auto reverseGraph = boost::make_reverse_graph(graph);
  INFO(std::cout << "Performing DFS in reverse graph from "
                 << graph[endVertex].name << "\n");
  ParentMap parentMap;
  boost::depth_first_search(reverseGraph,
      boost::visitor(DfsVisitor(parentMap, false))
        .root_vertex(endVertex));
  // Check for a path between endPoint and each register.
  std::vector<Path> paths;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    if (isStartPoint(graph[v])) {
      auto path = determinePath(parentMap,
                                Path(),
                                endVertex,
                                static_cast<VertexDesc>(graph[v].id));
      if (!path.empty()) {
        paths.push_back(path);
      }
    }
  }
  return paths;
}

std::vector<Path> Netlist::
getAllFanIn(const std::string &endName) const {
  auto endVertex = getEndVertexExcept(endName);
  return getAllFanIn(endVertex);
}

/// Report a single path between a set of named points.
Path Netlist::
getAnyPointToPoint() const {
  std::vector<VertexDesc> path;
  // Construct the path between each adjacent waypoints.
  for (std::size_t i = 0; i < waypoints.size()-1; ++i) {
    auto startVertex = waypoints[i];
    auto endVertex = waypoints[i+1];
    INFO(std::cout << "Performing DFS from "
                   << graph[startVertex].name << "\n");
    ParentMap parentMap;
    boost::depth_first_search(graph,
        boost::visitor(DfsVisitor(parentMap, false))
          .root_vertex(startVertex));
    INFO(std::cout << "Determining a path to "
                   << graph[endVertex].name << "\n");
    auto subPath = determinePath(parentMap,
                                 Path(),
                                 startVertex,
                                 endVertex);
    if (subPath.empty()) {
      // No path exists.
      return Path();
    }
    std::reverse(std::begin(subPath), std::end(subPath));
    path.insert(std::end(path), std::begin(subPath), std::end(subPath)-1);
  }
  path.push_back(waypoints.back());
  return path;
}

/// Report all paths between start and end points.
std::vector<Path> Netlist::
getAllPointToPoint() const {
  assert(waypoints.size() > 2 && "invlalid waypoints");
  INFO(std::cout << "Performing DFS\n");
  ParentMap parentMap;
  boost::depth_first_search(graph,
      boost::visitor(DfsVisitor(parentMap, true))
        .root_vertex(waypoints[0]));
  INFO(std::cout << "Determining all paths\n");
  std::vector<Path> paths;
  determineAllPaths(parentMap,
                    paths,
                    Path(),
                    waypoints[0],
                    waypoints[1]);
  for (auto &path : paths) {
    std::reverse(std::begin(path), std::end(path));
  }
  return paths;
}

/// Return the number of registers a start point fans out to.
unsigned Netlist::
getfanOutDegree(VertexDesc startVertex) {
  const auto paths = getAllFanOut(startVertex);
  unsigned count = 0;
  for (auto &path : paths) {
    auto endVertex = path.back();
    count += graph[endVertex].width;
  }
  return count;
}

unsigned Netlist::
getfanOutDegree(const std::string &startName) {
  auto startVertex = getStartVertexExcept(startName);
  return getfanOutDegree(startVertex);
}

/// Return he number of registers that fan into an end point.
unsigned Netlist::
getFanInDegree(VertexDesc endVertex) {
  const auto paths = getAllFanIn(endVertex);
  unsigned count = 0;
  for (auto &path : paths) {
    auto startVertex = path.front();
    count += graph[startVertex].width;
  }
  return count;
}

unsigned Netlist::
getFanInDegree(const std::string &endName) {
  auto endVertex = getEndVertexExcept(endName);
  return getFanInDegree(endVertex);
}

/// Check if a path exists between two points.
bool Netlist::
pathExists(const std::string &start, const std::string &end) {
  clearWaypoints();
  // Check that the start and end points exist.
  auto startPoint = getStartVertex(start);
  auto endPoint = getEndVertex(end);
  if (startPoint == boost::graph_traits<Graph>::null_vertex() ||
      endPoint == boost::graph_traits<Graph>::null_vertex()) {
    return false;
  }
  // Check the path exists.
  waypoints.push_back(startPoint);
  waypoints.push_back(endPoint);
  auto path = getAnyPointToPoint();
  return !path.empty();
}
