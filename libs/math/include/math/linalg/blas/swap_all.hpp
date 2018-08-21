#pragma once

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"

namespace fetch
{
namespace math
{
namespace linalg 
{


template<typename S, uint64_t V>
class Blas< S, 
            Signature( _X, _Y <= _n, _X, _m, _Y, _p ),
            Computes( _X, _Y = _Y, _X ), 
            V>
{
public:
  using type = S;
  using vector_register_type = typename Matrix< type >::vector_register_type;
  
  void operator()(int const &n, ShapeLessArray< type > &dx, int const &incx, ShapeLessArray< type > &dy, int const &incy ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch