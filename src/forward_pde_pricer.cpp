#include "one_dim_pde_pricer.hpp"
#include "solver.hpp"
#include "real_function.hpp"
#include "real_2d_function.hpp"
#include "option.hpp"
#include "payoff.hpp"
#include "pricer.hpp"
#include "interpolation_builder.hpp"

#include <iostream>
#include <iterator>

namespace beagle
{
  namespace valuation
  {
    namespace impl
    {
      struct OneDimForwardPDEArrowDebreuPricer : public OneDimParabolicPDEPricer,
                                                 public beagle::valuation::mixins::OneDimFokkerPlanck
      {
        OneDimForwardPDEArrowDebreuPricer(const beagle::real_function_ptr_t& forward,
                                          const beagle::real_function_ptr_t& discounting,
                                          const beagle::real_2d_function_ptr_t& drift,
                                          const beagle::real_2d_function_ptr_t& volatility,
                                          const beagle::real_2d_function_ptr_t& rate,
                                          const beagle::valuation::OneDimFiniteDifferenceSettings& settings) :
          OneDimParabolicPDEPricer(forward,
                                   discounting,
                                   drift,
                                   volatility,
                                   rate,
                                   beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(.0),
                                   settings)
        { }
      public:
        virtual double value(const beagle::product_ptr_t& product) const override
        {
          auto pA = dynamic_cast<beagle::product::option::mixins::American*>(product.get());
          if (pA)
            throw(std::string("Cannot value an American option with a forward equation!"));

          auto pO = dynamic_cast<beagle::product::mixins::Option*>(product.get());
          if (!pO)
            throw(std::string("The incoming product is not an option!"));

          double expiry = pO->expiry();
          double strike = pO->strike();
          const beagle::payoff_ptr_t& payoff = pO->payoff();

          // Set the initial condition and perform the forwardward induction
          beagle::dbl_vec_t stateVars;
          beagle::dbl_vec_t densities;
          formInitialCondition(expiry, stateVars, densities);
          evolve(0., expiry, stateVars, densities);

          double result(.0);
          int numStateVars = stateVars.size();
          double stateVarStep = (stateVars.back() - stateVars.front()) / (numStateVars - 1);
          double forward = forwardCurve()->value(expiry);
          for (int i=0; i<numStateVars; ++i)
          {
            result += stateVarStep * densities[i] * payoff->intrinsicValue(forward * std::exp(stateVars[i]), strike);
          }

          return discountCurve()->value(expiry) * result;
        }
        virtual void formInitialCondition(double expiry,
                                          beagle::dbl_vec_t& stateVars,
                                          beagle::dbl_vec_t& density) const override
        {
          double termForward = forwardCurve()->value(expiry);

          // Set up the state variable mesh
          int numStateVars = finiteDifferenceSettings().numberOfStateVariableSteps();
          if (numStateVars % 2 == 0)
            numStateVars += 1;

          double atmVol = volatilitySurface()->value(expiry, termForward);
          int centralIndex = numStateVars / 2;
          double centralValue = 0.;
          double stateVarStep = finiteDifferenceSettings().numberOfGaussianStandardDeviations() * atmVol * std::sqrt(expiry) / centralIndex;

          stateVars.resize(numStateVars);
          for (int i=0; i<numStateVars; ++i)
            stateVars[i] = centralValue + (i - centralIndex) * stateVarStep;

          // Set the initial condition
          density.resize(numStateVars, 0.0);
          density[centralIndex] = 1. / stateVarStep;
        }
        virtual void evolve(double start,
                            double end,
                            const beagle::dbl_vec_t& stateVars,
                            beagle::dbl_vec_t& density) const override
        {
          beagle::parabolic_pde_solver_ptr_t solver
            = beagle::math::OneDimParabolicPDESolver::formOneDimFokkerPlanckPDESolver(convectionCoefficient(),
                                                                                      diffusionCoefficient(),
                                                                                      rateCoefficient());

          // Perform the forward induction
          int numTimes = static_cast<int>((end - start) * finiteDifferenceSettings().numberOfTimeSteps());
          double timeStep = (end - start) / numTimes;
          for (int i=0; i<numTimes; ++i)
          {
            double end = start + (i+1) * timeStep;
            solver->evolve(end,
                           timeStep,
                           stateVars,
                           beagle::dbl_vec_t{0.0},
                           beagle::dbl_vec_t{0.0},
                           density);
          }
        }
      };

