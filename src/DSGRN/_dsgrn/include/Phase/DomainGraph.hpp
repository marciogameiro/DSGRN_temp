/// DomainGraph.hpp
/// Shaun Harker
/// 2015-05-24

#pragma once

#ifndef INLINE_IF_HEADER_ONLY
#define INLINE_IF_HEADER_ONLY
#endif

#include "DomainGraph.h"

INLINE_IF_HEADER_ONLY DomainGraph::
DomainGraph ( void ) {
  data_ . reset ( new DomainGraph_ );
}

INLINE_IF_HEADER_ONLY DomainGraph::
DomainGraph ( Parameter const& parameter ) {
  assign ( parameter );
}

INLINE_IF_HEADER_ONLY void DomainGraph::
assign ( Parameter const& parameter ) {
  // Check if there is a self repressing edge
  uint64_t dim = parameter . network () . size ();
  bool self_repressor = false;
  for ( uint64_t d = 0; d < dim; ++ d ) {
    // Check if node d is a self repressor
    std::vector<uint64_t> inputs = parameter . network () . inputs ( d );
    if ( ( std::find( inputs . begin(), inputs . end(), d ) != inputs . end() ) and
         ( not parameter . network () . interaction ( d, d ) ) ) { // and not activating
      self_repressor = true;
    }
  }
  if ( self_repressor ) { // If self repressor call assign_extended
    assign_extended ( parameter );
  } else { // Otherwise use this assign
    data_ . reset ( new DomainGraph_ );
    data_ -> parameter_ = parameter;
    uint64_t D = parameter . network () . size ();
    data_ -> dimension_ = D;
    std::vector<uint64_t> limits = parameter . network() . domains ();
    std::vector<uint64_t> jump ( D ); // index offset in each dim
    data_ -> self_repressor_ . resize (D, false);
    uint64_t N = 1;
    for ( uint64_t d = 0; d < D; ++ d ) {
      jump[d] =  N;
      N *=  limits [ d ];
      data_ -> direction_ [ jump[d] ] = d;
    }
    data_ -> digraph_ = Digraph ();
    data_ -> digraph_ . resize ( N );
    data_ -> labelling_ = parameter . labelling ();
    Digraph & digraph = data_ -> digraph_;
    std::vector<uint64_t> & labelling = data_ -> labelling_;
    for ( uint64_t i = 0; i < N; ++ i ) {
      if ( labelling [ i ] == 0 ) {
        digraph . add_edge ( i, i );
      }
      uint64_t leftbit = 1;
      uint64_t rightbit = (1LL << D);
      for ( int d = 0; d < D; ++ d, leftbit <<= 1, rightbit <<= 1 ) {
        if ( labelling [ i ] & rightbit ) {
          uint64_t j = i + jump[d];
          if ( not (labelling [ j ] & leftbit) ) {
            digraph . add_edge ( i, j );
          }
        }
        if ( labelling [ i ] & leftbit ) {
          uint64_t j = i - jump[d];
          if ( not (labelling [ j ] & rightbit) ) {
            digraph . add_edge ( i, j );
          }
        }
      }
    }
    digraph . finalize ();
  }
}

