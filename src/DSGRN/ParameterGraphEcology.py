# ParameterGraphEcology.py
# Marcio Gameiro
# MIT LICENSE
# 2021-02-09

import DSGRN
from operator import mul
from functools import reduce
from math import factorial

class ParameterGraphEcology:
    def size(self):
        # Number of nodes in parameter graph
        # return reduce(mul, [self.factorgraph_size(d) for d in range(self.dimension())])
        return self.place_values[-1]

    def dimension(self):
        # Phase space dimension
        return self.dim

    def logicsize(self, d):
        # Number of valid hex codes
        return len(self.factorgraph(d))

    def ordersize(self, d):
        # Note that, for filter_level 1, depending
        # on the hex code some orders are not allowed,
        # however every order in orders[d] is allowed
        # for at least one hex code. For filter_level
        # 2 every order is allowed for every hex code.
        return len(self.orders[d])

    def factorgraph(self, d):
        return self.hex_codes[d]

    def permutations(self, d):
        m = len(self.full_pg.network().outputs(d))
        perms = [DSGRN.OrderParameter(m, k).permutation() for k in self.orders[d]]
        return perms

    def hex_order_factorgraph(self, d):
        return self.hex_order_pairs[d]

    def parameter(self, index):
        full_index = self.index2full_index(index)
        if full_index == -1:
            return -1
        return self.full_pg.parameter(full_index)

    def index(self, parameter):
        full_index = self.full_pg.index(parameter)
        return self.full_index2index(full_index)

    def adjacencies(self, index):
        full_index = self.index2full_index(index)
        if full_index == -1:
            return -1
        full_adjs = self.full_pg.adjacencies(full_index)
        adjs = {self.full_index2index(i) for i in full_adjs if self.valid_full_index(i)}
        return sorted(list(adjs))

    def network(self):
        return self.full_pg.network()

    def fixedordersize(self):
        return reduce(mul, [self.logicsize(d) for d in range(self.dim)])

    def reorderings(self):
        return reduce(mul, [self.ordersize(d) for d in range(self.dim)])

    def factorgraph_size(self, d):
        return self.factor_sizes[d]

    def factor_coords2index(self, factor_coords):
        # Parameter index from factor graph coordinates
        return sum([factor_coords[d] * self.place_values[d] for d in range(self.dim)])

    def index2factor_coords(self, par_index):
        # Factor graph coordinates from parameter index
        return [(par_index // self.place_values[d]) % self.factor_sizes[d] for d in range(self.dim)]

    def full_index2index(self, full_index):
        parameter = self.full_pg.parameter(full_index)
        factor_coords = []
        for d in range(self.dim):
            hex_code = parameter.logic()[d].hex()
            k = parameter.order()[d].index()
            if hex_code not in self.hex_codes[d]:
                return -1
            if k not in self.orders[d]:
                return -1
            i = self.hex_codes[d].index(hex_code)
            if (i, k) not in self.hex_order_pairs[d]:
                return -1
            pair_index = self.hex_order_pairs[d].index(((i, k)))
            factor_coords.append(pair_index)
        return self.factor_coords2index(factor_coords)

    def index2full_index(self, index):
        if index >= self.size():
            print('Parameter index out of bounds.')
            return -1
        # Get factor graph coordinates for index
        factor_coords = self.index2factor_coords(index)
        # Construct a DSGRN parameter to get index
        logic_pars = []
        order_pars = []
        for d in range(self.dim):
            # Get number of inputs and outputs
            n_inputs = len(self.full_pg.network().inputs(d))
            n_outputs = len(self.full_pg.network().outputs(d))
            i, k = self.hex_order_pairs[d][factor_coords[d]]
            hex_code = self.hex_codes[d][i]
            logic_pars.append(DSGRN.LogicParameter(n_inputs, n_outputs, hex_code))
            order_pars.append(DSGRN.OrderParameter(n_outputs, k))
        # Get DSGRN parameter
        parameter = DSGRN.Parameter(logic_pars, order_pars, self.network())
        full_index = self.full_pg.index(parameter)
        return full_index

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
        # Return whether single term self-edge is negative
        return not self.full_pg.network().edge_sign(d, d, instance)

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

    def __valid_hex_code(self, hex_code, d, perm=None):
        """Check if the hex_code is valid if node d has
        positive decay and a negative single term self-edge
        (the hex_code is automatically valid otherwise).
        If perm is provided it also checks if the carrying
        capacity threshold is below the pj corresponding
        to the carrying capacity delta, that is, check if
        the order parameter is valid. Otherwise it only
        checks if there is at least one thrshold below pj.
        If filter_level is 2 it also makes sure that the
        carrying capacity threshold is the largest threshold.
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
        if perm == None: # Just need threshold(s) before pj
            # Need all thresholds before pj for filter_level 2
            if self.filter_level == 2:
                # Return whether no thresholds after pj
                return not any(p < 0 for p in partial_order[pj_index:])
            else: # Need at least one thresholds before pj for filter_level 1
                # Return whether any threshold before pj
                return any(p < 0 for p in partial_order[:pj_index])
        # Check if the order parameter is valid for this hex code
        # Original (before permutation) negative single term
        # self-edge threshold index, that is, t0, t1, ect.
        tk = self.__single_term_self_edge_threshold(d)
        # Threshold index for this order parameter, that is, the
        # threshold index in this parameter that corresponds to
        # the threshold tk in the original threshold index.
        thres_index = perm.index(tk)
        # Negative value t representing this threshold in partial order
        t = thres_index - n_outputs
        # Make sure tk is the largest threshold for filter_level 2
        if self.filter_level == 2:
            if any(partial_order.index(t) < partial_order.index(th_pos) for th_pos in range(-n_outputs, 0)):
                return False
        # Return whether the threshold t is before pj in partial order
        return any(partial_order[i] == t for i in range(pj_index))

    def valid_hex_code(self, hex_code, d):
        return self.__valid_hex_code(hex_code, d)

    def valid_order(self, perm, d):
        # Need tk to be largest threshold for filter_level 2
        if self.filter_level == 2:
            # Original (before permutation) negative single term
            # self-edge threshold index, that is, t0, t1, ect.
            tk = self.__single_term_self_edge_threshold(d)
            # Need tk to be last element of perm, that is,
            # the largest threshold for this order parameter
            return tk == perm[-1]
        else: # Filter level 1
            # All orders are valid for filter_level 1
            return True

    def valid_hex_order_pair(self, hex_code, perm, d):
        return self.__valid_hex_code(hex_code, d, perm=perm)

    def valid_parameter(self, parameter):
        return self.index(parameter) != -1

    def valid_full_index(self, full_index):
        return self.full_index2index(full_index) != -1

    def __init__(self, network, filter_level=1):
        # Initialize full parameter graph
        self.full_pg = DSGRN.ParameterGraph(network)
        self.dim = self.full_pg.dimension()
        self.filter_level = filter_level
        # We construct a list of valid hex codes and a list of
        # valid orders, orders[d], for each node. For filter_level 1,
        # depending on the hex code some orders are not allowed,
        # however every order in orders[d] is allowed for at least
        # one hex code. For filter_level 2 every order is allowed
        # for every hex code. So we also compute a list of allowed
        # (hex code, order) pairs.
        self.hex_codes = [] # List of valid hex codes for each node
        for d in range(self.dim):
            valid_hex = [h for h in self.full_pg.factorgraph(d) if self.valid_hex_code(h, d)]
            self.hex_codes.append(valid_hex)
        # self.orders = [] # List of valid orders for each node
        # for d in range(self.dim):
        #     m = len(self.full_pg.network().outputs(d))
        #     valid_ord = [k for k in range(factorial(m)) if self.valid_order(k, d)] 
        #     self.orders.append(valid_ord)
        self.orders = [] # List of valid orders for each node
        self.hex_order_pairs = [] # List of valid (hex, order) pairs
        for d in range(self.dim):
            m = len(self.full_pg.network().outputs(d))
            valid_ord = []
            valid_pairs = []
            for k in range(factorial(m)):
                # Make order parameter and get permutation
                perm = DSGRN.OrderParameter(m, k).permutation()
                if self.valid_order(perm, d):
                    valid_ord.append(k)
                for i, h in enumerate(self.hex_codes[d]):
                    if self.valid_hex_order_pair(h, perm, d):
                        valid_pairs.append((i, k))
            self.orders.append(valid_ord)
            self.hex_order_pairs.append(valid_pairs)
        # Size of each factor graph
        self.factor_sizes = [len(self.hex_order_pairs[d]) for d in range(self.dim)]
        # Place values used to convert between index and factor graph coordinates
        self.place_values = [reduce(mul, self.factor_sizes[:d], 1) for d in range(self.dim + 1)]
