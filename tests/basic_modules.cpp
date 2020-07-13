#define BOOST_TEST_MODULE compile_graph

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <fstream>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "netlist_paths/Netlist.hpp"
#include "netlist_paths/Options.hpp"
#include "netlist_paths/ReadVerilatorXML.hpp"
#include "netlist_paths/RunVerilator.hpp"
#include "tests/definitions.hpp"
#include "TestContext.hpp"

namespace fs = boost::filesystem;

netlist_paths::Options options;

BOOST_FIXTURE_TEST_CASE(verilator, TestContext) {
  // Check the Verilator binary exists.
  BOOST_ASSERT(boost::filesystem::exists(installPrefix));
}

BOOST_FIXTURE_TEST_CASE(adder, TestContext) {
  BOOST_CHECK_NO_THROW(compile("adder.sv"));
  uniqueNames();
  qualifiedNames("adder");
  //// Check paths between all start and end points are reported.
  //auto startPoints = {"adder.i_a", "adder.i_b"};
  //auto endPoints = {"adder.o_sum", "adder.o_co"};
  //for (auto s : startPoints) {
  //  for (auto e : endPoints) {
  //    BOOST_TEST(pathExists(s, e));
  //    BOOST_TEST(!pathExists(e, s));
  //  }
  //}
}

BOOST_FIXTURE_TEST_CASE(counter, TestContext) {
  BOOST_CHECK_NO_THROW(compile("counter.sv"));
  uniqueNames();
  qualifiedNames("counter");
  //// Registers.
  //BOOST_TEST(netlist.regExists("counter/counter_q"));
  //BOOST_TEST(netlist.regExists("counter_counter_q"));
  //// Regex register.
  //BOOST_TEST(netlist.regExists("counter_q"));
  //// Paths
  //BOOST_TEST(pathExists("counter.i_clk", "counter.counter_q"));
  //BOOST_TEST(pathExists("counter.i_rst", "counter.counter_q"));
  //BOOST_TEST(pathExists("counter.counter_q", "counter.o_count"));
  //BOOST_TEST(!pathExists("counter.counter_q", "counter.o_wrap"));
  //// TODO: check --from o_counter has no fan out paths
}

BOOST_FIXTURE_TEST_CASE(fsm, TestContext) {
  BOOST_CHECK_NO_THROW(compile("fsm.sv"));
  uniqueNames();
  qualifiedNames("fsm");
}

BOOST_FIXTURE_TEST_CASE(mux2, TestContext) {
  BOOST_CHECK_NO_THROW(compile("mux2.sv"));
  uniqueNames();
  qualifiedNames("mux2");
}

BOOST_FIXTURE_TEST_CASE(pipeline_module, TestContext) {
  BOOST_CHECK_NO_THROW(compile("pipeline_module.sv"));
  uniqueNames();
  qualifiedNames("pipeline");
  // Registers
  //BOOST_TEST(netlist.regExists("pipeline.g_pipestage\\[0\\].u_pipestage.data_q")); // Hier dot
  //BOOST_TEST(netlist.regExists("pipeline/g_pipestage\\[0\\]/u_pipestage/data_q")); // Hier slash
  //BOOST_TEST(netlist.regExists("pipeline_g_pipestage\\[0\\]_u_pipestage_data_q")); // Flat
  //BOOST_TEST(netlist.regExists("pipeline/g_pipestage\\[0\\]_u_pipestage_data_q")); // Mixed
  //BOOST_TEST(netlist.regExists("g_pipestage\\[0\\]/u_pipestage_data_q")); // Mixed
  //// Regexes
  //BOOST_TEST(netlist.regExists("pipeline/.*/u_pipestage_data_q"));
  //BOOST_TEST(netlist.regExists("pipeline/.*/data_q"));
  //// Paths
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[0\\].u_pipestage.data_q", "pipeline.g_pipestage\\[1\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[1\\].u_pipestage.data_q", "pipeline.g_pipestage\\[2\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[2\\].u_pipestage.data_q", "pipeline.g_pipestage\\[3\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[3\\].u_pipestage.data_q", "pipeline.g_pipestage\\[4\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[4\\].u_pipestage.data_q", "pipeline.g_pipestage\\[5\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[5\\].u_pipestage.data_q", "pipeline.g_pipestage\\[6\\].u_pipestage.data_q"));
  //BOOST_TEST(pathExists("pipeline.g_pipestage\\[6\\].u_pipestage.data_q", "pipeline.g_pipestage\\[7\\].u_pipestage.data_q"));
}

BOOST_FIXTURE_TEST_CASE(vlvbound, TestContext) {
  BOOST_CHECK_NO_THROW(compile("vlvbound.sv"));
  // Test that the inlined tasks do not share a merged VlVbound node.
  // See https://www.veripool.org/boards/3/topics/2619
  //BOOST_TEST( pathExists("i_foo_current", "o_foo_inactive"));
  //BOOST_TEST( pathExists("i_foo_next",    "o_next_foo_inactive"));
  //BOOST_TEST(!pathExists("i_foo_current", "o_next_foo_inactive"));
  //BOOST_TEST(!pathExists("i_foo_next",    "o_foo_inactive"));
}
