/// Network.hpp
/// Shaun Harker
/// 2015-05-22
///
/// Marcio Gameiro
/// 2020-08-03

#pragma once

#ifndef INLINE_IF_HEADER_ONLY
#define INLINE_IF_HEADER_ONLY
#endif

#include "Network.h"

INLINE_IF_HEADER_ONLY Network::
Network ( void ) {
  data_ . reset ( new Network_ );
  data_ -> model_ = "original";
}

INLINE_IF_HEADER_ONLY Network::
Network ( std::string const& s ) {
  data_ . reset ( new Network_ );
  data_ -> model_ = "original";
  assign(s);
}

INLINE_IF_HEADER_ONLY Network::
Network ( std::string const& s, std::string const& model ) {
  // Remove leading and trailing whitespaces of model
  std::string m = std::regex_replace(model, std::regex("^ +| +$|( ) +"), "$1");
  // Transform all letters of m to lower case
  std::transform(m.begin(), m.end(), m.begin(), ::tolower);
  // Check if model m is valid
  if ( (m != "original") and (m != "ecology") ) {
    throw std::runtime_error ( "Invalid model specification!" );
  }
  data_ . reset ( new Network_ );
  data_ -> model_ = m;
  assign(s);
}

INLINE_IF_HEADER_ONLY void Network::
assign ( std::string const& s ) {
  auto colon = s.find(':');
  if ( colon != std::string::npos ) {
    data_ -> specification_ = s;
    _parse ( _lines () );
  } else {
    load(s);
  }
}

INLINE_IF_HEADER_ONLY void Network::
load ( std::string const& filename ) {
data_ . reset ( new Network_ );
  std::ifstream infile ( filename );
  if ( not infile . good () ) { 
    throw std::runtime_error ( "Problem loading network specification file " + filename );
  }
  std::string line;
  while ( std::getline ( infile, line ) ) {
    data_ -> specification_ += line + '\n';
  }
  infile . close ();
  _parse ( _lines () );
}

INLINE_IF_HEADER_ONLY uint64_t Network::
size ( void ) const {
  return data_ -> name_by_index_ . size ();
}

INLINE_IF_HEADER_ONLY uint64_t Network::
index ( std::string const& name ) const {
  return data_ -> index_by_name_ . find ( name ) -> second;
}

INLINE_IF_HEADER_ONLY std::string const& Network::
name ( uint64_t index ) const {
  return data_ -> name_by_index_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
inputs ( uint64_t index ) const {
  return data_ -> inputs_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
outputs ( uint64_t index ) const {
  return data_ -> outputs_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
input_instances ( uint64_t index ) const {
  return data_ -> input_instances_[index];
}

INLINE_IF_HEADER_ONLY std::vector<std::vector<uint64_t>> const& Network::
logic ( uint64_t index ) const {
  return data_ -> logic_by_index_ [ index ];
}

INLINE_IF_HEADER_ONLY bool Network::
essential ( uint64_t index ) const {
  return data_ -> essential_ [ index ];
}

INLINE_IF_HEADER_ONLY bool Network::
interaction ( uint64_t source, uint64_t target ) const {
  return data_ -> edge_type_ . find ( std::make_pair ( source, target ) ) -> second;
}

INLINE_IF_HEADER_ONLY bool Network::
edge_sign ( uint64_t source, uint64_t target ) const {
  return data_ -> edge_sign_ . find ( std::make_pair ( source, target ) ) -> second;
}

INLINE_IF_HEADER_ONLY std::vector<bool> const& Network::
logic_term_sign ( uint64_t index ) const {
  return data_ -> logic_term_sign_ [ index ];
}

INLINE_IF_HEADER_ONLY uint64_t Network::
order ( uint64_t source, uint64_t target, uint64_t instance ) const {
  return data_ -> order_ . find ( std::make_tuple (source, target, instance) ) -> second;
}

INLINE_IF_HEADER_ONLY  std::vector<uint64_t> Network::
domains ( void ) const {
  std::vector<uint64_t> result;
  for ( auto const& output : data_ -> outputs_ ) {
    result . push_back ( output . size () + 1 );
  }
  return result;
}

INLINE_IF_HEADER_ONLY std::string Network::
specification ( void ) const {
  return data_ -> specification_;
}

INLINE_IF_HEADER_ONLY std::string Network::
model ( void ) const {
  return data_ -> model_;
}

INLINE_IF_HEADER_ONLY std::string Network::
graphviz ( std::vector<std::string> const& theme ) const {
  std::stringstream result;
  // std::cout << "graphviz. Looping through nodes.\n";
  result << "digraph {\n";
  result << "bgcolor = " << theme[0] << ";";
  for ( uint64_t i = 0; i < size (); ++ i ) {
    result << "\"" << name(i) << "\"" << " [style=filled fillcolor=" << theme[1] << "];\n";
  }
  std::string normalhead ("normal");
  std::string blunthead ("tee");
  // std::cout << "graphviz. Looping through edges.\n";
  for ( uint64_t target = 0; target < size (); ++ target ) {
    std::vector<std::vector<uint64_t>> logic_struct = logic ( target );
    std::reverse ( logic_struct . begin (), logic_struct . end () ); // prefer black
    std::vector<bool> terms_sign = logic_term_sign ( target );
    std::reverse ( terms_sign . begin (), terms_sign . end () );
    uint64_t partnum = 0;
    uint64_t loop_index = 0;
    for ( auto const& part : logic_struct ) {
      for ( uint64_t source : part ) {
        // std::cout << "Checking type of edge from " << source << " to " << target << "\n";
        std::string head = interaction(source,target) ? normalhead : blunthead;
        // For the ecology model the arrow head is based on the edge sign
        if ( model () == "ecology" ) {
          head = terms_sign [loop_index] ? normalhead : blunthead;
        }
        result << "\"" << name(source) << "\" -> \"" << name(target)
               << "\" [color=" << theme[partnum+2] << " arrowhead=\"" << head << "\"];\n";
      }
      ++ partnum;
      if ( partnum + 2 == theme . size () ) partnum = 0;
      ++ loop_index;
    }
  }
  result << "}\n";
  return result . str ();
}

namespace DSGRN_parse_tools {
  // http://stackoverflow.com/questions/236129/split-a-string-in-c
  INLINE_IF_HEADER_ONLY std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }
    return elems;
  }
  INLINE_IF_HEADER_ONLY std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }
  INLINE_IF_HEADER_ONLY std::string& removeSpace ( std::string &s ) {
    s.erase(std::remove_if(s.begin(), s.end(), (uint64_t(*)(uint64_t))isspace), s.end());
    return s;
  }
}

