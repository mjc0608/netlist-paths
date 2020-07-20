import os
import sys
import unittest
import definitions as defs
sys.path.insert(0, os.path.join(defs.BINARY_DIR_PREFIX, 'lib', 'netlist_paths'))
import py_netlist_paths

class TeastPyWrapper(unittest.TestCase):

    def setUp(self):
        pass

    def test_verilator_bin(self):
        self.assertTrue(os.path.exists(defs.INSTALL_PREFIX))

    def test_adder(self):
        comp = py_netlist_paths.RunVerilator(defs.INSTALL_PREFIX)
        comp.run(os.path.join(defs.TEST_SRC_PREFIX, 'adder.sv'), 'netlist.xml')
        netlist = py_netlist_paths.NetlistPaths('netlist.xml')
        # Check all valid paths are reported.
        self.assertTrue(netlist.path_exists('adder.i_a', 'adder.o_sum'))
        self.assertTrue(netlist.path_exists('adder.i_a', 'adder.o_co'))
        self.assertTrue(netlist.path_exists('adder.i_b', 'adder.o_sum'))
        self.assertTrue(netlist.path_exists('adder.i_b', 'adder.o_co'))
        # Check for invalid paths.
        self.assertFalse(netlist.path_exists('adder.o_sum', 'adder.i_a'))
        self.assertFalse(netlist.path_exists('adder.o_co',  'adder.i_a'))
        self.assertFalse(netlist.path_exists('adder.o_sum', 'adder.i_b'))
        self.assertFalse(netlist.path_exists('adder.o_co',  'adder.i_b'))
        # Check again, using the non-prefixed top names (these should resolve
        # to the prefixed names, and vice versa).
        self.assertTrue(netlist.path_exists('i_a', 'o_sum'))
        self.assertTrue(netlist.path_exists('i_a', 'o_co'))
        self.assertTrue(netlist.path_exists('i_b', 'o_sum'))
        self.assertTrue(netlist.path_exists('i_b', 'o_co'))
        self.assertFalse(netlist.path_exists('o_sum', 'i_a'))
        self.assertFalse(netlist.path_exists('o_co',  'i_a'))
        self.assertFalse(netlist.path_exists('o_sum', 'i_b'))
        self.assertFalse(netlist.path_exists('o_co',  'i_b'))

    def test_counter(self):
        comp = py_netlist_paths.RunVerilator(defs.INSTALL_PREFIX)
        comp.run(os.path.join(defs.TEST_SRC_PREFIX, 'counter.sv'), 'netlist.xml')
        netlist = py_netlist_paths.NetlistPaths('netlist.xml')
        # Register can be start or end point.
        self.assertTrue(netlist.reg_exists('counter_q'))
        self.assertTrue(netlist.startpoint_exists('counter_q'))
        self.assertTrue(netlist.endpoint_exists('counter_q'))
        # Output port can be endpoint only.
        self.assertFalse(netlist.reg_exists('counter.o_count'))
        self.assertTrue(netlist.endpoint_exists('counter.o_count'))
        self.assertTrue(netlist.endpoint_exists('o_count'))
        # A name that doesn't exist.
        self.assertFalse(netlist.reg_exists('foo'))
        self.assertFalse(netlist.startpoint_exists('foo'))
        self.assertFalse(netlist.endpoint_exists('foo'))
        # Check all valid paths are reported.
        self.assertTrue(netlist.path_exists('counter.i_clk',     'counter.counter_q'));
        self.assertTrue(netlist.path_exists('counter.i_rst',     'counter.counter_q'));
        self.assertTrue(netlist.path_exists('counter.counter_q', 'counter.o_count'));
        # Check invalid paths.
        self.assertFalse(netlist.path_exists('counter.o_count', 'counter.counter_q'));
        self.assertFalse(netlist.path_exists('counter.count_q', 'counter.i_clk'));
        self.assertFalse(netlist.path_exists('counter.count_q', 'counter.i_rst'));

if __name__ == '__main__':
    unittest.main()
