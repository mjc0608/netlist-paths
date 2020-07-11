#ifndef NETLIST_PATHS_NETLIST_HPP
#define NETLIST_PATHS_NETLIST_HPP

#include <string>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/tokenizer.hpp>
#include "netlist_paths/DTypes.hpp"
#include "netlist_paths/Vertex.hpp"

namespace netlist_paths {

using Graph = boost::adjacency_list<boost::vecS,
                                    boost::vecS,
                                    boost::bidirectionalS,
                                    Vertex>;
using VertexDesc = boost::graph_traits<Graph>::vertex_descriptor;
using ParentMap = std::map<VertexDesc, std::vector<VertexDesc>>;
using Path = std::vector<VertexDesc>;

class Netlist {
private:
  Graph graph;
  std::string topName;
  std::vector<File> files;
  std::vector<DType> dtypes;
  std::vector<VertexDesc> waypoints;

  bool vertexCompare(const VertexDesc a, const VertexDesc b) const;
  bool vertexEqual(const VertexDesc a, const VertexDesc b) const;

  VertexDesc getVertexDesc(const std::string &name,
                           bool matchVertex (const Vertex&)) const;
  void dumpPath(const Path &path) const;
  Path determinePath(ParentMap &parentMap,
                     Path path,
                     VertexDesc startVertexId,
                     VertexDesc endVertexId) const;
  void determineAllPaths(ParentMap &parentMap,
                         std::vector<Path> &result,
                         Path path,
                         VertexDesc startVertex,
                         VertexDesc endVertex) const;

public:
  Netlist() {}
  std::shared_ptr<File> addFile(File file) {
    files.push_back(file);
    return std::make_shared<File>(files.back());
  }
  std::shared_ptr<DType> addDtype(DType dtype) {
    dtypes.push_back(dtype);
    return std::make_shared<DType>(dtypes.back());
  }
  VertexDesc addLogicVertex(VertexType type, Location location) {
    auto vertex = Vertex(type, location);
    return boost::add_vertex(vertex, graph);
  }
  VertexDesc addVarVertex(VertexType type,
                          VertexDirection direction,
                          Location location,
                          std::shared_ptr<DType> dtype,
                          const std::string &name,
                          bool isParam,
                          const std::string &paramValue) {
    auto vertex = Vertex(type, direction, location, dtype, name, isParam, paramValue);
    return boost::add_vertex(vertex, graph);
  }
  void addEdge(VertexDesc src, VertexDesc dst) {
    boost::add_edge(src, dst, graph);
  }
  void setVertexReg(VertexDesc vertex) {
    graph[vertex].type = VertexType::REG_DST;
  }
  std::size_t numVertices() { return boost::num_vertices(graph); }
  std::size_t numEdges() { return boost::num_edges(graph); }
  void mergeDuplicateVertices();
  void checkGraph() const;
  //void dumpDotFile(const std::string &outputFilename) const;
  //int maxNameLength(const Path &path) const;
  //std::vector<VertexDesc> getNames() const;
  //void printNames(std::vector<VertexDesc> &names) const;
  //void printPathReport(const Path &path) const;
  //void printPathReport(const std::vector<Path> &paths) const;
  //VertexDesc getStartVertex(const std::string &name) const {
  //  return getVertexDesc(name, isStartPoint);
  //}
  //VertexDesc getEndVertex(const std::string &name) const {
  //  return getVertexDesc(name, isEndPoint);
  //}
  //VertexDesc getMidVertex(const std::string &name) const {
  //  return getVertexDesc(name, isMidPoint);
  //}
  //VertexDesc getRegVertex(const std::string &name) const {
  //  return getVertexDesc(name, isReg);
  //}
  //VertexDesc getStartVertexExcept(const std::string &name) const;
  //VertexDesc getEndVertexExcept(const std::string &name) const;
  //VertexDesc getMidVertexExcept(const std::string &name) const;
  //std::vector<Path> getAllFanOut(VertexDesc startVertex) const;
  //std::vector<Path> getAllFanOut(const std::string &startName) const;
  //std::vector<Path> getAllFanIn(VertexDesc endVertex) const;
  //std::vector<Path> getAllFanIn(const std::string &endName) const;
  //unsigned getfanOutDegree(VertexDesc startVertex);
  //unsigned getfanOutDegree(const std::string &startName);
  //unsigned getFanInDegree(VertexDesc endVertex);
  //unsigned getFanInDegree(const std::string &endName);
  //Path getAnyPointToPoint() const;
  //std::vector<Path> getAllPointToPoint() const;
  //void addStartpoint(const std::string &name) {
  //  waypoints.push_back(getStartVertex(name));
  //}
  //void addEndpoint(const std::string &name) {
  //  waypoints.push_back(getEndVertex(name));
  //}
  //void addWaypoint(const std::string &name) {
  //  waypoints.push_back(getMidVertex(name));
  //}
  //std::size_t numWaypoints() const { return waypoints.size(); }
  //void clearWaypoints() { waypoints.clear(); }
  //const std::string &getVertexName(VertexDesc vertex) const {
  //  return graph[vertex].name;
  //}
  //bool startpointExists(const std::string &name) const noexcept {
  //  return getStartVertex(name) != boost::graph_traits<Graph>::null_vertex();
  //}
  //bool endpointExists(const std::string &name) const noexcept {
  //  return getEndVertex(name) != boost::graph_traits<Graph>::null_vertex();
  //}
  //bool regExists(const std::string &name) const noexcept {
  //  return getRegVertex(name) != boost::graph_traits<Graph>::null_vertex();
  //}
  //bool pathExists(const std::string &start, const std::string &end);
};

} // End namespace.

#endif // NETLIST_PATHS_NETLIST_HPP
