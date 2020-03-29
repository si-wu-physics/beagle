#include "pricer.hpp"
#include "option.hpp"
#include "bond.hpp"
#include "payoff.hpp"
#include "real_function.hpp"
#include "real_2d_function.hpp"
#include "dividend_policy.hpp"
#include "interpolation_builder.hpp"
#include "util.hpp"
#include "round_trip.hpp"

#include <iostream>
#include <fstream>

void test1( void )
{
  std::cout << "\nStart of Test 1:\n\n";

  beagle::discrete_dividend_schedule_t dividends;
  //dividends.emplace_back( 0.5, 3.0 );
  //dividends.emplace_back( 1.5, 3.0 );
  //dividends.emplace_back( 2.5, 3.0 );
  //dividends.emplace_back( 3.5, 3.0 );
  //dividends.emplace_back( 4.5, 3.0 );
  //dividends.emplace_back( 5.5, 8.0 );
  //dividends.emplace_back( 6.5, 8.0 );

  double expiry = 1.;
  double strike = 100.;
  double rate = .01;
  double vol = .3;

  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::put();
  beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                            strike,
                                                                                            payoff );
  beagle::product_ptr_t amerOption = beagle::product::option::Option::createAmericanOption( expiry,
                                                                                            strike,
                                                                                            payoff );

  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-rate * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            100, discounting);

  beagle::valuation::FiniteDifferenceDetails fdDetails( 100., rate, vol, 1501, 1901, 7.5, dividends,
                                                        beagle::valuation::DividendPolicy::liquidator(),
                                                        beagle::math::InterpolationBuilder::linear());

  try
  {
    beagle::pricer_ptr_t bscfeop = beagle::valuation::Pricer::formBlackScholesClosedFormEuropeanOptionPricer(fdDetails.spot(), 
                                                                                                             fdDetails.rate(), 
                                                                                                             fdDetails.volatility(), 
                                                                                                             fdDetails.dividends() );
    beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                               forward,
                                                               discounting,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0.),
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(vol),
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0),
                                                               beagle::valuation::OneDimFiniteDifferenceSettings(1501, 1901, 7.5) );
    beagle::pricer_ptr_t odbpop2  = beagle::valuation::Pricer::formOneDimensionalBackwardPDEOptionPricer(
                                                               fdDetails,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(fdDetails.volatility()) );
    beagle::pricer_ptr_t odfpeop  = beagle::valuation::Pricer::formOneDimensionalForwardPDEEuropeanOptionPricer(
                                                               fdDetails,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(fdDetails.volatility()) );
    beagle::pricer_ptr_t odfpeop2  = beagle::valuation::Pricer::formOneDimForwardPDEEuroOptionPricer(
                                                               forward,
                                                               discounting,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0.),
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(vol),
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0),
                                                               beagle::valuation::OneDimFiniteDifferenceSettings(1501, 1901, 7.5) );

    std::cout << "European option price (CF)   is: " << bscfeop->value( euroOption ) << std::endl;
    std::cout << "European option price (FD-B) is: " << odbpop->value( euroOption ) << std::endl;
    std::cout << "European option price (FD-B) is: " << odbpop2->value( euroOption ) << std::endl;
    std::cout << "American option price (FD-B) is: " << odbpop->value( amerOption ) << std::endl;
    std::cout << "American option price (FD-B) is: " << odbpop2->value( amerOption ) << std::endl;
    std::cout << "European option price (FD-F) is: " << odfpeop->value( euroOption ) << std::endl;
    std::cout << "European option price (FD-F) is: " << odfpeop2->value( euroOption ) << std::endl;

    std::cout << "\nEnd of Test 1\n";
  }
  catch (const std::string& what)
  {
    std::cout << "A valuation error has occurred -- " << what << std::endl;
  }
}

