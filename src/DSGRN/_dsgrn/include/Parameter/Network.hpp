/// Network.hpp
/// Shaun Harker
/// 2015-05-22
///
/// Marcio Gameiro
/// 2021-02-05

#pragma once

#ifndef INLINE_IF_HEADER_ONLY
#define INLINE_IF_HEADER_ONLY
#endif

#include "Network.h"

INLINE_IF_HEADER_ONLY Network::
Network ( void ) { 
  data_ . reset ( new Network_ );
}

INLINE_IF_HEADER_ONLY Network::
Network ( std::string const& s ) {
  assign(s);
}

INLINE_IF_HEADER_ONLY void Network::
assign ( std::string const& s ) {
  auto colon = s.find(':');
  if ( colon != std::string::npos ) {
    data_ . reset ( new Network_ );
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
  return data_ ->  name_by_index_ . size ();
}

INLINE_IF_HEADER_ONLY uint64_t Network::
index ( std::string const& name ) const {
  return data_ ->  index_by_name_ . find ( name ) -> second;
}

INLINE_IF_HEADER_ONLY std::string const& Network::
name ( uint64_t index ) const {
  return data_ ->  name_by_index_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
inputs ( uint64_t index ) const {
  return data_ ->  inputs_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
outputs ( uint64_t index ) const {
  return data_ ->  outputs_[index];
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> const& Network:: 
input_instances ( uint64_t index ) const {
  return data_ -> input_instances_[index];
}

INLINE_IF_HEADER_ONLY std::vector<std::vector<uint64_t>> Network::
logic ( uint64_t index ) const {
  std::vector<std::vector<uint64_t>> logic_struct;
  for ( auto const& term : data_ -> logic_by_index_ [ index ] ) {
    std::vector<uint64_t> logic_term;
    for ( EdgeTuple edge_tuple : term ) {
      logic_term . push_back ( std::get<0> ( edge_tuple ) );
    }
    logic_struct . push_back ( logic_term );
  }
  return logic_struct;
}

INLINE_IF_HEADER_ONLY std::vector<std::vector<EdgeTuple>> Network::
logic_by_index ( uint64_t index ) const {
  return data_ -> logic_by_index_ [ index ];
}

INLINE_IF_HEADER_ONLY bool Network::
essential ( uint64_t index ) const {
  return data_ -> essential_ [ index ];
}

INLINE_IF_HEADER_ONLY bool Network::
interaction ( uint64_t source, uint64_t target, uint64_t instance ) const {
  return data_ -> edge_type_ . find ( std::make_tuple ( source, target, instance ) ) -> second;
}

INLINE_IF_HEADER_ONLY bool Network::
edge_sign ( uint64_t source, uint64_t target, uint64_t instance ) const {
  return data_ -> edge_sign_ . find ( std::make_tuple ( source, target, instance ) ) -> second;
}

INLINE_IF_HEADER_ONLY uint64_t Network::
order ( uint64_t source, uint64_t target, uint64_t instance ) const {
  return data_ -> order_ . find ( std::make_tuple (source, target, instance) ) -> second;
}

INLINE_IF_HEADER_ONLY  std::vector<uint64_t> Network::
domains ( void ) const {
  std::vector<uint64_t> result;
  for ( auto const& output : data_ ->  outputs_ ) {
    result . push_back ( output . size () + 1);
  }
  return result;
}

INLINE_IF_HEADER_ONLY std::string Network::
specification ( void ) const {
  return data_ -> specification_;
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
    std::vector<std::vector<EdgeTuple>> logic_struct = logic_by_index ( target );
    std::reverse ( logic_struct . begin (), logic_struct . end () ); // prefer black
    uint64_t partnum = 0;
    for ( auto const& term : logic_struct ) {
      for ( EdgeTuple edge_tuple : term ) {
        uint64_t source = std::get<0> ( edge_tuple );
        bool edge_interaction = std::get<1> ( edge_tuple );
        // bool edge_sign = std::get<2> ( edge_tuple );
        // std::cout << "Checking type of edge from " << source << " to " << target << "\n";
        std::string head = edge_interaction ? normalhead : blunthead;
        result << "\"" << name(source) << "\" -> \"" << name(target)
               << "\" [color=" << theme[partnum+2] << " arrowhead=\"" << head << "\"];\n";
      }
      ++ partnum;
      if ( partnum + 2 == theme . size () ) partnum = 0;
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

/// parse
///   Iterate through lines and produce data structures
INLINE_IF_HEADER_ONLY void Network::
_parse ( std::vector<std::string> const& lines ) {
  using namespace DSGRN_parse_tools;
  std::vector<std::string> logic_strings;
  std::map<std::string, bool> essential_nodes;
  //std::vector<std::string> constraint_strings;
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
      logic_strings . push_back ( " " );
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
  // Learn the logics
  // Trick: ignore everything but node names and +'s. 
  // Example: a + ~ b c d + e  corresponds to (a+~b)(c)(d+e)
  uint64_t target = 0;
  for ( auto const& logic_string : logic_strings ) {
    //std::cout << "Processing " << logic_string << "\n";
    std::vector<std::vector<EdgeTuple>> logic_struct;
    std::vector<EdgeTuple> factor;
    std::string token;
    bool edge_sign = true; // For regular DSGRN all edges are positive
    bool parity = true;
    bool appending = true;

    auto flush_factor = [&] () {
      if ( factor . empty () ) return;
      // Put factor into canonical ordering
      std::sort ( factor.begin(), factor.end() );
      logic_struct . push_back ( factor );
      //std::cout << "    Flushing factor ";
      //for ( uint64_t i : factor ) std::cout << name ( i ) << " ";
      //std::cout << "\n";
      factor . clear ();      
    };

    auto flush_token = [&] () {
      if ( token . empty () ) return;
      if ( not appending ) flush_factor ();
      //std::cout << "  Flushing token " << token << "\n";
      if ( data_ -> index_by_name_ . count ( token ) == 0 ) {
        throw std::runtime_error ( "Problem parsing network specification file: " 
                                   " Invalid input variable " + token );
      }
      uint64_t source = data_ -> index_by_name_ [ token ];
      factor . push_back ( std::make_tuple(source, parity, edge_sign) );
      //std::cout << "Creating edge from " << source << " to " << target << "\n";
      token . clear ();
      appending = false;
      parity = true;
    };

    for ( char c : logic_string ) {
      //std::cout << "Reading character " << c << "\n";
      if ( ( c == '\t' ) || (c == ' ') || (c == '(') || (c == ')') || (c == '+') || (c == '~') ) {
        flush_token ();
      } else {
        token . push_back ( c );
      }
      if ( c == '+' ) { 
        appending = true;
        //std::cout << "  Detected +\n";
      }
      if ( c == '~' ) parity = false;
    }
    flush_token ();
    flush_factor ();
    //std::cout << "The logic_struct formed.\n";
    std::unordered_set<uint64_t> inputs;
    for ( auto const& factor : logic_struct ) {
      for ( auto edge_tuple : factor ) {
        uint64_t source = std::get<0> ( edge_tuple );
        // if ( inputs . count ( source ) ) {
        //   throw std::runtime_error ( "Problem parsing network specification file: Repeated inputs in logic" );
        // }
        inputs . insert ( source );
      }
    }
    //std::cout << "\n";

    // Compare partitions by (size, max), where size is length and max is maximum index
    auto compare_partition = [](std::vector<EdgeTuple> const& lhs, std::vector<EdgeTuple> const& rhs) {
      if ( lhs . size () < rhs . size () ) return true;
      if ( lhs . size () > rhs . size () ) return false;
      auto cmp_tuple = [](const auto& a, const auto& b) {
        return std::get<0>(a) < std::get<0>(b);
      };
      EdgeTuple max_lhs = * std::max_element ( lhs.begin(), lhs.end(), cmp_tuple );
      EdgeTuple max_rhs = * std::max_element ( rhs.begin(), rhs.end(), cmp_tuple );
      if ( std::get<0>(max_lhs) < std::get<0>(max_rhs) ) return true;
      if ( std::get<0>(max_lhs) > std::get<0>(max_rhs) ) return false;  /* unreachable -> */ return false;
    };

    // Put the logic struct into a canonical ordering.
    std::sort ( logic_struct.begin(), logic_struct.end(), compare_partition );
    data_ -> logic_by_index_ . push_back ( logic_struct );
    //std::cout << "The logic_struct has been incorporated into the network.\n";
    ++ target;
  }

  // Compute inputs and outputs.
  data_ -> inputs_ . resize ( size () );
  data_ -> outputs_ . resize ( size () );
  data_ -> input_instances_ . resize ( size () );
  for ( target = 0; target < size (); ++ target ) {
    // Keep track of repeated inputs to target
    std::unordered_map<uint64_t, uint64_t> input_counts;
    for ( auto const& factor : logic_by_index ( target ) ) {
      for ( EdgeTuple edge_tuple : factor ) {
        uint64_t source = std::get<0> ( edge_tuple );
        bool parity = std::get<1> ( edge_tuple );
        bool edge_sign = std::get<2> ( edge_tuple );
        uint64_t instance = 0; // Keep track of input instances
        if ( input_counts . find (source) != input_counts . end() ) {
          instance = input_counts . find (source) -> second;
        }
        input_counts [source] = instance + 1;
        data_ -> inputs_[target] . push_back ( source );
        data_ -> outputs_[source] . push_back ( target );
        data_ -> input_instances_[target] . push_back ( instance );
        data_ -> edge_type_[std::make_tuple(source, target, instance)] = parity;
        data_ -> edge_sign_[std::make_tuple(source, target, instance)] = edge_sign;
        // Output order of this instance of edge (source, target)
        data_ -> order_[std::make_tuple(source, target, instance)] = data_ -> outputs_[source].size() - 1;
      }
    }
  }
  //std::cout << "_parse complete.\n";
}

INLINE_IF_HEADER_ONLY std::ostream& operator << ( std::ostream& stream, Network const& network ) {
  stream << "[";
  bool first1 = true;
  for ( uint64_t v = 0; v < network.size (); ++ v ) {
    if ( first1 ) first1 = false; else stream << ",";
    stream << "[\"" << network.name(v) << "\","; // node
    std::vector<std::vector<EdgeTuple>> logic_struct = network.logic_by_index (v);
    stream << "["; // logic_struct
    bool first2 = true;
    for ( auto const& term : logic_struct ) {
      if ( first2 ) first2 = false; else stream << ",";
      stream << "["; // factor
      bool first3 = true;
      for ( EdgeTuple edge_tuple : term ) {
        uint64_t source = std::get<0> ( edge_tuple );
        bool edge_interaction = std::get<1> ( edge_tuple );
        if ( first3 ) first3 = false; else stream << ",";
        std::string head = edge_interaction ? "" : "~";
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
