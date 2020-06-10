/// Network.hpp
/// Shaun Harker
/// 2015-05-22
///
/// Marcio Gameiro
/// 2020-05-31

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

INLINE_IF_HEADER_ONLY bool Network::
ptm_edge ( uint64_t source, uint64_t target ) const {
  return data_ -> ptm_edge_ . find ( std::make_pair ( source, target ) ) -> second;
}

INLINE_IF_HEADER_ONLY bool Network::
decay_edge ( uint64_t source, uint64_t target ) const {
  return data_ -> decay_edge_ . find ( std::make_pair ( source, target ) ) -> second;
}

INLINE_IF_HEADER_ONLY uint64_t Network::
order ( uint64_t source, uint64_t target ) const {
  return data_ -> order_ . find ( std::make_pair ( source, target ) ) -> second;
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
    result << "\"" << name(i) << "\"" << " [style=filled, fillcolor=" << theme[1] << "];\n";
  }
  std::string normalhead ("normal");
  std::string blunthead ("tee");
  std::string dashed ("dashed");
  std::string solid ("solid");
  // std::cout << "graphviz. Looping through edges.\n";
  for ( uint64_t target = 0; target < size (); ++ target ) {
    std::vector<std::vector<uint64_t>> logic_struct = logic ( target );
    std::reverse ( logic_struct . begin (), logic_struct . end () ); // prefer black
    uint64_t partnum = 0;
    for ( auto const& part : logic_struct ) {
      for ( uint64_t source : part ) {
        // std::cout << "Checking type of edge from " << source << " to " << target << "\n";
        // Get edge interaction and sign
        bool e_intr = interaction (source, target);
        bool e_sign = edge_sign (source, target);
        bool e_decay = decay_edge (source, target);
        // Set arrow head style based on interaction and sign
        std::string head = ( e_decay != (e_intr == e_sign) ) ? normalhead : blunthead;
        // std::string head = interaction (source, target) ? normalhead : blunthead;
        std::string line_style = ptm_edge (source, target) ? dashed : solid;
        result << "\"" << name(source) << "\" -> \"" << name(target) << "\" [color=" << theme[partnum+2]
               << ", style=\"" << line_style << "\"" << ", arrowhead=\"" << head << "\"];\n";
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
      logic_strings . push_back ( " " );
    } else {
      logic_strings . push_back ( splitline[1] );
    }
    // std::cout << line << " has " << splitline.size() << " parts.\n";
    if ( splitline . size () >= 3 ) {
      // TODO: make it check for keyword "essential"
      essential_nodes [ splitline[0] ] = true;
      // std::cout << "Marking " << splitline[0] << " as essential \n";
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
  // Example: a + ~ b c d + e corresponds to (a+~b)(c)(d+e)
  // PTM input nodes are inside []
  // Decay PTM edges are in between < and >
  // Example < [a] - [b] > + c [d] e
  uint64_t target = 0;
  for ( auto const& logic_string : logic_strings ) {
    // std::cout << "Processing " << logic_string << "\n";
    std::vector<std::vector<uint64_t>> logic_struct;
    std::vector<uint64_t> factor;
    std::string token;
    bool ptm_edge = false;
    bool decay_term = false;
    bool sign_edge = true; // true for positive edges, false for negative
    bool sign_term = true;
    bool parity = true;
    bool appending = true;

    auto flush_factor = [&] () {
      if ( factor . empty () ) return;
      // Put factor into canonical ordering
      std::sort ( factor.begin(), factor.end() );
      logic_struct . push_back ( factor );
      // std::cout << " Flushing factor ";
      // for ( uint64_t i : factor ) std::cout << name ( i ) << " ";
      // std::cout << "\n";
      factor . clear ();
    };

    auto flush_token = [&] () {
      if ( token . empty () ) return;
      if ( token . front () == '[' ) {
        if ( token . back () == ']' ) {
          // Remove [ and ] from token
          token . erase ( token . begin () );   // Remove [
          token . erase ( token . end () - 1 ); // Remove ]
          ptm_edge = true;
        } else if ( token . back () != '\n' ) {
          // If ends in \n there is no matching closing ]
          // so we continue and process the wrong token below
          // (it will abort as an invalid input variable).
          // If it does not end in \n we return the
          // incomplete token to be completed
          return;
        }
      }
      if ( token . front () == '~' ) {
      	token . erase ( token . begin () );   // Remove ~
      	parity = false;
      }
      if ( not appending ) flush_factor ();
      // std::cout << " Flushing token " << token << "\n";
      if ( data_ -> index_by_name_ . count ( token ) == 0 ) {
        throw std::runtime_error ( "Problem parsing network specification: " 
                                   "Invalid input variable " + token );
      }
      if ( factor . size() ) { // Only applies to decay edges (non-decay are all positive)
      	if ( sign_term != sign_edge ) { // All edges in a term must have same sign
      	  throw std::runtime_error ( "Problem parsing network specification: "
      	  	                         "All edges in a term must have the same sign" );
      	}
      } else { // Term sign is term of first edge
      	sign_term = sign_edge;
      }
      uint64_t source = data_ -> index_by_name_ [ token ];
      factor . push_back ( source );
      data_ -> edge_type_ [std::make_pair( source, target )] = parity;
      data_ -> edge_sign_ [std::make_pair( source, target )] = sign_edge;
      data_ -> ptm_edge_ [std::make_pair( source, target )] = ptm_edge;
      data_ -> decay_edge_ [std::make_pair( source, target )] = decay_term;
      // std::cout << "Creating edge from " << source << " to " << target << "\n";
      token . clear ();
      appending = false;
      parity = true;
      ptm_edge = false;
    };

    // Add \n as the last character
    char last_c = ' ';
    for ( char c : (logic_string + '\n') ) {
      // std::cout << "Reading character " << c << "\n";
      if ( ( c == '\n' ) || ( c == '\t' ) || ( c == ' ' ) || ( c == '(' ) ||
           ( c == ')' ) || ( c == '+' ) || ( c == '-' ) || ( c == '~' ) ||
           ( c == ']' ) || ( c == '>' ) ) {
        if ( ( c == '\n' ) and ( token . front () == '[' ) ) { // No matching closing ]
          // Append \n to cause flush_token to print the wrong token and abort
          token . push_back ( c );
        }
        if ( ( c == '>' ) and ( not decay_term ) ) { // No matching opening <
          throw std::runtime_error ( "Problem parsing network specification: No opening <" );
        }
        if ( ( c == '\n' ) and decay_term ) { // No matching closing >
          throw std::runtime_error ( "Problem parsing network specification: No closing >" );
        }
        if ( ( c == '-' ) and ( not decay_term ) ) { // No - allowed on regular terms
          throw std::runtime_error ( "Problem parsing network specification: No negative term allowed for non-decay edge" );
        }
        if ( c == ']' ) {
          token . push_back ( c );
        }
        flush_token ();
      } else if ( c == '<' ) { // Start reading decay edges
        if ( last_c != ' ' ) { // Decay term no at the beginning
          throw std::runtime_error ( "Problem parsing network specification: Decay term must be at the beginning" );
        }
        decay_term = true;
      } else {
        token . push_back ( c );
      }
      if ( c == '>' ) { // Stop reading decay edges
        flush_factor ();
        decay_term = false;
      }
      if ( c == '+' ) {
        appending = true;
        sign_edge = true;
        // std::cout << " Detected +\n";
      }
      if ( c == '-' ) {
        appending = true;
        sign_edge = false;
      }
      if ( c == '~' ) parity = false;
      if ( not ( ( c == '\n' ) || ( c == '\t' ) || ( c == ' ' ) ) )
        last_c = c;
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
          throw std::runtime_error ( "Problem parsing network specification: Repeated inputs in logic" );
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
    // Put factors with negative decay edges before factors with
    // positive decay edges and all decay before regular ones
    std::vector<std::vector<uint64_t>> logic_struct_neg;
    std::vector<std::vector<uint64_t>> logic_struct_pos;
    std::vector<std::vector<uint64_t>> logic_struct_reg;
    for ( auto const& factor : logic_struct ) {
      auto source = factor . front();
      if ( decay_edge ( source, target ) ) { // Decay edge
        if ( edge_sign ( source, target ) ) { // Positive edge
          logic_struct_pos . push_back ( factor );
        } else { // Negative edge
          logic_struct_neg . push_back ( factor );
        }
      } else { // Regular edge
        logic_struct_reg . push_back ( factor );
      }
    }
    // Insert logic for positive edges after negative edges
    logic_struct_neg . insert ( logic_struct_neg . end (), logic_struct_pos . begin (), logic_struct_pos . end ());
    // Insert logic for regular edges after positive edges
    logic_struct_neg . insert ( logic_struct_neg . end (), logic_struct_reg . begin (), logic_struct_reg . end ());
    data_ -> logic_by_index_ . push_back ( logic_struct_neg );
    // std::cout << "The logic_struct has been incorporated into the network.\n";
    ++ target;
  }
  // Compute inputs and outputs.
  data_ -> inputs_ . resize ( size () );
  data_ -> outputs_ . resize ( size () );
  for ( target = 0; target < size (); ++ target ) {
    for ( auto const& factor : logic ( target ) ) {
      for ( uint64_t source : factor ) {
        data_ -> inputs_[target] . push_back ( source );
        data_ -> outputs_[source] . push_back ( target );
        data_ -> order_[std::make_pair(source,target)] = data_ -> outputs_[source].size()-1;
      }
    }
  }
  // std::cout << "_parse complete.\n";
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