void test2( void )
{
  std::cout << "\nStart of Test 2:\n\n";

  beagle::discrete_dividend_schedule_t dividends;
  // dividends.emplace_back(0.4, 3.0);
  // dividends.emplace_back(0.6, 2.0);
  // dividends.emplace_back(0.8, 1.0);

  beagle::valuation::FiniteDifferenceDetails fdDetails(100., .00, .5, 500, 1500, 5.5, dividends,
                                                       beagle::valuation::DividendPolicy::liquidator(),
                                                       beagle::math::InterpolationBuilder::linear());

  beagle::dbl_vec_t expiries;
  beagle::dbl_vec_vec_t strikesColl;
  beagle::dbl_vec_vec_t pricesColl;
  beagle::test::generateEuropeanMarketQuotes(fdDetails, expiries, strikesColl, pricesColl );

  beagle::dbl_vec_t initialGuesses(expiries.size(), .6);

  try
  {
    beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::call();
    beagle::interp_builder_ptr_t interp = beagle::math::InterpolationBuilder::linear();

    beagle::pricer_ptr_t forwardPricer = beagle::valuation::Pricer::formOneDimensionalForwardPDEEuropeanOptionPricer(
                                                                 fdDetails,
                                                                 beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(fdDetails.volatility()) );

    beagle::real_2d_function_ptr_t localVol =
      beagle::math::RealTwoDimFunction::createBootstrappedLocalVolatilityFunction( expiries,
                                                                                   initialGuesses,
                                                                                   strikesColl,
                                                                                   pricesColl,
                                                                                   forwardPricer,
                                                                                   payoff,
                                                                                   interp );

    // Output local volatility surface
    for (beagle::dbl_vec_t::size_type i = 0; i < expiries.size() - 1; ++i)
    {
      for (beagle::dbl_vec_t::size_type j = 0; j < strikesColl[i].size(); ++j)
        std::cout << expiries[i] - .1 << "\t"
                  << strikesColl[i][j] << "\t"
                  << localVol->value(expiries[i] - .1, strikesColl[i][j]) << "\n";

      std::cout << "\n";
    }

    std::cout << "\n";

    // Output price quotes
    for (beagle::dbl_vec_t::size_type i = 0; i < expiries.size() - 1; ++i)
    {
      for (beagle::dbl_vec_t::size_type j = 0; j < strikesColl[i].size(); ++j)
        std::cout << pricesColl[i][j] << "\t";

      std::cout << "\n";
    }

    std::cout << "\n";

    // Output calibrated prices
    beagle::pricer_ptr_t calibratedPricer = beagle::valuation::Pricer::formOneDimensionalForwardPDEEuropeanOptionPricer( fdDetails, localVol );
    for (beagle::dbl_vec_t::size_type i = 0; i < expiries.size() - 1; ++i)
    {
      for (beagle::dbl_vec_t::size_type j = 0; j < strikesColl[i].size(); ++j)
      {
        beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption(expiries[i],
                                                                                                 strikesColl[i][j],
                                                                                                 payoff);
        std::cout << calibratedPricer->value(euroOption) << "\t";
      }

      std::cout << "\n";
    }

    std::cout << "\n";

    std::cout << "\nEnd of Test 2\n";
  }
  catch (const std::string& what)
  {
    std::cout << what << std::endl;
  }
}

void test3( void )
{
  std::cout << "\nStart of Test 3:\n\n";

  beagle::discrete_dividend_schedule_t dividends;
  // dividends.emplace_back( 0.5, 6.0 );
  // dividends.emplace_back( 1.5, 6.5 );
  // dividends.emplace_back( 2.5, 7.0 );
  // dividends.emplace_back( 3.5, 7.5 );
  // dividends.emplace_back( 4.5, 8.0 );
  // dividends.emplace_back( 5.5, 8.0 );
  // dividends.emplace_back( 6.5, 8.0 );

  double expiry = 1.;
  double strike = 100.;
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::put();
  beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                            strike,
                                                                                            payoff );
  beagle::product_ptr_t amerOption = beagle::product::option::Option::createAmericanOption( expiry,
                                                                                            strike,
                                                                                            payoff );

  double spot = 100.;
  double rate = .01;

  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-rate * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot, discounting);

  // CEV modeling
  double alpha = .3;
  double beta = .5;
  beagle::real_function_ptr_t localVolFunction = beagle::math::RealFunction::createUnaryFunction(
                                                              [alpha, beta](double x) { return alpha * std::pow(x, beta - 1.); } );
  beagle::real_2d_function_ptr_t localVolSurface =
            beagle::math::RealTwoDimFunction::createPiecewiseConstantRightFunction( beagle::dbl_vec_t(1U, 1.),
                                                                                    beagle::real_function_ptr_coll_t(1U, localVolFunction) );

  beagle::valuation::FiniteDifferenceDetails fdDetails(100., .00, .25, 1501, 1901, 7.5, dividends,
                                                       beagle::valuation::DividendPolicy::liquidator(),
                                                       beagle::math::InterpolationBuilder::linear());

  try
  {
    beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimensionalBackwardPDEOptionPricer(
                                                               fdDetails,
                                                               localVolSurface );

    beagle::real_2d_function_ptr_t cev = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                 [=](double time, double price){ return alpha * std::pow(price, beta - 1.); } );
    beagle::pricer_ptr_t odbpop2  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                               forward,
                                                               discounting,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0.),
                                                               cev,
                                                               beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(0.),
                                                               beagle::valuation::OneDimFiniteDifferenceSettings(1500, 1501, 7.5) );
    beagle::pricer_ptr_t odfpeop  = beagle::valuation::Pricer::formOneDimensionalForwardPDEEuropeanOptionPricer(
                                                               fdDetails,
                                                               localVolSurface );

    std::cout << "European option price (FD-B) is: " << odbpop->value( euroOption ) << std::endl;
    std::cout << "European option price (FD-B) is: " << odbpop2->value( euroOption ) << std::endl;
    std::cout << "American option price (FD-B) is: " << odbpop->value( amerOption ) << std::endl;
    std::cout << "American option price (FD-B) is: " << odbpop2->value( amerOption ) << std::endl;
    std::cout << "European option price (FD-F) is: " << odfpeop->value( euroOption ) << std::endl;

    std::cout << "\nEnd of Test 3\n";
  }
  catch (const std::string& what)
  {
    std::cout << "A valuation error has occurred -- " << what << std::endl;
  }
}