      struct OneDimForwardPDEEuroOptionPricer : public OneDimParabolicPDEPricer,
                                                public beagle::valuation::mixins::Dupire
      {
        OneDimForwardPDEEuroOptionPricer(const beagle::real_function_ptr_t& forward,
                                         const beagle::real_function_ptr_t& discounting,
                                         const beagle::real_2d_function_ptr_t& volatility,
                                         const beagle::valuation::OneDimFiniteDifferenceSettings& settings) :
          OneDimParabolicPDEPricer(forward,
                                   discounting,
                                   beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(.0),
                                   volatility,
                                   beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(.0),
                                   beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(.0),
                                   settings)
        { }
      public:
        virtual double value(const beagle::product_ptr_t& product) const override
        {
          auto pA = dynamic_cast<beagle::product::option::mixins::American*>(product.get());
          if (pA)
            throw(std::string("Cannot value an American option with a forward equation!"));

          auto pO = dynamic_cast<beagle::product::mixins::Option*>(product.get());
          if (!pO)
            throw(std::string("The incoming product is not an option!"));

          double expiry = pO->expiry();
          double strike = pO->strike();
          const beagle::payoff_ptr_t& payoff = pO->payoff();

          // Set the initial condition and perform the forwardward induction
          beagle::dbl_vec_t stateVars;
          beagle::dbl_vec_t prices;
          formInitialCondition(expiry, payoff, stateVars, prices);

          // Store exponential of state variables for speed
          beagle::dbl_vec_t moneynesses(stateVars);
          std::transform(stateVars.cbegin(),
                         stateVars.cend(),
                         moneynesses.begin(),
                         [=](double logMoneyness)
                         { return std::exp(logMoneyness); });

          // Check dividends and use ex-dividend dates in forward induction
          beagle::dbl_vec_t dates;
          std::vector<beagle::two_dbl_t> dividends;
          std::vector<beagle::two_dbl_t> cumAndExDivForwards;
          beagle::dividend_policy_ptr_t policy;
          auto pDS = dynamic_cast<beagle::math::mixins::ContainsDividends*>(forwardCurve().get());
          if (pDS)
          {
            auto schedule = pDS->dividendSchedule();
            auto it = std::lower_bound(schedule.cbegin(),
                                       schedule.cend(),
                                       expiry,
                                       [](const beagle::dividend_schedule_t::value_type& dividend,
                                          double value)
                                       { return std::get<0>(dividend) < value; });
            auto diff = std::distance(schedule.cbegin(), it);

            std::transform(schedule.cbegin(),
                           it,
                           std::back_inserter(dates),
                           [=](const beagle::dividend_schedule_t::value_type& item)
                           { return std::get<0>(item); });
            std::transform(schedule.cbegin(),
                           it,
                           std::back_inserter(dividends),
                           [=](const beagle::dividend_schedule_t::value_type& item)
                           { return std::make_pair(std::get<1>(item), std::get<2>(item)); });

            cumAndExDivForwards = pDS->cumAndExDividendForwards();
            cumAndExDivForwards.erase(cumAndExDivForwards.begin() + diff,
                                      cumAndExDivForwards.end());

            policy = pDS->dividendPolicy();

            if (cumAndExDivForwards.size() != dividends.size())
              throw(std::string(""));
            if (dates.size() != dividends.size())
              throw(std::string(""));
          }

          dates.emplace_back(expiry);

          // Perform the forward induction
          auto it = dates.cbegin();
          auto itEnd = dates.cend();
          auto jt = dividends.cbegin();
          auto jtEnd = dividends.cend();
          auto kt = cumAndExDivForwards.cbegin();
          double start = 0.;
          for ( ; it!=itEnd; ++it, ++jt, ++kt)
          {
            double end = *it;
            evolve(start, end, payoff, stateVars, prices);

            if (jt != jtEnd)
            {
              double cumForward = kt->first;
              double exForward = kt->second;

              beagle::dbl_vec_t strikes(stateVars);
              std::transform(moneynesses.cbegin(),
                             moneynesses.cend(),
                             strikes.begin(),
                             [=](double moneyness)
                             { return cumForward * moneyness; });

              beagle::real_function_ptr_t results = finiteDifferenceSettings().interpolationMethod()->formFunction( strikes, prices );

              std::transform( moneynesses.begin(),
                              moneynesses.end(),
                              strikes.begin(),
                              [=](double moneyness)
                              { return (exForward * moneyness  + jt->second) / (1. - jt->first); } );
              std::transform( strikes.cbegin(),
                              strikes.cend(),
                              prices.begin(),
                              [=](double strike)
                              { return cumForward *  (1. - jt->first) * results->value(strike) / exForward; } );
            }

            start = end;
          }

          double forward = forwardCurve()->value(expiry);
          beagle::dbl_vec_t strikes(stateVars);
          std::transform(moneynesses.begin(),
                         moneynesses.end(),
                         strikes.begin(),
                         [=](double moneyness)
                         { return forward * moneyness; });
          beagle::real_function_ptr_t result = finiteDifferenceSettings().interpolationMethod()->formFunction(strikes, prices);
          return discountCurve()->value(expiry) * forward * result->value(strike);
        }
        virtual void formInitialCondition(double expiry,
                                          const beagle::payoff_ptr_t& payoff,
                                          beagle::dbl_vec_t& stateVars,
                                          beagle::dbl_vec_t& prices) const override
        {
          double termForward = forwardCurve()->value(expiry);

          // Set up the state variable mesh
          int numStateVars = finiteDifferenceSettings().numberOfStateVariableSteps();
          if (numStateVars % 2 == 0)
            numStateVars += 1;

          double atmVol = volatilitySurface()->value(expiry, termForward);
          int centralIndex = numStateVars / 2;
          double centralValue = 0.;
          double stateVarStep = finiteDifferenceSettings().numberOfGaussianStandardDeviations() * atmVol * std::sqrt(expiry) / centralIndex;

          stateVars.resize(numStateVars);
          for (int i=0; i<numStateVars; ++i)
            stateVars[i] = centralValue + (i - centralIndex) * stateVarStep;

          // Set the initial condition
          prices.resize(numStateVars);
          std::transform(stateVars.cbegin(),
                         stateVars.cend(),
                         prices.begin(),
                         [&payoff](double logMoneyness)
                         {
                           return payoff->intrinsicValue(1., std::exp(logMoneyness));
                         });

          //for (int i=0; i<prices.size(); ++i)
          //  std::cout << prices[i] << "\n";
          //std::cout << "\n\n";
        }
        virtual void evolve(double start,
                            double end,
                            const beagle::payoff_ptr_t& payoff,
                            const beagle::dbl_vec_t& stateVars,
                            beagle::dbl_vec_t& prices) const override
        {
          beagle::parabolic_pde_solver_ptr_t solver
            = beagle::math::OneDimParabolicPDESolver::formDupirePDESolver(
                                   convectionCoefficient(),
                                   diffusionCoefficient());

          // Perform the forward induction
          int numTimes = static_cast<int>((end - start) * finiteDifferenceSettings().numberOfTimeSteps());
          double timeStep = (end - start) / numTimes;
          double lbc = payoff->intrinsicValue( 1.0, std::exp(stateVars.front()) );
          double ubc = payoff->intrinsicValue( 1.0, std::exp(stateVars.back()) );
          for (int i=0; i<numTimes; ++i)
          {
            double thisTime = start + (i+1) * timeStep;
            solver->evolve(thisTime,
                           timeStep,
                           stateVars,
                           beagle::dbl_vec_t{lbc},
                           beagle::dbl_vec_t{ubc},
                           prices);
          }
        }
      };
    }

