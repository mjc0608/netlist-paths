# Netlist paths

This is a command line tool for querying paths in a Verilog netlist.

## Compile and install

Install dependencies:
```
C/C++ compiler
CMake (minimum 3.12.0)
Boost (minimum 1.68.0)
Python
```
Install Verilator dependencies:
```
make
autoconf
flex
bison
```
Then:
```
git submodule update --init --recursive
mkdir Debug
cd Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
make install
ctest . --verbose
```

## Debugging

Produce XML from a test case:
```
netlist-paths  --compile tests/verilog/adder.sv --outfile out.xml --verbose --debug
Running: ".../np-verilator_bin" +1800-2012ext+.sv --bbox-sys --bbox-unsup \
  --xml-only --flatten --error-limit 10000 --xml-output out.xml tests/verilog/adder.sv
```
Produce a visualisation of the netlist graph:
```
netlist-paths out.xml --verbose --debug --dotfile --outfile graph.dot
dot -Tpdf graph.dot -o graph.pdf
```
Run ``pytest`` directly:
```
cd Debug
pytest tests/py_wrapper_tests.py
```