void test4(void)
{
  std::cout << "\nStart of Test 4:\n\n";

  beagle::dbl_vec_t diag{ -2.6, -2.6, -2.6, -2.6 };
  beagle::dbl_vec_t upper{ 1., 1., 1., 1. };
  beagle::dbl_vec_t lower{ 1., 1., 1., 1. };
  beagle::dbl_vec_t rhs{ -240., 0., 0., -150. };

  beagle::util::tridiagonalSolve(rhs, diag, upper, lower);
  for (auto result : rhs)
    std::cout << result << "\n";

  std::cout << "\nEnd of Test 4:\n\n";
}

void test5( void )
{
  std::cout << "\nStart of Test 5:\n\n";

  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .4;

  double c = .03;
  double p = 2.;

  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::call();

  // Model parameters
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);
  beagle::real_2d_function_ptr_t rate = drift;

  beagle::dbl_vec_t expiries{5.}; //{.25, .5, 1., 2., 3., 4., 5.};
  for (double expiry : expiries)
  {
    double strike = forward->value(expiry);
    beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                              strike,
                                                                                              payoff );
    beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                             0.,
                                                                                             beagle::product::option::Payoff::digitalCall() );

    beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                               forward,
                                                               discounting,
                                                               drift,
                                                               volatility,
                                                               rate,
                                                               beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );
    double price = odbpop->value( euroOption );
    double bondPrice = odbpop->value(riskyBond);
    std::cout << expiry << "\t"
              << price << "\t"
              << beagle::util::impliedBlackVolatility(price,
                                                      strike,
                                                      forward->value(expiry),
                                                      expiry,
                                                      discounting ) << "\t"
              << bondPrice << "\t"
              << -std::log(bondPrice) / expiry - r << "\n";

    beagle::pricer_ptr_t odfpeop  = beagle::valuation::Pricer::formOneDimForwardPDEEuroOptionPricer(
                                                               forward,
                                                               discounting,
                                                               drift,
                                                               volatility,
                                                               rate,
                                                               beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );
    price = odfpeop->value( euroOption );
    bondPrice = odfpeop->value(riskyBond);
    std::cout << expiry << "\t"
              << price << "\t"
              << beagle::util::impliedBlackVolatility(price,
                                                      strike,
                                                      forward->value(expiry),
                                                      expiry,
                                                      discounting ) << "\t"
              << bondPrice << "\t"
              << -std::log(bondPrice) / expiry - r << "\n\n";
  
  }

  std::cout << "\nEnd of Test 5\n";
}