    beagle::pricer_ptr_t
    Pricer::formOneDimForwardPDEArrowDebreuPricer(const beagle::real_function_ptr_t& forward,
                                                  const beagle::real_function_ptr_t& discounting,
                                                  const beagle::real_2d_function_ptr_t& drift,
                                                  const beagle::real_2d_function_ptr_t& volatility,
                                                  const beagle::real_2d_function_ptr_t& rate,
                                                  const beagle::valuation::OneDimFiniteDifferenceSettings& settings)
    {
      return std::make_shared<impl::OneDimForwardPDEArrowDebreuPricer>( forward,
                                                                        discounting,
                                                                        drift,
                                                                        volatility,
                                                                        rate,
                                                                        settings );
    }

    beagle::pricer_ptr_t
    Pricer::formOneDimForwardPDEEuroOptionPricer(const beagle::real_function_ptr_t& forward,
                                                 const beagle::real_function_ptr_t& discounting,
                                                 const beagle::real_2d_function_ptr_t& volatility,
                                                 const beagle::valuation::OneDimFiniteDifferenceSettings& settings)
    {
      return std::make_shared<impl::OneDimForwardPDEEuroOptionPricer>( forward,
                                                                       discounting,
                                                                       volatility,
                                                                       settings );
    }
  }
}