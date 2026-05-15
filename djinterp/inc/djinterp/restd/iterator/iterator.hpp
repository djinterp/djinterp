/***********************************************************************
* restd                                                            iterator.hpp
*
* umbrella header for restd's <iterator> implementation.
*
* current contents (Phase 7a + Phase 7b):
*   tag types:
*     input_iterator_tag, output_iterator_tag, forward_iterator_tag,
*     bidirectional_iterator_tag, random_access_iterator_tag,
*     contiguous_iterator_tag (C++20+).
*   iterator_traits (primary + raw-pointer specs).
*   stepping:
*     advance, distance, next, prev.
*   range access:
*     begin, end, cbegin, cend, rbegin, rend, crbegin, crend,
*     size, empty, data.
*   adaptors:
*     reverse_iterator, make_reverse_iterator (Phase 7a),
*     move_iterator, make_move_iterator (Phase 7b),
*     back_insert_iterator, back_inserter (Phase 7b),
*     front_insert_iterator, front_inserter (Phase 7b),
*     insert_iterator, inserter (Phase 7b).
*
* not yet implemented:
*   stream iterators: istream_iterator, ostream_iterator,
*     istreambuf_iterator, ostreambuf_iterator (await <iostream>),
*   common_iterator, counted_iterator (C++20+ ranges machinery),
*   concept-based iterator detection (C++20+ concepts).
*
*
* path:      /inc/restd/iterator.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_
#define RESTD_ITERATOR_ 1

#include "djinterp.hpp"

// tag types
#include "restd/iterator/input_iterator_tag.hpp"
#include "restd/iterator/output_iterator_tag.hpp"
#include "restd/iterator/forward_iterator_tag.hpp"
#include "restd/iterator/bidirectional_iterator_tag.hpp"
#include "restd/iterator/random_access_iterator_tag.hpp"
#include "restd/iterator/contiguous_iterator_tag.hpp"

// traits
#include "restd/iterator/iterator_traits.hpp"

// stepping
#include "restd/iterator/advance.hpp"
#include "restd/iterator/distance.hpp"
#include "restd/iterator/next.hpp"
#include "restd/iterator/prev.hpp"

// range access
#include "restd/iterator/begin.hpp"
#include "restd/iterator/end.hpp"
#include "restd/iterator/cbegin.hpp"
#include "restd/iterator/cend.hpp"
#include "restd/iterator/rbegin.hpp"
#include "restd/iterator/rend.hpp"
#include "restd/iterator/crbegin.hpp"
#include "restd/iterator/crend.hpp"
#include "restd/iterator/size.hpp"
#include "restd/iterator/empty.hpp"
#include "restd/iterator/data.hpp"

// adaptors
#include "restd/iterator/reverse_iterator.hpp"
#include "restd/iterator/move_iterator.hpp"
#include "restd/iterator/back_insert_iterator.hpp"
#include "restd/iterator/front_insert_iterator.hpp"
#include "restd/iterator/insert_iterator.hpp"

#endif  // RESTD_ITERATOR_