void test6( void )
{
  std::cout << "\nStart of Test 6:\n\n";

  // Model parameters 1
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .25;

  double c = .02;
  double p = 2.;
  double rec = 0.4;

  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);
  beagle::real_2d_function_ptr_t rate = drift;
  beagle::real_2d_function_ptr_t recovery = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return -100. * rec * rate->value(time, price); } );
  beagle::pricer_ptr_t odbpbp  = beagle::valuation::Pricer::formOneDimBackwardPDEBondPricer(
                                                              forward,
                                                              discounting,
                                                              drift,
                                                              volatility,
                                                              rate,
                                                              recovery,
                                                              beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );

  // Create a fixed coupon bond: 5-year maturity, 1.5% coupon, semi-annual
  beagle::product_ptr_t fcb = beagle::product::bond::Bond::createFixedCouponBond(5., .015, 2);
  std::cout << "The bond price is: " << odbpbp->value(fcb) << "\n";

  // Model parameters 2
  sigma = .4;
  c = .03;
  p = 2.;

  drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);
  rate = drift;
  recovery = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return -100. * rec * rate->value(time, price); } );
  odbpbp  = beagle::valuation::Pricer::formOneDimBackwardPDEBondPricer(
                                                              forward,
                                                              discounting,
                                                              drift,
                                                              volatility,
                                                              rate,
                                                              recovery,
                                                              beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );
  
  // Create a fixed coupon bond: 10-year maturity, 3% coupon, semi-annual
  fcb = beagle::product::bond::Bond::createFixedCouponBond(10, .03, 2);
  std::cout << "The bond price is: " << odbpbp->value(fcb) << "\n";

  // Create a zero coupon bond: 5-year maturity
  beagle::product_ptr_t zcb = beagle::product::bond::Bond::createZeroCouponBond(5.);
  std::cout << "The bond price is: " << odbpbp->value(zcb) << "\n";

  std::cout << "\nEnd of Test 6\n";
}

void test7( void )
{
  std::cout << "\nStart of Test 7:\n\n";

  // Model parameters 1
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .25;

  double c = .02;
  double p = .5;
  double rec = 0.4;

  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);
  beagle::real_2d_function_ptr_t rate = drift;
  beagle::real_2d_function_ptr_t recovery = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return -100. * rec * rate->value(time, price); } );
  beagle::pricer_ptr_t odbpbp  = beagle::valuation::Pricer::formOneDimBackwardPDEBondPricer(
                                                              forward,
                                                              discounting,
                                                              drift,
                                                              volatility,
                                                              rate,
                                                              recovery,
                                                              beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );
try {
  // Create a fixed coupon bond: 5-year maturity, 1.5% coupon, semi-annual
  beagle::callable_schedule_t callSchedule;
  callSchedule.emplace_back(3., beagle::math::RealFunction::createConstantFunction(106.), 5.);

  beagle::puttable_schedule_t putSchedule;
  putSchedule.emplace_back(4., 100.);

  beagle::real_function_ptr_t conversionRatio = beagle::math::RealFunction::createConstantFunction(1.);

  beagle::product_ptr_t fcb = beagle::product::bond::Bond::createFixedCouponBond(5., .015, 2);
  beagle::product_ptr_t cb = beagle::product::bond::Bond::createConvertibleBond(fcb, conversionRatio, callSchedule, putSchedule);

  std::cout << "The convertible bond price is: " << odbpbp->value(cb) << " for p = " << p << "\n";
} catch (const std::string& what) {
  std::cout << what << "\n";
}

  // Model parameters 2
  sigma = .4;
  c = .03;
  p = .5;

  drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);
  rate = drift;
  recovery = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return -100. * rec * rate->value(time, price); } );
  odbpbp  = beagle::valuation::Pricer::formOneDimBackwardPDEBondPricer(
                                                              forward,
                                                              discounting,
                                                              drift,
                                                              volatility,
                                                              rate,
                                                              recovery,
                                                              beagle::valuation::OneDimFiniteDifferenceSettings(100, 500, 4.5) );
  
  // Create a fixed coupon bond: 10-year maturity, 3% coupon, semi-annual
  beagle::callable_schedule_t callSchedule;
  callSchedule.emplace_back(5., beagle::math::RealFunction::createConstantFunction(108.), 10.);

  beagle::puttable_schedule_t putSchedule;
  putSchedule.emplace_back(6., 100.);
  putSchedule.emplace_back(8., 100.);

  beagle::real_function_ptr_t conversionRatio = beagle::math::RealFunction::createConstantFunction(1.);

  beagle::product_ptr_t fcb = beagle::product::bond::Bond::createFixedCouponBond(10, .03, 2);
  beagle::product_ptr_t cb = beagle::product::bond::Bond::createConvertibleBond(fcb, conversionRatio, callSchedule, putSchedule);
  std::cout << "The convertible bond price is: " << odbpbp->value(cb) << " for p = " << p << "\n";

  std::cout << "\nEnd of Test 7\n";
}

