#ifndef REAL_FUNCTION_HPP
#define REAL_FUNCTION_HPP

#include "fwd_decl.hpp"

namespace beagle
{
  namespace math
  {
    struct RealFunction
    {
      RealFunction( void );
      virtual ~RealFunction( void );
    public:
      virtual beagle::dbl_t value( beagle::dbl_t arg ) const = 0;
    public:
      static beagle::real_function_ptr_t createConstantFunction( beagle::dbl_t constant );
      static beagle::real_function_ptr_t createUnaryFunction( const beagle::real_func_t& func );
      static beagle::real_function_ptr_t createCompositeFunction( const beagle::real_function_ptr_t& f,
                                                                  const beagle::real_function_ptr_t& g );
      static beagle::real_function_ptr_t createLinearWithFlatExtrapolationInterpolatedFunction(
                                                         const beagle::dbl_vec_t& xValues,
                                                         const beagle::dbl_vec_t& yValues );
      static beagle::real_function_ptr_t createNaturalCubicSplineWithFlatExtrapolationInterpolatedFunction(
                                                         const beagle::dbl_vec_t& xValues,
                                                         const beagle::dbl_vec_t& yValues );
      static beagle::real_function_ptr_t createPiecewiseConstantRightInterpolatedFunction(
                                                         const beagle::dbl_vec_t& xValues,
                                                         const beagle::dbl_vec_t& yValues );
      static beagle::real_function_ptr_t createContinuousForwardAssetPriceFunction(
                                                         beagle::dbl_t spot,
                                                         const beagle::real_function_ptr_t& funding );
      static beagle::real_function_ptr_t createGeneralForwardAssetPriceFunction(
                                                         beagle::dbl_t spot,
                                                         const beagle::real_function_ptr_t& funding,
                                                         const beagle::discrete_dividend_schedule_t& dividends );
    };

    namespace mixins
    {
      struct InterpolationParameters
      {
        virtual ~InterpolationParameters( void );
      public:
        virtual const beagle::dbl_vec_t& xParameters( void ) const = 0;
        virtual const beagle::dbl_vec_t& yParameters( void ) const = 0;
      };

      struct DividendSchedule
      {
        virtual ~DividendSchedule( void );
      public:
        virtual const beagle::discrete_dividend_schedule_t& dividendSchedule( void ) const = 0;
      };
    }

    namespace impl
    {
      struct InterpolatedFunction : public RealFunction,
                                    public beagle::math::mixins::InterpolationParameters
      {
        InterpolatedFunction( const beagle::dbl_vec_t& xValues,
                              const beagle::dbl_vec_t& yValues );
        virtual ~InterpolatedFunction( void );
      public:
        virtual beagle::dbl_t value( beagle::dbl_t arg ) const = 0;
        virtual const beagle::dbl_vec_t& xParameters( void ) const override;
        virtual const beagle::dbl_vec_t& yParameters( void ) const override;
      private:
        beagle::dbl_vec_t m_XValues;
        beagle::dbl_vec_t m_YValues;
      };
    }
  }
}


#endif