// Create an extended phase space decomposition by adding a double wall
// (double threshold) for each threshold corresponding to a self repressor.
// The parameter labelling refers to the original domain. We loop through
// each cell, dom_ext, in the extended domain and set the labelling of the
// extended domain cell dom_ext follows:
//
// 1) If the extended domain corresponds to a regular domains, just set
// the labels from the labelling of the corresponding original domain.
//
// 2) If the extended domain dom_ext does not correspond to a regular domain,
// then find the first dimension d_thres in which a self repressing threshold
// is crossed to form this domain and find the previous, dom_ext_prev, and the
// next, dom_ext_next, domains along this dimension. If the labellings of
// dom_ext_prev or dom_ext_next are not set, call the appropriate function
// (recursively if necessary) to set them. Once they are set use their
// labellings to set the labeling of the domain dom_ext.
INLINE_IF_HEADER_ONLY void DomainGraph::
assign_extended ( Parameter const& parameter ) {
  data_ . reset ( new DomainGraph_ );
  data_ -> parameter_ = parameter;
  data_ -> labelling_ = parameter . labelling ();
  uint64_t D = parameter . network () . size ();
  data_ -> dimension_ = D;
  std::vector<uint64_t> limits = parameter . network() . domains ();
  std::vector<uint64_t> limits_ext = parameter . network() . domains ();  // Extended domain
  std::vector<uint64_t> self_thres_pos ( D );
  std::vector<uint64_t> jump ( D ); // index offset in each dim
  data_ -> self_repressor_ . resize (D, false);
  data_ -> jump_ext_ . resize ( D );
  uint64_t N = 1;
  uint64_t N_ext = 1;
  for ( uint64_t d = 0; d < D; ++ d ) {
    // Check if node d is a self repressor and get position of self threshold
    std::vector<uint64_t> inputs = parameter . network () . inputs ( d );
    if ( std::find( inputs . begin(), inputs . end(), d ) != inputs . end() ) {
      uint64_t source = d;
      // bool activating = parameter . network () . interaction ( source, d );
      if ( not parameter . network () . interaction ( source, d ) ) {  // if not activating
        data_ -> self_repressor_ [d] = true;
        uint64_t outorder = parameter . network () . order ( source, d );
        self_thres_pos [d] = parameter . order() [ source ] . inverse ( outorder );
      }
    }
    limits_ext [ d ] = not data_ -> self_repressor_ [d] ? limits [ d ] : limits [ d ] + 1;
    jump [d] =  N;
    data_ -> jump_ext_ [d] =  N_ext;
    N *=  limits [d];
    N_ext *=  limits_ext [d];
    // data_ -> direction_ [ data_ -> jump_ext_ [d] ] = d;
    data_ -> direction_ [ jump[d] ] = d;
  }
  data_ -> N_ = N;

  // dom_from_ext_dom
  //   Given an extended domain index dom_ext get the regular domain
  //   index dom if the extended domain is a regular domain. If dom_ext
  //   is not a regular domain returns the number N of regular domains
  auto dom_from_ext_dom = [&]( uint64_t dom_ext ) {
    std::vector<uint64_t> coords ( D );
    std::vector<uint64_t> coords_ext ( D );
    uint64_t i_ext = dom_ext;
    uint64_t dom = 0;
    for ( uint64_t d = 0; d < D; ++d ) {
      coords_ext [d] = i_ext % limits_ext [d];
      i_ext = i_ext / limits_ext [d];
      if ( not data_ -> self_repressor_ [d] ) {
        coords [d] = coords_ext [d];
      } else {
        if ( coords_ext [d] == self_thres_pos [d] + 1 )
          return N; // not a regular domain
        if ( coords_ext [d] < self_thres_pos [d] + 1 )
          coords [d] = coords_ext [d];
        else  // coords_ext [d] > self_thres_pos [d] + 1
          coords [d] = coords_ext [d] - 1;
      }
      dom += coords [d] * jump [d];
    }
    return dom;
  };

  // first_dim_self_threshold
  //   Given a non-regular extended domain dom_ext get the
  //   first dimension in which a self repressing threshold
  //   is crossed to form this domain. If the dom_ext is a
  //   regular domain returns the space dimension D
  auto first_dim_self_threshold = [&]( uint64_t dom_ext ) {
    uint64_t i_ext = dom_ext;
    for ( uint64_t d = 0; d < D; ++d ) {
      uint64_t coord_ext = i_ext % limits_ext [d];
      i_ext = i_ext / limits_ext [d];
      // Crossed a self repressor if coord_ext == self_thres_pos [d] + 1
      if ( data_ -> self_repressor_ [d] and (coord_ext == self_thres_pos [d] + 1) )
        return d;
    }
    return D;
  };

  // Set regular domain index for regular extended domains
  // and set the first dimension of self repressing threshold
  for ( uint64_t dom_ext = 0; dom_ext < N_ext; ++dom_ext ) {
    // Get regular domain index, if any
    // Set as N if dom_ext is not regular
    uint64_t dom = dom_from_ext_dom (dom_ext);
    data_ -> regular_domain_ . push_back (dom);
    // Get the first dimension of a self repressing threshold
    // Set as D if dom_ext is a regular extended domain
    uint64_t d_thres = first_dim_self_threshold (dom_ext);
    data_ -> dim_self_threshold_ . push_back (d_thres);
  }

  // For each domain index we construct vectors of bools,
  // for the left walls and the right walls of that
  // domain, that determine if the wall is an entrance
  // wall or and absorbing wall. A value of true means
  // absorbing and false means entrance.
  data_ -> label_left_wall_ . resize (N_ext);
  data_ -> label_right_wall_ . resize (N_ext);

  // Set the labellings of all the extended domains
  for ( uint64_t dom_ext = 0; dom_ext < N_ext; ++dom_ext ) {
    // Get regular domain index, if any
    uint64_t dom = data_ -> regular_domain_ [dom_ext];
    if ( dom < N ) // dom_ext is a regular domain
      _setRegularDomainLabelling ( dom_ext, dom );
    else
      _setNonRegularDomainLabelling ( dom_ext );
  }

  // attracting_domain
  //   Return true if the domain is attracting and
  //   false otherwise
  auto attracting_domain = [&] ( uint64_t dom_ext ) {
    // If any left wall is absorbing return false
    if ( std::any_of( data_ -> label_left_wall_ [dom_ext] . begin(),
         data_ -> label_left_wall_ [dom_ext] . end(), [](bool v) { return v; } ) )
      return false;
    // If any right wall is absorbing return false
    if ( std::any_of( data_ -> label_right_wall_ [dom_ext] . begin(),
         data_ -> label_right_wall_ [dom_ext] . end(), [](bool v) { return v; } ) )
      return false;
    return true;
  };

  // Set the state transition graph
  data_ -> digraph_ = Digraph ();
  data_ -> digraph_ . resize ( N_ext );
  Digraph & digraph = data_ -> digraph_;

  // Using the labelling we set the edges in the digraph
  for ( uint64_t i_ext = 0; i_ext < N_ext; ++ i_ext ) {
    if ( attracting_domain ( i_ext ) ) {
      digraph . add_edge ( i_ext, i_ext );
    }
    for ( uint64_t d = 0; d < D; ++ d ) {
      if ( data_ -> label_right_wall_ [i_ext] [d] ) {
      	uint64_t j_ext = i_ext + data_ -> jump_ext_ [d];
        if ( not data_ -> label_left_wall_ [j_ext] [d] ) {
          digraph . add_edge ( i_ext, j_ext );
        }
      }
      if ( data_ -> label_left_wall_ [i_ext] [d] ) {
      	uint64_t j_ext = i_ext - data_ -> jump_ext_ [d];
        if ( not data_ -> label_right_wall_ [j_ext] [d] ) {
          digraph . add_edge ( i_ext, j_ext );
        }
      }
    }
  }
  digraph . finalize ();
}