void generateAndersenBuffumFigureTwo( void )
{
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .3;

  // Call option valuation with discounting, funding, and volatility
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::call();
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);

  // Generate implied volatilities for a series of expiries
  beagle::dbl_vec_t expiries{.25, .5, 1., 2., 3., 4., 5., 7., 10., 15., 20., 25., 30.};

  // Panel A
  double p = 2.;
  beagle::dbl_vec_t cs{0., .03, .05, .1};

  std::ofstream outA(".\\figure\\fig_2_panel_A.txt");
  outA << "[";
  for (double expiry : expiries)
    outA << expiry << ", ";
  outA << "]\n";

  for (double c : cs)
  {
    outA << "[";
    for (double expiry : expiries)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      double strike = forward->value(expiry);
      beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                                strike,
                                                                                                payoff );
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               beagle::product::option::Payoff::digitalCall() );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outA << beagle::util::impliedBlackVolatility(odbpop->value(euroOption),
                                                   strike,
                                                   strike,
                                                   expiry,
                                                   discounting ) << ", ";
    }

    outA << "]\n";
  }

  // Panel B
  double c = .05;
  beagle::dbl_vec_t ps{0., .5, 2.};

  std::ofstream outB(".\\figure\\fig_2_panel_B.txt");
  outB << "[";
  for (double expiry : expiries)
    outB << expiry << ", ";
  outB << "]\n";

  for (double p : ps)
  {
    outB << "[";
    for (double expiry : expiries)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      double strike = forward->value(expiry);
      beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                                strike,
                                                                                                payoff );
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               beagle::product::option::Payoff::digitalCall() );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outB << beagle::util::impliedBlackVolatility(odbpop->value(euroOption),
                                                   strike,
                                                   strike,
                                                   expiry,
                                                   discounting ) << ", ";
    }

    outB << "]\n";
  }
}

void generateAndersenBuffumFigureThree( void )
{
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .3;
  double p = 2.;

  // Call option valuation with discounting, funding, and volatility
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::call();
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);

  // Generate implied volatility smile
  beagle::dbl_vec_t cs{.02, .05, .1};
  beagle::dbl_vec_t moneynesses{.6, .7, .8, .9, 1., 1.1, 1.2, 1.3, 1.4, 1.5};

  // Panel A
  double expiry = .5;

  std::ofstream outA(".\\figure\\fig_3_panel_A.txt");
  outA << "[";
  for (double moneyness : moneynesses)
    outA << moneyness << ", ";
  outA << "]\n";

  for (double c : cs)
  {
    outA << "[";
    for (double moneyness : moneynesses)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      double strike = forward->value(expiry) * moneyness;
      beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                                strike,
                                                                                                payoff );
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               beagle::product::option::Payoff::digitalCall() );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outA << beagle::util::impliedBlackVolatility(odbpop->value(euroOption),
                                                   strike,
                                                   forward->value(expiry),
                                                   expiry,
                                                   discounting ) << ", ";
    }

    outA << "]\n";
  }

  // Panel B
  expiry = 5.;

  std::ofstream outB(".\\figure\\fig_3_panel_B.txt");
  outB << "[";
  for (double moneyness : moneynesses)
    outB << moneyness << ", ";
  outB << "]\n";

  for (double c : cs)
  {
    outB << "[";
    for (double moneyness : moneynesses)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      double strike = forward->value(expiry) * moneyness;
      beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                                strike,
                                                                                                payoff );
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               beagle::product::option::Payoff::digitalCall() );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outB << beagle::util::impliedBlackVolatility(odbpop->value(euroOption),
                                                   strike,
                                                   forward->value(expiry),
                                                   expiry,
                                                   discounting ) << ", ";
    }

    outB << "]\n";
  }
}

void generateAndersenBuffumFigureFour( void )
{
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .3;
  double expiry = 1;
  double c = .05;

  // Call option valuation with discounting, funding, and volatility
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::call();
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);

  // Generate implied volatility smile
  beagle::dbl_vec_t ps{0., 2.};
  beagle::dbl_vec_t moneynesses{.6, .7, .8, .9, 1., 1.1, 1.2, 1.3, 1.4, 1.5};

  // Panel A

  std::ofstream outA(".\\figure\\fig_4.txt");
  outA << "[";
  for (double moneyness : moneynesses)
    outA << moneyness << ", ";
  outA << "]\n";

  for (double p : ps)
  {
    outA << "[";
    for (double moneyness : moneynesses)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      double strike = forward->value(expiry) * moneyness;
      beagle::product_ptr_t euroOption = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                                strike,
                                                                                                payoff );
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               beagle::product::option::Payoff::digitalCall() );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outA << beagle::util::impliedBlackVolatility(odbpop->value(euroOption),
                                                   strike,
                                                   forward->value(expiry),
                                                   expiry,
                                                   discounting ) << ", ";
    }

    outA << "]\n";
  }
}

