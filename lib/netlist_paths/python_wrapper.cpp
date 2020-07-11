#include <boost/python.hpp>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/RunVerilator.hpp"

BOOST_PYTHON_MODULE(py_netlist_paths)
{
  using namespace boost::python;
  using namespace netlist_paths;

  int (RunVerilator::*run)(const std::string&, const std::string&) const = &RunVerilator::run;

  class_<RunVerilator>("RunVerilator")
    .def(init<const std::string&>())
    .def("run", run);

  class_<Netlist>("Netlist");
  //  //.def("parse_file",        &Netlist::parseFile)
  //  .def("reg_exists",        &Netlist::regExists)
  //  .def("startpoint_exists", &Netlist::startpointExists)
  //  .def("endpoint_exists",   &Netlist::endpointExists)
  //  .def("path_exists",       &Netlist::pathExists);
}