/// _lines
///   Open the network file and read it line by line
INLINE_IF_HEADER_ONLY std::vector<std::string> Network::
_lines ( void ) {
  // Remove quote marks if they exist, and convert "\n" substrings to newlines
  std::string & str = data_ -> specification_;
  const std::regex quote_regex("\"", std::regex::basic);
  const std::regex newline_regex("\n", std::regex::basic);
  data_ -> specification_ = std::regex_replace(data_ -> specification_, newline_regex, "\n" );
  data_ -> specification_ = std::regex_replace(data_ -> specification_, quote_regex, "" );

  // Parse the lines
  std::vector<std::string> result;
  std::stringstream spec ( data_ -> specification_ );
  std::string line;
  while ( std::getline ( spec, line ) ) {
    result . push_back ( line );
  }
  return result;
}

/// _parse
///   Iterate through lines and produce data structures
INLINE_IF_HEADER_ONLY void Network::
_parse ( std::vector<std::string> const& lines ) {
  using namespace DSGRN_parse_tools;
  std::vector<std::string> logic_strings;
  std::map<std::string, bool> essential_nodes;
  // std::vector<std::string> constraint_strings;
  // Learn the node names
  for ( auto const& line : lines ) {
    auto splitline = split ( line, ':' );
    if ( splitline . empty () ) continue;
    removeSpace(splitline[0]);
    // If begins with . or @, skip
    if ( (splitline[0][0] == '.') || (splitline[0][0] == '@' ) ) continue; 
    data_ -> name_by_index_ . push_back ( splitline[0] );
    // If no logic specified, zero inputs.
    if ( splitline . size () < 2 ) {
      logic_strings . push_back (" ");
    } else {
      logic_strings . push_back ( splitline[1] );
    }
    //std::cout << line << " has " << splitline.size() << " parts.\n";
    if ( splitline . size () >= 3 ) {
      // TODO: make it check for keyword "essential"
      essential_nodes [ splitline[0] ] = true;
      //std::cout << "Marking " << splitline[0] << " as essential \n";
    } else {
      essential_nodes [ splitline[0] ] = false;
    }
  }
  // Index the node names
  uint64_t loop_index = 0;
  data_ -> essential_ . resize ( essential_nodes . size () );
  for ( auto const& name : data_ -> name_by_index_ ) { 
    data_ -> index_by_name_ [ name ] = loop_index; 
    data_ -> essential_ [ loop_index ] = essential_nodes [ name ];
    ++ loop_index;
  }

  // Parse logic strings to learn the logic
  if ( data_ -> model_ == "original" ) {
    _parse_logic ( logic_strings );
  } else { // model == "ecology"
    _parse_logic_ecology ( logic_strings );
  }

  // Compute inputs and outputs
  data_ -> inputs_ . resize ( size () );
  data_ -> outputs_ . resize ( size () );
  data_ -> input_instances_ . resize ( size () );
  for ( uint64_t target = 0; target < size (); ++ target ) {
    // Keep track of repeated inputs to target
    std::unordered_map<uint64_t, uint64_t> input_counts;
    for ( auto const& factor : logic ( target ) ) {
      for ( uint64_t source : factor ) {
        uint64_t instance = 0; // Keep track of input instances
        if ( input_counts . find (source) != input_counts . end() ) {
          instance = input_counts . find (source) -> second;
        }
        input_counts [source] = instance + 1;
        data_ -> inputs_[target] . push_back ( source );
        data_ -> outputs_[source] . push_back ( target );
        data_ -> input_instances_[target] . push_back ( instance );
        // Output order of this instance of edge (source, target)
        data_ -> order_[std::make_tuple(source, target, instance)] = data_ -> outputs_[source].size() - 1;
      }
    }
  }
  //std::cout << "_parse complete.\n";
}

