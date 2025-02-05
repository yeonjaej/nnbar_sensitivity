#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include "cubature.h"

// Gaussian distribution function
double gaus(double param, double mean, double sigma)
{
  double gaussian = (1 / (sqrt(2 * M_PI) * sigma)) * exp(-1 * pow(param - mean, 2) / (2 * pow(sigma, 2)));
  return gaussian;
}

// Heaviside distribution function
double heaviside(double param)
{
  if (param > 0)
    return 1;
  else if (param < 0)
    return 0;
  else if (param == 0)
    return 0.5;
  else
  {
    std::cout << "Error processing Heaviside function; lifetime param has no value!" << std::endl;
    exit(1);
  }
}

// Factorial function
double factorial(double n)
{
  double n_factorial = 1;
  for (int i = n; i > 0; i--)
    n_factorial *= i;
  return n_factorial;
}

// 4D integral of probability distribution, including lifetime variable
int f(unsigned ndim, const double * x, void * params, unsigned fdim, double * fval)
{
  // Get input variables
  double width      = x[0];
  double exposure   = x[1];
  double efficiency = x[2];
  double background = x[3];

  std::vector<double> integralParams = *((std::vector<double> *) params);

  // Evaluate probability distributions for variables
  double p_exposure   = gaus(exposure  , integralParams[0], integralParams[1]);
  double p_efficiency = gaus(efficiency, integralParams[2], integralParams[3]);
  double p_background = gaus(background, integralParams[4], integralParams[5]);
  double p_width      = heaviside(width);

  // Get some other useful variables
  double n_factorial = factorial(integralParams[6]);
  double param = (exposure * efficiency * width) + background;

  // Evaluate Bayesian function
  double bayesian = (exp(-1 * param) * pow(param, integralParams[6])) / n_factorial;

  // Put it all together
  double everything = bayesian * p_exposure * p_efficiency * p_background * p_width;
  fval[0] = everything;

  return 0;
}

int main(int argc, char * argv[]) {
  
  // dune mass / density
    double mass    = 40;           // kt
    double density = 3.3164892e32; // neutrons / kt
  
  // adding in super-k parameters for debugging purposes
  //  double mass    = 22.5;          // kt
  //  double density = 2.6742369e+32; // neutrons / kt

  // check command line arguments  
  if (argc < 8 || argc > 9) {
    std::cerr << "Usage: " << argv[0] << " [run_name] [params]" << std::endl;
    exit(1);
  }
 
  // parse the input parameters
  std::string run_name = argv[1];
  std::vector<double> initialParams;
  initialParams.reserve(6);
  for (int i = 2; i < 8; i++)
    initialParams.push_back(atof(argv[i]));
  //initialParams[0] = val_exp
  //[1] = sig_exp
  //[2] = val_eff
  //[3] = sig_eff
  //[4] = val_bkg
  //[5] = sig_bkg

  // Convert input values into their final form
  std::vector<double> finalParams;
  finalParams.reserve(7);
  finalParams.push_back(initialParams[0] * mass * density);
  //finalParams[0] = val_exp * mass * density
  finalParams.push_back(initialParams[1] * finalParams[0]);
  //finalParams[1] = sig_exp * val_exp * mass * density
  finalParams.push_back(initialParams[2]);
  //finalParams[2] = val_eff
  finalParams.push_back(initialParams[3] * finalParams[2]);
  //finalParams[3] = sig_eff * val_eff
  finalParams.push_back(initialParams[4] * mass * initialParams[0]);
  //finalParams[4] = val_bkg * mass * val_exp
  finalParams.push_back(initialParams[5] * finalParams[4]);
  //finalParams[5] = sig_bkg * val_bkg * mass * val_exp

  if (argc == 9)
    finalParams.push_back(atof(argv[8]));
  else
    finalParams.push_back(finalParams[4]);
  //finalParams[6] (this is n_obs = b ) = val_bkg * mass * val_exp


  // figure out limits
  double xmin[4];
  double xmax[4];

  double width_range = 1e-37;
  double integral_range = 5;

  xmin[0] = 0;
  xmin[1] = finalParams[0] - (integral_range * finalParams[1]);
  xmin[2] = finalParams[2] - (integral_range * finalParams[3]);
  xmin[3] = finalParams[4] - (integral_range * finalParams[5]);

  xmax[0] = width_range;
  xmax[1] = finalParams[0] + (integral_range * finalParams[1]);
  xmax[2] = finalParams[2] + (integral_range * finalParams[3]);
  xmax[3] = finalParams[4] + (integral_range * finalParams[5]);

  if (xmin[1] < 0) xmin[1] = 0;
  if (xmin[2] < 0) xmin[2] = 0;
  if (xmin[3] < 0) xmin[3] = 0;

  if (xmax[2] > 1) xmax[2] = 1;

  double val, err;
  hcubature(1, f, &finalParams, 4, xmin, xmax, 0, 0, 1e-5, ERROR_INDIVIDUAL, &val, &err);
  
  double norm = 1 / val;
  double minimum = 0;
  double maximum = 1e-37;
  double accuracy = 1e-5;
  double width;

  while(true) {
    xmax[0] = minimum + ((maximum - minimum) / 2);
    hcubature(1, f, &finalParams, 4, xmin, xmax, 0, 0, 1e-5, ERROR_INDIVIDUAL, &val, &err);
    double diff = (val * norm) - 0.9;
    std::cout << diff << std::endl;
    if (std::abs(diff) < accuracy) {
      width = xmax[0];
      break;
    }
    else if (diff > 0)
      maximum = xmax[0];
    else if (diff < 0)
      minimum = xmax[0];
  }

  double limit = 1 / width;
  double R_factor = 0.666e23;
  double year_to_sec = 3.1536e7;
  double free_limit = sqrt(limit*year_to_sec/R_factor);
  std::cout << run_name.c_str() << " " << limit << " , " << free_limit << std::endl;
  
}

