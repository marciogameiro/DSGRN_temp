# ParameterGraphEcology.py
# Marcio Gameiro
# MIT LICENSE
# 2021-01-29

import DSGRN
from operator import mul
from functools import reduce

class ParameterGraphEcology:
    def size(self):
        # Number of nodes in parameter graph
        return len(self.valid_indices)

    def dimension(self):
        # Phase space dimension
        return self.full_pg.dimension()

    def logicsize(self, d):
        # Number of valid hex codes
        return len(self.factorgraph(d))

    def ordersize(self, d):
        # Order size of full parameter graph
        # Note that depending on the hex code some
        # orders are not allowed, however every order
        # is allowed for at least some hex codes
        return self.full_pg.ordersize(d)

    def factorgraph(self, d):
        return [h for h in self.full_pg.factorgraph(d) if self.valid_hex_code(h, d)]

    def parameter(self, index):
        full_index = self.index2full_index(index)
        return self.full_pg.parameter(full_index)

    def index(self, param):
        full_index = self.full_pg.index(param)
        return self.full_index2index(full_index)

    def adjacencies(self, index):
        full_index = self.index2full_index(index)
        adjs = [self.full_index2index(i) for i in self.full_pg.adjacencies(full_index) if self.valid_index(i)]
        return adjs

    def network(self):
        return self.full_pg.network()

    def fixedordersize(self):
        return reduce(mul, [self.logicsize(d) for d in range(self.dimension())])

    def reorderings(self):
        return reduce(mul, [self.ordersize(d) for d in range(self.dimension())])

    def full_index2index(self, full_index):
        if full_index not in self.valid_indices:
            return -1
        return self.valid_indices.index(full_index)

    def index2full_index(self, index):
        return self.valid_indices[index]

    def __positive_decay(self, d):
        # Check if node d has positive decay
        return self.full_pg.network().decay_sign(d)

    def __instance_single_term_self_edge(self, d):
        # Returns instance of single term self-edge
        # Returns -1 if no single term self-edge
        logic_struct = self.full_pg.network().logic(d)
        if [d] in logic_struct: # If single term self-edge
            # Get term index in list of logic terms
            term_index = logic_struct.index([d])
            # Get all in-edges before [d] in logic_struct
            in_nodes = [source for i in range(term_index) for source in logic_struct[i]]
            instance = in_nodes.count(d) # Instance of single term self-edge [d]
            return instance
        return -1

    def __single_term_negative_self_edge(self, d):
        # Check if node d has a single term negative self-edge
        # Instance of single term self-edge [d]
        instance = self.__instance_single_term_self_edge(d)
        if instance == -1: # No single term self-edge
            return False
        # If single term self-edge is negative
        if not self.full_pg.network().edge_sign(d, d, instance):
            return True
        return False

    def __single_term_negative_self_edge_old(self, d):
        # Check if node d has a single term negative self-edge
        for term in self.full_pg.network().logic_by_index(d):
            # If single term get edge = term[0] and check
            # if it is a self-edge and if it is negative
            if (len(term) == 1) and (term[0][0] == d) and (term[0][2] == False):
                return True
        return False

    def __single_term_self_edge_threshold(self, d):
        # Get index of single term negative self-edge threshold
        # If no single term negative self-edge
        if not self.__single_term_negative_self_edge(d):
            return -1
        # Instance of single term self-edge [d]
        instance = self.__instance_single_term_self_edge(d)
        # Out edge order of single term self-edge [d]
        out_order = self.full_pg.network().order(d, d, instance)
        return out_order

    def __single_term_negative_self_edge_polynomial(self, d):
        # Get j such that pj equals the delta corresponding
        # to the single term negative self-edge
        # If no single term negative self-edge
        if not self.__single_term_negative_self_edge(d):
            return -1
        logic_struct = self.full_pg.network().logic(d)
        # Get the [d] term index in list of logic terms
        term_index = logic_struct.index([d])
        # Get all in-edges before [d] in logic_struct
        in_nodes = [source for i in range(term_index) for source in logic_struct[i]]
        in_index = len(in_nodes) # Index of single term self-edge [d] in input list
        return 2**in_index # pj equals delta for j = 2**in_index

    def __valid_hex_code(self, hex_code, d, check_thres=False):
        """Check if the hex_code is valid if node d has
        positive decay and a negative single term self-edge
        (the hex_code is automatically valid otherwise).
        If check_thres is True it checks if the carrying
        capacity threshold is below the pj corresponding
        to the carrying capacity delta. Otherwise it only
        checks if there is at least one thrshold below pj.
        """
        # All hex codes are valid for negative decay
        if not self.__positive_decay(d):
            return True
        # Hex code is valid if no negative single term self-edge
        if not self.__single_term_negative_self_edge(d):
            return True
        # Get number of inputs and outputs
        n_inputs = len(self.full_pg.network().inputs(d))
        n_outputs = len(self.full_pg.network().outputs(d))
        # Compute partial order corresponding to hex_code
        partial_order = DSGRN.hex2partial(hex_code, n_inputs, n_outputs)
        # Get j such that pj equals the carrying capacity delta
        j = self.__single_term_negative_self_edge_polynomial(d)
        # Index of pj in partial order
        pj_index = partial_order.index(j)
        if not check_thres: # Just need a threshold before pj
            # If any threshold before pj
            if any(partial_order[i] < 0 for i in range(pj_index)):
                return True
            return False
        # Original (before permutation) negative single
        # term self-edge threshold index
        thres_index = self.__single_term_self_edge_threshold(d)
        # Negative value t representing this threshold in partial order
        t = thres_index - n_outputs
        # If the threshold t is before pj is partial order
        if any(partial_order[i] == t for i in range(pj_index)):
            return True
        return False

    def valid_hex_code(self, hex_code, d):
        return self.__valid_hex_code(hex_code, d)

    def valid_parameter(self, parameter):
        for d in range(self.dimension()):
            hex_code = parameter.logic()[d].hex()
            if not self.__valid_hex_code(hex_code, d, check_thres=True):
                return False
        return True

    def valid_index(self, full_index):
        return self.valid_parameter(self.full_pg.parameter(full_index))

    def __init__(self, network):
        # Initialize full parameter graph
        self.full_pg = DSGRN.ParameterGraph(network)
        self.valid_indices = [i for i in range(self.full_pg.size()) if self.valid_index(i)]
