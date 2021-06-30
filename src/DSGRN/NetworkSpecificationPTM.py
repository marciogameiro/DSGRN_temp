# NetworkSpecificationPTM.py
# Marcio Gameiro
# MIT LICENSE
# 2021-06-29

import DSGRN
import graphviz
import re

def SpecPTM2Spec(ptm_net_spec):
    """Return regular DSGRN network specification
    from PTM network specification"""
    # Assume it is a file name if no :
    if ':' not in ptm_net_spec:
        with open(ptm_net_spec, 'r') as f:
            ptm_net_spec = f.read()
    # Initialize net_spec as ptm_net_spec
    net_spec = ptm_net_spec
    # Find PTM pairs (substrings enclosed by [] with a comma)
    pattern = '\[.*?,.*?\]' # RE pattern
    ptm_terms = re.findall(pattern, net_spec)
    for term in ptm_terms:
        # Remove leading [ and trailing ]
        ptm_term = term[1:-1].strip()
        node1, node2 = [node.strip() for node in ptm_term.split(',')]
        # Check if node1 is active (enclosed by [])
        if ('[' in node1) and (']' in node1):
            # Remove [ and ] from node1
            node1 = node1.replace('[', '').replace(']', '')
            # Add or remove ~ from node2
            node2 = node2.replace('~', '') if '~' in node2 else '~' + node2
            # Make modified PTM term
            ptm_term = '[' + node1 + ', ' + node2 + ']'
            # Replace the old term by the new PTM term
            net_spec = net_spec.replace(term, ptm_term)
    return net_spec

def NetworkPTM(ptm_net_spec):
    """Define a DSGRN network from PTM network specification"""
    return DSGRN.Network(SpecPTM2Spec(ptm_net_spec))

def DrawNetworkPTM(ptm_net_spec):
    """Draw network in PTM format from specification
    """
    # Assume it is a file name if no :
    if ':' not in ptm_net_spec:
        with open(ptm_net_spec, 'r') as f:
            ptm_net_spec = f.read()

    def ptm_regular_nodes(logic):
        """Get input nodes from logic as lists PTM pairs of
        nodes and regular nodes. Assume that the decay term
        delimiters < and > have been removed."""
        # Find the PTM pairs
        # Find substrings enclosed by [] with a comma
        pattern = '\[.*?,.*?\]' # RE pattern
        ptm_terms = re.findall(pattern, logic)
        # Remove substring from logic
        logic = re.sub(pattern, '', logic)
        ptm_pairs = [] # List of PTM pairs names
        for term in ptm_terms:
            # Replace first '[', last ']', and ',' by spaces
            ptm_term = re.sub('^\[|,|\]$', ' ', term)
            # Get the ptm pair nodes names
            ptm_pair = [s.strip() for s in ptm_term.split(' ') if s.strip()]
            ptm_pairs.append(ptm_pair)
        # Now get the regular terms
        # Replace '(', ')', and '+' by white spaces
        regular_terms = re.sub('[()+]', ' ', logic)
        # Get the regular terms nodes names
        regular_nodes = [s.strip() for s in regular_terms.split(' ') if s.strip()]
        # List of input nodes: first decay, then PTM, the regular
        return ptm_pairs, regular_nodes

    def input_nodes(logic):
        """Get input nodes from logic as lists of ptm decay nodes,
        decay nodes, PTM pairs of nodes, and regular nodes"""
        # First get the decay term
        # Find substring enclosed by <>
        pattern = '<.*?>' # RE pattern
        decay_term = re.findall(pattern, logic)
        # Remove substring from logic
        logic = re.sub(pattern, '', logic)
        # Replace '<' and '>' by white spaces
        decay_term = re.sub('[<>]', ' ', decay_term[0]) if decay_term else ''
        # Get the ptm decay terms and the decay term node names
        ptm_decay_pairs, decay_nodes = ptm_regular_nodes(decay_term)
        # Get the the PTM pairs and regular terms node names
        ptm_pairs, regular_nodes = ptm_regular_nodes(logic)
        # List of input nodes: first ptm decay, then decay, PTM, and regular
        in_nodes = [ptm_decay_pairs, decay_nodes, ptm_pairs, regular_nodes]
        return in_nodes

    # Get list of network specifications
    specs = [spec.strip() for spec in ptm_net_spec.strip().split('\n') if spec.strip()]
    # Get node names and list of input logics
    node_names = [spec.split(':')[0].strip() for spec in specs]
    logics = [spec.split(':')[1].strip() for spec in specs]
    # Create graphviz string
    gv_str = 'digraph {\n'
    # First add the graph nodes
    for n, name in enumerate(node_names):
        gv_str += str(n) + ' [label=' + name + '];\n'
    # Now add the edges to the graph
    gv_edges = ''
    num_ptm_pairs = 0
    for n, logic in enumerate(logics):
        # Get the ptm decay, decay, PTM, and regular input nodes
        ptm_decay_pairs, decay_nodes, ptm_pairs, regular_nodes = input_nodes(logic)
        # Add decay edges
        for node in decay_nodes:
            name, head = (node[1:], 'tee') if node[0] == '~' else (node, 'vee')
            u = node_names.index(name) # Get node index
            gv_edges += str(u) + ' -> ' + str(n) + ' [style=dashed, arrowhead=' + head + '];\n'
        # Add regular edges
        for node in regular_nodes:
            name, head = (node[1:], 'tee') if node[0] == '~' else (node, 'vee')
            u = node_names.index(name) # Get node index
            gv_edges += str(u) + ' -> ' + str(n) + ' [style=solid, arrowhead=' + head + '];\n'
        # Add (pairs of) PTM decay and regular PTM edges
        ptm_decay_regular = ptm_decay_pairs + ptm_pairs
        for node1, node2 in ptm_decay_regular:
            # Check if node1 is active (default is inactive)
            # A node is active if enclosed by []
            active = ('[' in node1) and (']' in node1)
            if active: # Remove '[' and ']' from node1
                node1 = re.sub('[\[\]]', '', node1)
            name1, head1 = (node1[1:], 'tee') if node1[0] == '~' else (node1, 'vee')
            name2, head2 = (node2[1:], 'tee') if node2[0] == '~' else (node2, 'vee')
            u1 = node_names.index(name1) # Get node index
            u2 = node_names.index(name2) # Get node index
            # Add extra node for PTM edges
            u3 = len(node_names) + num_ptm_pairs
            # Extra node fill style
            style = '""' if active else 'filled'
            gv_str += str(u3) + ' [shape=circle, style=' + style + ', label="", width=0.15];\n'
            # Next add edges u1 -> u3, u2 -> u3, and u3 -> n
            # Add invisible edge to adjust node ranking
            gv_edges += str(n) + ' -> ' + str(u3) + ' [style=invisible, dir=none];\n'
            # Do not use self edges in ranking the nodes
            constraint = 'false' if n in [u1, u2] else 'true'
            # Make decay edges dashed
            style = 'dashed' if ([node1, node2] in ptm_decay_pairs) else 'solid'
            gv_edges += str(u1) + ' -> ' + str(u3) + ' [style=' + style + ', dir=none, headport=w];\n'
            gv_edges += str(u3) + ' -> ' + str(n) + ' [style=' + style + ', arrowhead=' + head1 + ', constraint=' + constraint + ', tailport=e];\n'
            gv_edges += str(u2) + ' -> ' + str(u3) + ' [style=' + style + ', arrowhead=' + head2 + '];\n'
            num_ptm_pairs += 1 # Increment PTM pairs counter
    gv_str += gv_edges + '}\n'
    return graphviz.Source(gv_str) # Return graphviz render
