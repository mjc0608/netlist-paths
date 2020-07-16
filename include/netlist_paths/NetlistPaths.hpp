#ifndef NETLIST_PATHS_NETLIST_PATHS_HPP
#define NETLIST_PATHS_NETLIST_PATHS_HPP

#include <memory>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"

namespace netlist_paths {

/// Wrapper for Python to manage the netlist paths object.
class NetlistPaths {
  std::unique_ptr<Netlist> netlist;
public:
  NetlistPaths() = delete;
  NetlistPaths(const std::string &filename) :
      netlist(ReadVerilatorXML().readXML(filename)) {}
};

}; // End namespace.

#endif // NETLIST_PATHS_NETLIST_PATHS_HPP