/// _parse_logic
///   Iterate through logic strings to learn the logic structures
INLINE_IF_HEADER_ONLY void Network::
_parse_logic ( std::vector<std::string> const& logic_strings ) {
  // Learn the logics
  // Trick: ignore everything but node names and +'s.
  // Example: a + ~ b c d + e  corresponds to (a+~b)(c)(d+e)
  uint64_t target = 0;
  for ( auto const& logic_string : logic_strings ) {
    // std::cout << "Processing " << logic_string << "\n";
    std::vector<std::vector<uint64_t>> logic_struct;
    std::vector<uint64_t> factor;
    std::string token;
    bool parity = true;
    bool appending = true;

    auto flush_factor = [&] () {
      if ( factor . empty () ) return;
      // Put factor into canonical ordering
      std::sort ( factor.begin(), factor.end() );
      logic_struct . push_back ( factor );
      // std::cout << "    Flushing factor ";
      // for ( uint64_t i : factor ) std::cout << name ( i ) << " ";
      // std::cout << "\n";
      factor . clear ();
    };

    auto flush_token = [&] () {
      if ( token . empty () ) return;
      if ( not appending ) flush_factor ();
      // std::cout << "  Flushing token " << token << "\n";
      if ( data_ -> index_by_name_ . count ( token ) == 0 ) {
        throw std::runtime_error ( "Problem parsing network specification file: " 
                                   " Invalid input variable " + token );
      }
      uint64_t source = data_ -> index_by_name_ [ token ];
      factor . push_back ( source );
      data_ ->  edge_type_[std::make_pair( source, target )] = parity;
      // For regular DSGRN all edges are positive
      data_ -> edge_sign_[std::make_pair( source, target )] = true;
      // std::cout << "Creating edge from " << source << " to " << target << "\n";
      token . clear ();
      appending = false;
      parity = true;
    };

    for ( char c : logic_string ) {
      // std::cout << "Reading character " << c << "\n";
      if ( ( c == '\t' ) || (c == ' ') || (c == '(') || (c == ')') || (c == '+') || (c == '~') ) {
        flush_token ();
      } else {
        token . push_back ( c );
      }
      if ( c == '+' ) {
        appending = true;
        // std::cout << "  Detected +\n";
      }
      if ( c == '~' ) parity = false;
    }
    flush_token ();
    flush_factor ();
    // std::cout << "The logic_struct formed.\n";
    // Ensure logic_struct is acceptable (no repeats!)
    std::unordered_set<uint64_t> inputs;
    for ( auto const& factor : logic_struct ) {
      // std::cout << "# ";
      for ( auto i : factor ) {
        // std::cout << i << " ";
        if ( inputs . count ( i ) ) {
          throw std::runtime_error ( "Problem parsing network specification file: Repeated inputs in logic" );
        }
        inputs . insert ( i );
      }
    }
    // std::cout << "\n";
    // std::cout << "The logic_struct is acceptable.\n";
    // Compare partitions by (size, max), where size is length and max is maximum index
    auto compare_partition = [](std::vector<uint64_t> const& lhs, std::vector<uint64_t> const& rhs) {
      if ( lhs . size () < rhs . size () ) return true;
      if ( lhs . size () > rhs . size () ) return false;
      uint64_t max_lhs = * std::max_element ( lhs.begin(), lhs.end() );
      uint64_t max_rhs = * std::max_element ( rhs.begin(), rhs.end() );
      if ( max_lhs < max_rhs ) return true;
      if ( max_lhs > max_rhs ) return false;  /* unreachable -> */ return false;
    };

    // Put the logic struct into a canonical ordering.
    std::sort ( logic_struct.begin(), logic_struct.end(), compare_partition );
    data_ -> logic_by_index_ . push_back ( logic_struct );
    // For regular DSGRN all terms are positive
    std::vector<bool> logic_terms_sign ( logic_struct . size (), true);
    data_ -> logic_term_sign_ . push_back ( logic_terms_sign );
    // std::cout << "The logic_struct has been incorporated into the network.\n";
    ++ target;
  }
}

