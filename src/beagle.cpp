#include "gocsei_sahel.hpp"
#include "andersen_buffum.hpp"

int main( void )
{
  // beagle::test::test_local_vol_calibration();
  // beagle::test::test_implied_vol_credit_spread();
  // beagle::test::test_bond_pricer();
  // beagle::test::test_discontinuous_forward_curve();

  // beagle::test::generateAndersenBuffumFigureTwo();
  // beagle::test::generateAndersenBuffumFigureThree();
  // beagle::test::generateAndersenBuffumFigureFour();
  // beagle::test::generateAndersenBuffumFigureFive();
  // beagle::test::generateAndersenBuffumFigureSix();
  // beagle::test::generateAndersenBuffumFigureSeven();
  beagle::test::generateAndersenBuffumTableOneCalibratedPrice();
  beagle::test::generateAndersenBuffumTableOneNaivePrice();

  beagle::test::test_volatility_smile_credit_spread_calibration();

  return 0;
}
