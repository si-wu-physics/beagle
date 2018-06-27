#ifndef REAL_2D_FUNCTION_HPP
#define REAL_2D_FUNCTION_HPP

#include "fwd_decl.hpp"

namespace beagle
{
  namespace math
  {
    struct RealTwoDimFunction
    {
      RealTwoDimFunction( void );
      virtual ~RealTwoDimFunction( void );
    public:
      virtual double value( double argX,
                            double argY ) const = 0;
    public:
      static beagle::real_2d_function_ptr_t createTwoDimConstantFunction( double constant );
    };
  }
}


#endif