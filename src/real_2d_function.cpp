#include "real_2d_function.hpp"
#include "real_function.hpp"

namespace beagle
{
  namespace math
  {
    namespace impl
    {
      struct BinaryFunction : public RealTwoDimFunction
      {
        explicit BinaryFunction(const beagle::real_2d_func_t& func) :
          m_Func(func)
        { }
      public:
        virtual double value(double argX,
                             double argY) const override
        {
          return m_Func(argX, argY);
        }
      private:
        beagle::real_2d_func_t m_Func;
      };

      struct TwoDimConstantFunction : public RealTwoDimFunction
      {
        explicit TwoDimConstantFunction( double constant ) :
          m_Const( constant )
        { }
      public:
        virtual double value( double argX,
                              double argY ) const override
        {
          return m_Const;
        }
      private:
        double m_Const;
      };

      struct PiecewiseConstantRightFunction : public RealTwoDimFunction
      {
        PiecewiseConstantRightFunction( const beagle::dbl_vec_t& params,
                                        const beagle::real_function_ptr_coll_t& funcs ) :
          m_Params( params ),
          m_Funcs( funcs )
        { }
      public:
        virtual double value( double argX,
                              double argY ) const override
        {
          auto it = std::lower_bound(m_Params.cbegin(),
                                     m_Params.cend(),
                                     argX);

          if (it == m_Params.cend())
            return m_Funcs.back()->value(argY);
          else
          {
            beagle::dbl_vec_t::difference_type diff = it - m_Params.cbegin();
            auto jt = m_Funcs.cbegin();
            std::advance(jt, diff);
            return (*jt)->value(argY);
          }
        }
      private:
        beagle::dbl_vec_t m_Params;
        beagle::real_function_ptr_coll_t m_Funcs;
      };
    }

    beagle::real_2d_function_ptr_t
    RealTwoDimFunction::createBinaryFunction( const beagle::real_2d_func_t& func )
    {
      return std::make_shared<impl::BinaryFunction>( func );
    }

    beagle::real_2d_function_ptr_t
    RealTwoDimFunction::createTwoDimConstantFunction( double constant )
    {
      return std::make_shared<impl::TwoDimConstantFunction>( constant );
    }

    beagle::real_2d_function_ptr_t
    RealTwoDimFunction::createPiecewiseConstantRightFunction( const beagle::dbl_vec_t& params,
                                                              const beagle::real_function_ptr_coll_t& funcs )
    {
      if (params.size() != funcs.size())
        throw(std::string("Cannot form piecewise constant function with unequal numbers of parameters and functions!"));

      return std::make_shared<impl::PiecewiseConstantRightFunction>( params, funcs );
    }
  }
}