/// _parse_logic_ecology
///   Iterate through logic strings to learn the logic structures
INLINE_IF_HEADER_ONLY void Network::
_parse_logic_ecology ( std::vector<std::string> const& logic_strings ) {
  // Learn the logics
  // Trick: ignore everything but node names and +'s and -'s.
  // Example: a + ~ b c d - e  corresponds to (a+~b)(c)(d-e)
  // For the ecology model the logic consists of sums or
  // subtraction of products. Example: x + y z w - u v
  uint64_t target = 0;
  for ( auto const& logic_string : logic_strings ) {
    // std::cout << "Processing " << logic_string << "\n";
    std::vector<std::vector<uint64_t>> logic_struct;
    std::vector<uint64_t> term;
    std::string token;
    bool self_edge_term_sign = false; // Sign of carrying capacity self edge
    bool sign_edge = true; // true for positive edges, false for negative
    bool parity = true; // Always true (activating) for now

    auto flush_term = [&] () {
      if ( term . empty () ) return;
      // If a single term self edge set self_edge_term_sign
      if ( (term . size () == 1) and (term[0] == target) ) {
        self_edge_term_sign = sign_edge;
      }
      // Put term into canonical ordering
      std::sort ( term.begin(), term.end() );
      logic_struct . push_back ( term );
      // std::cout << "    Flushing term ";
      // for ( uint64_t i : term ) std::cout << name ( i ) << " ";
      // std::cout << "\n";
      term . clear ();
    };

    auto flush_token = [&] () {
      if ( token . empty () ) return;
      // std::cout << "  Flushing token " << token << "\n";
      if ( data_ -> index_by_name_ . count ( token ) == 0 ) {
        throw std::runtime_error ( "Problem parsing network specification file: " 
                                   " Invalid input variable " + token );
      }
      uint64_t source = data_ -> index_by_name_ [ token ];
      term . push_back ( source );
      data_ -> edge_type_[std::make_pair( source, target )] = parity;
      data_ -> edge_sign_[std::make_pair( source, target )] = sign_edge;
      // std::cout << "Creating edge from " << source << " to " << target << "\n";
      token . clear ();
      // parity = true; // Always true for now
    };

    for ( char c : logic_string ) {
      // std::cout << "Reading character " << c << "\n";
      if ( (c == '\t') || (c == ' ') || (c == '(') || (c == ')') || (c == '+') || (c == '-') || (c == '~') ) {
        flush_token ();
      } else {
        token . push_back ( c );
      }
      if ( (c == '+') || (c == '-') ) {
        flush_term ();
      }
      if ( c == '+' ) {
        sign_edge = true;
      }
      if ( c == '-' ) {
        sign_edge = false;
      }
    }
    flush_token ();
    flush_term ();
    // std::cout << "The logic_struct formed.\n";
    // Ensure logic_struct is acceptable (no repeats except for self edge!)
    std::unordered_set<uint64_t> inputs;
    bool repeated_self_edge = false;
    for ( auto const& term : logic_struct ) {
      // std::cout << "# ";
      for ( auto i : term ) {
        // std::cout << i << " ";
        // Do not allow repeats except for self edges
        if ( inputs . count ( i ) ) { // Repeated input edge
          if ( i != target ) { // If is it not a self edge
            throw std::runtime_error ( "Problem parsing network specification file: Repeated inputs in logic!" );
          } else if ( self_edge_term_sign ) { // If it is positive a self edge
            throw std::runtime_error ( "Repeated single term self edge cannot be positive!");
          } else {
            repeated_self_edge = true;
          }
        }
        inputs . insert ( i );
      }
    }
    // If there is a repeated input it is a double self edge
    // Make sure one of them is part of a larger term
    bool single_term_self_edge = false;
    for ( auto const& term : logic_struct ) {
      if ( (term . size () == 1) and (term[0] == target) ) {
        if ( single_term_self_edge ) {
          throw std::runtime_error ( "Repeated single term self edges not allowed!" );
        } else {
          single_term_self_edge = true;
        }
      }
    }
    // std::cout << "\n";
    // std::cout << "The logic_struct is acceptable.\n";
    // Compare partitions by (size, max), where size is length and max is maximum index
    auto compare_partition = [](std::vector<uint64_t> const& lhs, std::vector<uint64_t> const& rhs) {
      if ( lhs . size () < rhs . size () ) return true;
      if ( lhs . size () > rhs . size () ) return false;
      uint64_t max_lhs = * std::max_element ( lhs.begin(), lhs.end() );
      uint64_t max_rhs = * std::max_element ( rhs.begin(), rhs.end() );
      if ( max_lhs < max_rhs ) return true;
      if ( max_lhs > max_rhs ) return false;  /* unreachable -> */ return false;
    };
    // Put the logic struct into a canonical ordering.
    std::sort ( logic_struct.begin(), logic_struct.end(), compare_partition );

    // edge_sign may be overwritten because of double self edge
    // so we define the term sign based on the non-self edges in
    // the term and define the sign a single term self edge to
    // be negative if it is a double edge.
    auto sign_term = [&] (std::vector<uint64_t> const& term) {
      if ( term . size () == 1 ) { // If it is a single term
        auto source = term . front();
        if ( (source == target) and repeated_self_edge ) {
          return false; // Negative if repeated self edge
        } else {
          return edge_sign ( source, target ); // Edge sign defined before
        }
      } else { // Not a single term
        for ( auto source : term ) { // Find a non-self edge
          if ( source != target ) {
            return edge_sign ( source, target ); // Edge sign defined before
          }
        }
        // If we got to here there is a problem
        throw std::runtime_error ( "Term with repeated self edge in a logic term!" );
      }
    };

    // Put negative terms before positive terms
    std::vector<std::vector<uint64_t>> logic_sorted;
    std::vector<std::vector<uint64_t>> logic_pos;
    for ( auto const& term : logic_struct ) {
      if ( sign_term ( term ) ) {
        logic_pos . push_back ( term );
      } else {
        logic_sorted . push_back ( term );
      }
    }
    // Insert logic for positive edges at the end of the logic struct
    logic_sorted . insert ( logic_sorted . end (), logic_pos . begin (), logic_pos . end ());
    // Assign the logic term signs
    std::vector<bool> logic_terms_sign;
    for ( auto const& term : logic_sorted ) {
      logic_terms_sign . push_back ( sign_term ( term ) );
    }
    data_ -> logic_by_index_ . push_back ( logic_sorted );
    data_ -> logic_term_sign_ . push_back ( logic_terms_sign );
    // std::cout << "The logic_struct has been incorporated into the network.\n";
    ++ target;
  }
}

