#ifndef NETLIST_PATHS_WAYPOINTS_HPP
#define NETLIST_PATHS_WAYPOINTS_HPP

#include "netlist_paths/Exception.hpp"
#include "netlist_paths/Vertex.hpp"

namespace netlist_paths {

class Waypoints {
  std::vector<std::string> waypoints;
  bool got_start_point;
  bool got_finish_point;

public:
  Waypoints() : got_start_point(false), got_finish_point(false) {}

  Waypoints(const std::string start,
            const std::string finish) :
      got_start_point(false), got_finish_point(false) {
    addStartPoint(start);
    addFinishPoint(finish);
  }

  void addStartPoint(const std::string name) {
    if (got_start_point) {
      throw Exception("start point already defined");
    }
    got_start_point = true;
    if (waypoints.size() > 0) {
      waypoints.insert(waypoints.begin(), name);
    } else {
      waypoints.push_back(name);
    }
  }

  void addFinishPoint(const std::string name) {
    if (got_finish_point) {
      throw Exception("finish point already defined");
    }
    got_finish_point = true;
    if (waypoints.size() > 0) {
      waypoints.insert(waypoints.end(), name);
    } else {
      waypoints.push_back(name);
    }
  }

  void addThroughPoint(const std::string name) {
    if (waypoints.size() > 0) {
      waypoints.insert(waypoints.end()-(got_finish_point?1:0), name);
    } else {
      waypoints.push_back(name);
    }
  }

  std::vector<std::string>::iterator begin() { return waypoints.begin(); }
  std::vector<std::string>::iterator end() { return waypoints.end(); }
  bool empty() const { return waypoints.empty(); }
  size_t size() const { return waypoints.size(); }
};

}; // End namespace.

#endif // NETLIST_PATHS_WAYPOINTS_HPP