INLINE_IF_HEADER_ONLY Parameter const DomainGraph::
parameter ( void ) const {
  return data_ -> parameter_;
}

INLINE_IF_HEADER_ONLY Digraph const DomainGraph::
digraph ( void ) const {
  return data_ -> digraph_;
}

INLINE_IF_HEADER_ONLY uint64_t DomainGraph::
dimension ( void ) const {
  return data_ -> dimension_;
}

INLINE_IF_HEADER_ONLY std::vector<uint64_t> DomainGraph::
coordinates ( uint64_t domain ) const {
  std::vector<uint64_t> result ( dimension () );
  std::vector<uint64_t> limits = data_ -> parameter_ . network() . domains ();
  for ( int d = 0; d < dimension(); ++ d ) {
    limits[d] += data_ -> self_repressor_ [d] ? 1 : 0; // Add 1 if self repressor
    result[d] = domain % limits[d];
    domain = domain / limits[d];
  }
  return result;
}

INLINE_IF_HEADER_ONLY uint64_t DomainGraph::
label ( uint64_t domain ) const {
  // Return 0 for non regular domains
  uint64_t domain_reg = domain;
  if ( not data_ -> regular_domain_ . empty () ) {
    domain_reg = data_ -> regular_domain_ [domain];
    if ( domain_reg == data_ -> N_ ) // Not a regular domain
      return 0;
  }
  return data_ -> labelling_ [ domain_reg ];
}

INLINE_IF_HEADER_ONLY uint64_t DomainGraph::
label ( uint64_t source, uint64_t target ) const {
  // Return 0 for non regular domains
  uint64_t source_reg = source;
  uint64_t target_reg = target;
  if ( not data_ -> regular_domain_ . empty () ) {
    source_reg = data_ -> regular_domain_ [source];
    target_reg = data_ -> regular_domain_ [target];
    // If any domain is not regular
    if ( ( source_reg == data_ -> N_ ) or ( target_reg == data_ -> N_ ) )
      return 0;
  }
  if ( source_reg == target_reg ) return 0;
  uint64_t i = direction(source_reg, target_reg);
  uint64_t j = regulator(source_reg, target_reg);
  if ( i == j ) return 0;
  return 1L << ( j + ( ((source_reg < target_reg) ^ parameter().network().interaction(i,j)) ? 0 : dimension() ) );
}

