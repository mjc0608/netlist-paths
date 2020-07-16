#include "netlist_paths/NetlistPaths.hpp"

using namespace netlist_paths;

/// Determine the max length of a name.
int NetlistPaths::maxNameLength(const std::vector<VertexDesc> &names) const {
  size_t maxLength = 0;
  for (auto v : names) {
    if (netlist->getVertex(v).canIgnore()) {
      continue;
    }
    maxLength = std::max(maxLength, netlist->getVertex(v).name.size());
  }
  return static_cast<int>(maxLength);
}

void NetlistPaths::printNames() const {
  //auto names = netlist->getNames();
  //// Print the output.
  //int maxWidth = netlist->maxNameLength(names) + 1;
  //std::cout << std::left << std::setw(maxWidth) << "Name"
  //          << std::left << std::setw(10)       << "Type"
  //          << std::left << std::setw(10)       << "Direction"
  //          << std::left << std::setw(10)       << "Width"
  //                                              << "Location\n";
  //for (auto v : names) {
  //  auto type = getVertexAstTypeStr(graph[v].astType);
  //  auto srcPath = netlist_paths::options.fullFileNames ? fs::path(graph[v].location.getFilename())
  //                                                      : fs::path(graph[v].location.getFilename()).filename();
  //  std::cout << std::left << std::setw(maxWidth) << graph[v].name
  //            << std::left << std::setw(10)       << (std::string(type) == "REG_DST" ? "REG" : type)
  //            << std::left << std::setw(10)       << getVertexDirectionStr(graph[v].direction)
  //            //<< std::left << std::setw(10)       << graph[v].width
  //                                                << srcPath.string()
  //            << "\n";
  //}
}
