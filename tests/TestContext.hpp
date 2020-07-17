#ifndef NETLIST_PATHS_TEST_CONTEXT_HPP
#define NETLIST_PATHS_TEST_CONTEXT_HPP

#include <string>
#include <vector>
#include <boost/test/unit_test.hpp>
#include <netlist_paths/NetlistPaths.hpp>

struct TestContext {
  TestContext() {}
  netlist_paths::Netlist netlist;
  /// Compile a test and create a netlist object.
  void compile(const std::string &inFilename) {
    auto testPath = fs::path(testPrefix) / inFilename;
    std::vector<std::string> includes = {};
    std::vector<std::string> defines = {};
    std::vector<std::string> inputFiles = {testPath.string()};
    netlist_paths::RunVerilator runVerilator(installPrefix);
    auto outTemp = fs::unique_path();
    runVerilator.run(includes,
                     defines,
                     inputFiles,
                     outTemp.native());
    auto netlistPaths = netlist_paths::NetlistPaths(outTemp.native());
    fs::remove(outTemp);
  }
  /// Check all names are unique.
  void uniqueNames() {
    //auto vertices = netlist.getNames();
    //std::vector<std::string> names;
    //for (auto v : vertices)
    //  names.push_back(netlist.getVertexName(v));
    //auto last = std::unique(std::begin(names), std::end(names));
    //names.erase(last, std::end(names));
    //BOOST_TEST(vertices.size() == names.size());
  }
  /// Check all names are qualified with the top module name.
  void qualifiedNames(const char *topName) {
    //for (auto v : netlist.getNames()) {
    //  BOOST_TEST(boost::starts_with(netlist.getVertexName(v), topName));
    //}
  }
  //bool pathExists(const std::string &start,
  //                const std::string &end) { return netlist.pathExists(start, end); }
};

#endif // NETLIST_PATHS_TEST_CONTEXT_HPP