INLINE_IF_HEADER_ONLY uint64_t DomainGraph::
direction ( uint64_t source, uint64_t target ) const {
  // Return dimension() for non regular domains
  uint64_t source_reg = source;
  uint64_t target_reg = target;
  if ( not data_ -> regular_domain_ . empty () ) {
    source_reg = data_ -> regular_domain_ [source];
    target_reg = data_ -> regular_domain_ [target];
    // If any domain is not regular
    if ( ( source_reg == data_ -> N_ ) or ( target_reg == data_ -> N_ ) )
      return dimension ();
  }
  if ( source_reg == target_reg ) return dimension ();
  return data_ -> direction_ [ std::abs((int64_t)source_reg-(int64_t)target_reg) ];
}

INLINE_IF_HEADER_ONLY uint64_t DomainGraph::
regulator ( uint64_t source, uint64_t target ) const {
  // Return dimension() for non regular domains
  uint64_t source_reg = source;
  uint64_t target_reg = target;
  if ( not data_ -> regular_domain_ . empty () ) {
    source_reg = data_ -> regular_domain_ [source];
    target_reg = data_ -> regular_domain_ [target];
    // If any domain is not regular
    if ( ( source_reg == data_ -> N_ ) or ( target_reg == data_ -> N_ ) )
      return dimension ();
  }
  if ( source_reg == target_reg ) return dimension ();
  std::vector<uint64_t> limits = data_ -> parameter_ . network() . domains ();
  uint64_t variable = direction ( source_reg, target_reg );
  uint64_t domain = std::min(source_reg, target_reg);
  for ( int d = 0; d < variable; ++ d ) domain = domain / limits[d];
  uint64_t threshold = domain % limits[variable];
  return data_ -> parameter_ . regulator ( variable, threshold );
}

INLINE_IF_HEADER_ONLY Annotation const DomainGraph::
annotate ( Component const& vertices ) const {
  uint64_t D = data_ -> parameter_ . network() . size ();
  std::vector<uint64_t> limits = data_ -> parameter_ . network() . domains ();
  std::vector<uint64_t> domain_indices ( vertices.begin(), vertices.end() );
  std::vector<uint64_t> min_pos(D);
  std::vector<uint64_t> max_pos(D);
  for ( int d = 0; d < D; ++ d ) {
    limits[d] += data_ -> self_repressor_ [d] ? 1 : 0; // Add 1 if self repressor
    min_pos[d] = limits[d];
    max_pos[d] = 0;
  }
  for ( int d = 0; d < D; ++ d ) {
    for ( uint64_t & v : domain_indices ) {
      uint64_t pos = v % limits[d];
      v = v / limits[d];
      min_pos[d] = std::min(min_pos[d], pos);
      max_pos[d] = std::max(max_pos[d], pos);
    }
  }
  std::vector<uint64_t> signature;
  for ( int d = 0; d < D; ++ d ) {
    if ( min_pos[d] != max_pos[d] ) {
      signature . push_back ( d );
    }
  }
  Annotation a;
  std::stringstream ss;
  if ( signature . size () == 0 ) {
    ss << "FP { ";
    bool first_term = true;
    for ( int d = 0; d < D; ++ d ) {
      if ( first_term ) first_term = false; else ss << ", ";
      ss << min_pos[d];
    }
    ss << " }";
  } else if ( signature . size () == D ) {
    ss << "FC";
  } else {
    ss << "XC {";
    bool first_term = true;
    for ( uint64_t d : signature ) {
      if ( first_term ) first_term = false; else ss << ", ";
      ss << data_ -> parameter_ . network() . name ( d );
    }
    ss << "}";
  }
  a . append ( ss . str () );
  return a;
}

INLINE_IF_HEADER_ONLY std::string DomainGraph::
graphviz ( void ) const {
  return digraph () . graphviz (); 
}

INLINE_IF_HEADER_ONLY std::ostream& operator << ( std::ostream& stream, DomainGraph const& dg ) {
  stream << dg . digraph ();
  return stream;
}