INLINE_IF_HEADER_ONLY std::ostream& operator << ( std::ostream& stream, Network const& network ) {
  stream << "[";
  bool first1 = true;
  for ( uint64_t v = 0; v < network.size (); ++ v ) {
    if ( first1 ) first1 = false; else stream << ",";
    stream << "[\"" << network.name(v) << "\","; // node
    std::vector<std::vector<uint64_t>> logic_struct = network.logic ( v );
    stream << "["; // logic_struct
    bool first2 = true;
    for ( auto const& part : logic_struct ) {
      if ( first2 ) first2 = false; else stream << ",";
      stream << "["; // factor
      bool first3 = true;
      for ( uint64_t source : part ) {
        if ( first3 ) first3 = false; else stream << ",";
        std::string head = network.interaction(source,v) ? "" : "~";
        stream << "\"" << head << network.name(source) << "\"";
      }
      stream << "]"; // factor
    }
    stream << "],"; // logic_struct
    stream << "["; // outputs
    bool first4 = true;
    for ( uint64_t target : network.outputs ( v ) ) {
      if ( first4 ) first4 = false; else stream << ",";
      stream << "\"" << network.name(target) << "\"";
    }
    stream << "]"; // outputs 
    stream << "]"; // node
  }  
  stream << "]"; // network
  return stream;
}
