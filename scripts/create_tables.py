#!/usr/bin/python2

"""
Generate C++ header files with tables for quadrature nodes and weights for
different orders.
"""

from orthogonal import psi_roots

def generate_gausshermite(filename, orders):
    f = open(filename, "w")
    f.write("#pragma once\n\n")
    f.write("// Automatically generated by scripts/create_tables.py.\n\n")
    f.write("#include <vector>\n\n")
    f.write('#include "quadrature_rule.hpp"\n\n')
    f.write("namespace waveblocks {\n\n")
    f.write("// Gauss-Hermite quadrature nodes and weights for different orders.\n")
    f.write("const std::vector<QuadratureRule> gauss_hermite_rules{")
    for order in orders:
        nodes, weights = psi_roots(order)
        if order > 1: f.write(",")
        f.write("\n    QuadratureRule(" + str(order) + ", {")
        # Write nodes.
        for i in xrange(order):
            if i > 0: f.write(",")
            f.write("\n        " + str(nodes[i]))
        f.write("\n    }, {")
        # Write weights.
        for i in xrange(order):
            if i > 0: f.write(",")
            f.write("\n        " + str(weights[i]))
        f.write("\n    })")
    f.write("\n};\n")

    f.write("\n}\n")
    f.close()

if __name__ == "__main__":
    orders = (list(range(1, 25)) +
              list(range(25, 100, 5)) +
              list(range(100, 551, 25)))

    generate_gausshermite("../waveblocks/tables_gausshermite.hpp", orders)
