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
  BOOST_CHECK_NO_THROW(compile("adder.sv", "adder"));
  // Check paths between all start and end points are reported.
  auto startPoints = {"i_a", "i_b"};
  auto endPoints = {"o_sum", "o_co"};
  for (auto s : startPoints) {
    for (auto e : endPoints) {
      BOOST_TEST(pathExists(s, e));
      BOOST_TEST(!pathExists(e, s));
    }
  }
}

BOOST_FIXTURE_TEST_CASE(counter, TestContext) {
  BOOST_CHECK_NO_THROW(compile("counter.sv", "counter"));
  // Registers.
  BOOST_TEST(regExists("counter/counter_q"));
  BOOST_TEST(regExists("counter_counter_q"));
  // Regex register.
  BOOST_TEST(regExists("counter_q"));
  // Paths
  BOOST_TEST(pathExists("i_clk", "counter.counter_q"));
  BOOST_TEST(pathExists("i_rst", "counter.counter_q"));
  BOOST_TEST(pathExists("counter.counter_q", "o_count"));
  BOOST_TEST(pathExists("counter.counter_q", "o_wrap"));
  BOOST_TEST(!pathExists("i_clk", "o_count"));
  BOOST_TEST(!pathExists("i_rst", "o_counto_count"));
  BOOST_TEST(!pathExists("i_clk", "o_wrap"));
  BOOST_TEST(!pathExists("i_rst", "o_wrap"));
  // TODO: check --from o_counter has no fan out paths
}

BOOST_FIXTURE_TEST_CASE(fsm, TestContext) {
  BOOST_CHECK_NO_THROW(compile("fsm.sv", "fsm"));
}

BOOST_FIXTURE_TEST_CASE(mux2, TestContext) {
  BOOST_CHECK_NO_THROW(compile("mux2.sv", "mux2"));
}

BOOST_FIXTURE_TEST_CASE(pipeline_module, TestContext) {
  BOOST_CHECK_NO_THROW(compile("pipeline_module.sv", "pipeline"));
  // Registers
  BOOST_TEST(regExists("pipeline.g_pipestage\\[0\\].u_pipestage.data_q")); // Hier dot
  BOOST_TEST(regExists("pipeline/g_pipestage\\[0\\]/u_pipestage/data_q")); // Hier slash
  BOOST_TEST(regExists("pipeline_g_pipestage\\[0\\]_u_pipestage_data_q")); // Flat
  BOOST_TEST(regExists("pipeline/g_pipestage\\[0\\]_u_pipestage_data_q")); // Mixed
  BOOST_TEST(regExists("g_pipestage\\[0\\]/u_pipestage_data_q")); // Mixed
  // Regexes
  BOOST_TEST(regExists("pipeline/.*/u_pipestage_data_q"));
  BOOST_TEST(regExists("pipeline/.*/data_q"));
  // Paths
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[0\\].u_pipestage.data_q", "pipeline.g_pipestage\\[1\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[1\\].u_pipestage.data_q", "pipeline.g_pipestage\\[2\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[2\\].u_pipestage.data_q", "pipeline.g_pipestage\\[3\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[3\\].u_pipestage.data_q", "pipeline.g_pipestage\\[4\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[4\\].u_pipestage.data_q", "pipeline.g_pipestage\\[5\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[5\\].u_pipestage.data_q", "pipeline.g_pipestage\\[6\\].u_pipestage.data_q"));
  BOOST_TEST(pathExists("pipeline.g_pipestage\\[6\\].u_pipestage.data_q", "pipeline.g_pipestage\\[7\\].u_pipestage.data_q"));
}

BOOST_FIXTURE_TEST_CASE(vlvbound, TestContext) {
  BOOST_CHECK_NO_THROW(compile("vlvbound.sv", "vlvbound_test"));
  // Test that the inlined tasks do not share a merged VlVbound node.
  // See https://www.veripool.org/boards/3/topics/2619
  BOOST_TEST( pathExists("i_foo_current", "o_foo_inactive"));
  BOOST_TEST( pathExists("i_foo_next",    "o_next_foo_inactive"));
  BOOST_TEST(!pathExists("i_foo_current", "o_next_foo_inactive"));
  BOOST_TEST(!pathExists("i_foo_next",    "o_foo_inactive"));
}