/// _setRegularDomainLabelling
///   Set the labelling of the extended regular domain dom_ext from
///   the labelling of the original domain dom
INLINE_IF_HEADER_ONLY void DomainGraph::
_setRegularDomainLabelling ( uint64_t dom_ext, uint64_t dom ) {
  // Do nothing if labels already set
  if ( not ( data_ -> label_left_wall_ [dom_ext] . empty () or
             data_ -> label_right_wall_ [dom_ext] . empty ()   ) )
    return;
  std::vector<uint64_t> & labelling = data_ -> labelling_;
  uint64_t D = data_ -> dimension_;
  data_ -> label_left_wall_ [dom_ext] . resize (D, false);
  data_ -> label_right_wall_ [dom_ext] . resize (D, false);
  uint64_t leftbit = 1;
  uint64_t rightbit = (1LL << D);
  for ( uint64_t d = 0; d < D; ++ d, leftbit <<= 1, rightbit <<= 1 ) {
    if ( labelling [ dom ] & leftbit )
      data_ -> label_left_wall_ [dom_ext] [d] = true;
    if ( labelling [ dom ] & rightbit )
      data_ -> label_right_wall_ [dom_ext] [d] = true;
  }
}

/// _setNonRegularDomainLabelling
///   Set the labelling of the extended domain dom_ext
INLINE_IF_HEADER_ONLY void DomainGraph::
_setNonRegularDomainLabelling ( uint64_t dom_ext ) {
  // Do nothing if labels already set
  if ( not ( data_ -> label_left_wall_ [dom_ext] . empty () or
             data_ -> label_right_wall_ [dom_ext] . empty ()   ) )
    return;
  uint64_t D = data_ -> dimension_;
  uint64_t N = data_ -> N_;
  // dom_ext is not a regular domain (set the labellings)
  data_ -> label_left_wall_ [dom_ext] . resize (D, false);
  data_ -> label_right_wall_ [dom_ext] . resize (D, false);

  // Get first dim of self threshold defining dom_ext
  uint64_t d_thres = data_ -> dim_self_threshold_ [dom_ext];

  // Get the previous and next domains along dimension d_thres
  // (no need to check if d_thres < D since dom_ext is non regular)
  uint64_t dom_ext_prev = dom_ext - data_ -> jump_ext_ [d_thres];
  uint64_t dom_ext_next = dom_ext + data_ -> jump_ext_ [d_thres];

  // Set labelling of dom_ext_prev and dom_ext_next if not
  // already set by calling itself recursively if needed
  // Get regular domain index, if any
  uint64_t dom_prev = data_ -> regular_domain_ [dom_ext_prev];
  if ( dom_prev < N ) // dom_ext_prev is a regular domain
    _setRegularDomainLabelling ( dom_ext_prev, dom_prev );
  else // Call itself recursively
    _setNonRegularDomainLabelling ( dom_ext_prev );
  // Get regular domain index, if any
  uint64_t dom_next = data_ -> regular_domain_ [dom_ext_next];
  if ( dom_next < N ) // dom_ext_next is a regular domain
    _setRegularDomainLabelling ( dom_ext_next, dom_next );
  else // Call itself recursively
    _setNonRegularDomainLabelling ( dom_ext_next );

  // Use the labellings of dom_ext_prev and dom_ext_next to
  // set the labelling of dom_ext
  for ( uint64_t d = 0; d < D; ++d ) {
    if ( d == d_thres ) {
      // The left label of dom_ext is the opposite of the right label of dom_ext_prev
      data_ -> label_left_wall_ [dom_ext] [d] = not data_ -> label_right_wall_ [dom_ext_prev] [d];
      // The right label of dom_ext is the opposite of the left label of dom_ext_next
      data_ -> label_right_wall_ [dom_ext] [d] = not data_ -> label_left_wall_ [dom_ext_next] [d];
    } else {
      // The labels of dom_ext are set to be the labels of dom_ext_prev
      // which should be the same as the labels of dom_ext_next
      //
      // Check to make sure this is true, that is, check that:
      // data_ -> label_left_wall_ [dom_ext_prev] [d] == data_ -> label_left_wall_ [dom_ext_next] [d]
      // data_ -> label_right_wall_ [dom_ext_prev] [d] == data_ -> label_right_wall_ [dom_ext_next] [d]
      data_ -> label_left_wall_ [dom_ext] [d] = data_ -> label_left_wall_ [dom_ext_prev] [d];
      data_ -> label_right_wall_ [dom_ext] [d] = data_ -> label_right_wall_ [dom_ext_prev] [d];
    }
  }
}
