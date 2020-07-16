#ifndef NETLIST_PATHS_NETLIST_PATHS_HPP
#define NETLIST_PATHS_NETLIST_PATHS_HPP

#include <memory>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

namespace netlist_paths {

/// Wrapper for Python to manage the netlist paths object.
class NetlistPaths {
  std::unique_ptr<Netlist> netlist;
  std::vector<VertexDesc> waypoints;
  int maxNameLength(const Path &path) const;
public:
  NetlistPaths() = delete;
  NetlistPaths(const std::string &filename) :
      netlist(ReadVerilatorXML().readXML(filename)) {
    netlist->mergeDuplicateVertices();
    netlist->checkGraph();
  }
  /// Waypoints.
  void addStartpoint(const std::string &name) {
    waypoints.push_back(netlist->getStartVertex(name));
  }
  void addEndpoint(const std::string &name) {
    waypoints.push_back(netlist->getEndVertex(name));
  }
  void addWaypoint(const std::string &name) {
    waypoints.push_back(netlist->getMidVertex(name));
  }
  std::size_t numWaypoints() const { return waypoints.size(); }
  void clearWaypoints() { waypoints.clear(); }
  /// Names and types.
  void printNames() const;
  /// Basic path querying.
  bool startpointExists(const std::string &name) const noexcept {
    return netlist->getStartVertex(name) != netlist->nullVertex();
  }
  bool endpointExists(const std::string &name) const noexcept {
    return netlist->getEndVertex(name) != netlist->nullVertex();
  }
  bool regExists(const std::string &name) const noexcept {
    return netlist->getRegVertex(name) != netlist->nullVertex();
  }
};

}; // End namespace.

#endif // NETLIST_PATHS_NETLIST_PATHS_HPP
