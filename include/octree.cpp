#include "octree.hpp"

namespace OCTree {
  template<>
  const std::array<std::array<int, 1>, 2> cartesian_product<1>::product = {{ {{-1}}, {{1}} }};
  template<>
  const std::array<std::array<int, 2>, 4> cartesian_product<2>::product = {{ {{-1,-1}}, {{ 1,-1}}, {{ 1, 1}}, {{-1, 1}} }};
  template<>
  const std::array<std::array<int, 3>, 8> cartesian_product<3>::product = {{ {{-1,-1,-1}}, {{-1, 1,-1}}, {{-1, 1, 1}}, {{-1,-1, 1}}, {{1,-1,-1}}, {{1, 1,-1}}, {{1, 1, 1}}, {{1,-1, 1}} }};
}