void generateAndersenBuffumFigureFive( void )
{
  double spot = 50;
  double r = .04;
  double q = .02;
  double sigma = .3;

  // Call option valuation with discounting, funding, and volatility
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::digitalCall();
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);

  // Generate implied volatilities for a series of expiries
  beagle::dbl_vec_t expiries{.25, .5, 1., 2., 3., 4., 5., 7., 10., 15., 20., 25., 30.};

  // Panel A
  double p = 2.;
  beagle::dbl_vec_t cs{.02, .05, .1};

  std::ofstream outA(".\\figure\\fig_5_panel_A.txt");
  outA << "[";
  for (double expiry : expiries)
    outA << expiry << ", ";
  outA << "]\n";

  for (double c : cs)
  {
    outA << "[";
    for (double expiry : expiries)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               payoff );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outA << -std::log(odbpop->value(riskyBond)) / expiry - r << ", ";
    }

    outA << "]\n";
  }

  // Panel B
  double c = .05;
  beagle::dbl_vec_t ps{0., 2., 3.};

  std::ofstream outB(".\\figure\\fig_5_panel_B.txt");
  outB << "[";
  for (double expiry : expiries)
    outB << expiry << ", ";
  outB << "]\n";

  for (double p : ps)
  {
    outB << "[";
    for (double expiry : expiries)
    {
      beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                                [=](double time, double price){ return c * std::pow(price / spot, -p); } );
      beagle::real_2d_function_ptr_t rate = drift;

      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               payoff );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outB << -std::log(odbpop->value(riskyBond)) / expiry - r << ", ";
    }

    outB << "]\n";
  }
}

void generateAndersenBuffumFigureSix( void )
{
  double spot = 50;
  double r = .04;
  double q = .02;
  double p = 2.;
  double c = .1;

  // Call option valuation with discounting, funding, and volatility
  beagle::payoff_ptr_t payoff = beagle::product::option::Payoff::digitalCall();
  beagle::real_function_ptr_t discounting = beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-r * arg);});
  beagle::real_function_ptr_t forward = beagle::math::RealFunction::createContinuousForwardAssetPriceFunction(
                                            spot,
                                            beagle::math::RealFunction::createUnaryFunction(
                                            [=](double arg) { return std::exp(-(r - q) * arg);}));
  beagle::real_2d_function_ptr_t drift = beagle::math::RealTwoDimFunction::createBinaryFunction(
                                            [=](double time, double price){ return c * std::pow(price / spot, -p); } );
  beagle::real_2d_function_ptr_t rate = drift;

  // Generate implied volatilities for a series of expiries
  beagle::dbl_vec_t expiries{.25, .5, 1., 2., 3., 4., 5., 7., 10., 15., 20., 25., 30.};

  // Panel A
  beagle::dbl_vec_t sigmas{.15, .3, .5};

  std::ofstream outA(".\\figure\\fig_6.txt");
  outA << "[";
  for (double expiry : expiries)
    outA << expiry << ", ";
  outA << "]\n";

  for (double sigma : sigmas)
  {
    beagle::real_2d_function_ptr_t volatility = beagle::math::RealTwoDimFunction::createTwoDimConstantFunction(sigma);

    outA << "[";
    for (double expiry : expiries)
    {
      beagle::product_ptr_t riskyBond = beagle::product::option::Option::createEuropeanOption( expiry,
                                                                                               0.,
                                                                                               payoff );

      beagle::pricer_ptr_t odbpop  = beagle::valuation::Pricer::formOneDimBackwardPDEOptionPricer(
                                                                 forward,
                                                                 discounting,
                                                                 drift,
                                                                 volatility,
                                                                 rate,
                                                                 beagle::valuation::OneDimFiniteDifferenceSettings(365, 750, 4.5) );

      outA << -std::log(odbpop->value(riskyBond)) / expiry - r << ", ";
    }

    outA << "]\n";
  }
}

int main( void )
{
  //test1();
  //test2();
  //test3();
  //test4();
  //test5();
  //test6();
  //test7();
  generateAndersenBuffumFigureTwo();
  generateAndersenBuffumFigureThree();
  generateAndersenBuffumFigureFour();
  generateAndersenBuffumFigureFive();
  generateAndersenBuffumFigureSix();

  return 0;
}
