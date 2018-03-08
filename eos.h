/*
  -------------------------------------------------------------------
  
  Copyright (C) 2018, Xingfu Du and Andrew W. Steiner
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  -------------------------------------------------------------------
*/
#include <fstream>

#ifndef NO_MPI
#include <mpi.h>
#endif

#include <o2scl/test_mgr.h>
#include <o2scl/eos_had_skyrme.h>
#include <o2scl/fermion_nonrel.h>
#include <o2scl/nstar_cold.h>
#include <o2scl/format_float.h>
#include <o2scl/hdf_file.h>
#include <o2scl/hdf_io.h>
#include <o2scl/hdf_eos_io.h>
#include <o2scl/cli.h>
#include <o2scl/lib_settings.h>
#include <o2scl/fit_nonlin.h>
#include <o2scl/eos_crust_virial.h>
#include <o2scl/nucmass_densmat.h>
#include <o2scl/eos_sn.h>
#include <o2scl/cloud_file.h>
#include <o2scl/rng_gsl.h>
#include <gsl/gsl_sf_hyperg.h>
#include <o2scl/root_brent_gsl.h>
#include <o2scl/smooth_func.h>
#include <time.h>

#include "virial_solver.h" 
#include <o2scl/deriv_gsl.h>
#include <o2scl/deriv_cern.h>

/** \brief O'Connor and Ott EOS with a modified electron-photon EOS
*/
class eos_sn_oo1: public o2scl::eos_sn_oo {

public:

  /** \brief Compute the electron-photon EOS at the
      specified density, electron fraction, and temperature
   */
  double compute_eg_point(double nB, double Ye, double T);

};

/** \brief An updated version of \ref o2scl::eos_crust_virial
    with a better fit for the virial coefficients
*/
class eos_crust_virial_v2: public o2scl::eos_crust_virial {

public:

  /** \brief The neutron-neutron virial coefficient given the
      function parameters specified in \c par
   */
  double bn_func(size_t np, const std::vector<double> &par, double T);
  
  /** \brief The neutron-proton virial coefficient given the
      function parameters specified in \c par
   */
  double bpn_func(size_t np, const std::vector<double> &par, double T);

  /** \brief The neutron-neutron virial coefficient
   */
  double bn_f(double T);
  
  /** \brief The neutron-proton virial coefficient
   */
  double bpn_f(double T);
  
  /** \brief The temperature derivative of the
      neutron-neutron virial coefficient
   */
  double dbndT_f(double T);
  
  /** \brief The temperature derivative of the
      neutron-proton virial coefficient
   */
  double dbpndT_f(double T);
  
  /** \brief The current neutron-neutron virial coefficient parameters
   */
  std::vector<double> bn_params;
  
  /** \brief The current neutron-proton virial coefficient parameters
   */
  std::vector<double> bpn_params;

  /** \brief The number of neutron-neutron virial coefficient parameters
   */
  static const size_t bn_np=10;

  /** \brief The number of neutron-proton virial coefficient parameters
   */
  static const size_t bpn_np=6;
  
  /** \brief Perform the fit to the scattering data
   */
  virtual void fit(bool show_fit=false);

  eos_crust_virial_v2();
  
}; 

/** \brief Phenomenological EOS for homeogeneous nucleonic matter

    Andrew's internal todo list
    - Fix numerical derivative section of test_deriv()
 */
class eos {
  
public:

  /** \brief If true, a model has ben selected (default false)
   */
  bool model_selected;
  
  /** \brief If true, test the neutron star speed of sound
   */
  bool test_ns_cs2;
  
  /** \brief Construct a new neutron star EOS which ensures
      causality at high densities
  */
  int new_ns_eos(double nb, o2scl::fermion &n, double &e_ns,
		 double &densdnn);

  /** \brief Electron/positron object
   */
  o2scl::fermion electron;

  /** \brief Photon object
   */
  o2scl::boson photon;

  /** \brief Object for computing electron/positron thermodynamic integrals
   */
  o2scl::fermion_rel relf;

  /// Parameters for the function which fits the neutron star EOS
  std::vector<double> ns_fit_parms;

  /// The value of the virial modulation function
  double g_virial;

  /// The temperature derivative of the virial modulation function
  double dgvirialdT;

  /** \brief If true, test cs2 in the \ref select_internal() function
      (default true)
  */
  bool select_cs2_test;

  /** \brief If true, include muons (default false)
   */
  bool include_muons;
  
  /// The free energy of degenerate matter
  double f_deg;
  
  /// The virial free energy
  double f_virial;
  
  /// The virial entropy
  double s_virial;
  
  /** \brief The fit function for the energy per particle
      in units of MeV as a function of the baryon density
      (in \f$ \mathrm{fm}^{-3} \f$ )

      Note this function does not include the rest mass 
      energy density for the nucleons. 
  */
  double fit_fun(size_t np, const std::vector<double> &parms,
		 double nb);

  /** \brief The energy density (in \f$ \mathrm{fm}^{-4} \f$ )
      as a function of baryon density (in \f$ \mathrm{fm}^{-3} \f$ )

      Note this function does not include the rest mass 
      energy density for the nucleons. 
  */
  double ed_fit(double nb);
  
  /** \brief The inverse susceptibility (in \f$ \mathrm{fm}^{2} \f$ )
      as a function of baryon density (in \f$ \mathrm{fm}^{-3} \f$ )
   */
  double dmudn_fit(double nb);
  
  /** \brief The speed of sound
      as a function of baryon density (in \f$ \mathrm{fm}^{-3} \f$ )
   */
  double cs2_fit(double nb);
  
  /** \brief Compute the minimum and maximum speed of sound
      between 0.08 and \ref ns_nb_max
  */
  void min_max_cs2(double &cs2_min, double &cs2_max);

  /** \brief If true, use the old neutron star fit
   */
  bool old_ns_fit;

  /** \brief If true, save the results of the neutron star fit to
      a file, and immediately exit
   */
  bool ns_record;

  /** \brief The maximum baryon density at which the neutron star
      EOS is causal

      This quantity is determined by \ref ns_fit()
  */
  double ns_nb_max;
  
  /** \brief The baryon number chemical potential (in \f$
      \mathrm{fm}^{-1} \f$ ) as a function of number density (in \f$
      \mathrm{fm}^{-3} \f$ )

      Note this function does not include the rest mass 
      for the nucleons. 
  */
  double mu_fit(double nb);
  
  /** \brief Compute the entropy density including photons and 
      electons
   */
  double entropy(o2scl::fermion &n, o2scl::fermion &p,
		 double nn, double np, double T, o2scl::thermo &th);

  /** \brief Compute energy density including photons and electons
      (without the rest mass energy density for the nucleons)
  */
  double ed(o2scl::fermion &n, o2scl::fermion &p,
		 double nn, double np, double T, o2scl::thermo &th);

  /** \brief Compute dfdnn including photons and electons
  */
  double dfdnn_total(o2scl::fermion &n, o2scl::fermion &p,
		   double nn, double pn, double T, o2scl::thermo &th);

  /** \brief compute dfdpn including photons and electons
  */
  double dfdpn_total(o2scl::fermion &n, o2scl::fermion &p,
		   double nn, double pn, double T, o2scl::thermo &th);

  /** \brief compute depsilondsonb including photons and electons
   */
  double depsilondsonb(double sonb, double nb, double Ye,
		       double cssq);

  /** \brief Compute the squared speed of sound from the 
      analytical expressions for fixed mul
  */
  double cs2(o2scl::fermion &n, o2scl::fermion &p, double T,
	     o2scl::thermo &th);

 /** \brief Compute the squared speed of sound from the 
      analytical expressions for fixed Ye
  */
  double cs2_fixYe(o2scl::fermion &n, o2scl::fermion &p, double T,
		   o2scl::thermo &th);

 /** \brief Compute the smoothed squared speed of sound from the 
      analytical expressions for fixed Ye 
  */
  int cs2_fixYe_mod(size_t nv,const ubvector &x, ubvector &y,
		double Ye);

  /** \brief Solve for Ye to ensure a specified value of muL at fixed T
   */
  int solve_Ye(size_t nv,const ubvector &x, ubvector &y,
		double nb, double T, double muL);

  /** \brief Solve for T to ensure a specified value of sonb at fixed Ye
   */
  int solve_T(size_t nv,const ubvector &x, ubvector &y,
		double nb, double Ye, double sonb);

  /** \brief solve nb_star, given T, Ye, cs2
   */
  int solve_nbstar(size_t nv, const ubvector &x,
		ubvector &y, double T, double Ye, double cs2);


  /** \brief solve T_star nb_star, given sonb, Ye, cs2
   */
  int solve_nbstarTstar(size_t nv, const ubvector &x,
		ubvector &y, double T, double Ye, double cs2);

  /** \brief solve sonb, given nb, Ye, T at high density
   */
  int solve_sonb(size_t nv, const ubvector &x,
		ubvector &y, double nb, double Ye, double T);


  /** \brief solve nbstar1 Tstar1 nbstar2 Tstar2 sonb given nb Ye T
   */
  int solve_fiveparams(size_t nv, const ubvector &x,
		ubvector &y, double nb, double Ye, double T,double eps);

  /** \brief solve nbstar1 Tstar1 nbstar2 Tstar2 given nb Ye sonb
   */
  int solve_fourparams(size_t nv, const ubvector &x,
		ubvector &y, double nb, double Ye, double sonb, double eps);

  /** \brief solve nbstar1 Tstar1 nbstar2 Tstar2 nbstar3 Tstar3 given nb Ye sonb
   */
  int solve_sixparams(size_t nv, const ubvector &x,
		ubvector &y, double nb, double Ye, double sonb, double eps);

  /** \brief solve nbstar1 Tstar1 nbstar2 Tstar2 nbstar3 
      Tstar3 sonb given nb Ye T
   */
  int solve_sevenparams(size_t nv, const ubvector &x,
		ubvector &y, double nb, double Ye, double T, double eps);

  /** \brief Desc
   */
  int solve_nbstar_andrew(size_t nv, const ubvector &x,
			  ubvector &y, double sonb, double Ye, double beta);
  
  /** \brief Desc
   */
  int solve_five_andrew(size_t nv, const ubvector &x,
			ubvector &y, double nb, double Ye, double T,
			double beta);
  
  /** \brief find derivatives dPstardYe, dnbstardYe, dsstardYe, nb_star
             P_star, mul star
   */
  int find_deriv(ubvector &x, double Ye, double T);

  /** \brief Fit neutron star data from Bamr to an analytical 
      expression 
  */
  void ns_fit(int row);

  /// Neutron
  o2scl::fermion neutron;

  /// Proton
  o2scl::fermion proton;

  /// Neutron for chiral part
  o2scl::fermion n_chiral;

  /// Proton for chiral part
  o2scl::fermion p_chiral;

  /// Thermodynamic quantities
  o2scl::thermo th2;
  
  /// Thermodynamic quantities for chiral part
  o2scl::thermo th_chiral;
  
  /// Base EOS model
  o2scl::eos_had_skyrme sk;

  /// Skyrme interaction for finite temperature correction
  o2scl::eos_had_skyrme sk_chiral;

  /// Verbose parameter
  int verbose;

  /// If true, create output files for individual EOSs
  bool output_files;

  /// Prefix for output files
  std::string file_prefix;

  /// The virial EOS
  eos_crust_virial_v2 ecv;

  /// Coefficient for modulation of virial EOS
  double a_virial;

  /// Coefficient for modulation of virial EOS
  double b_virial;
  
  /// \name The parameters for the QMC energy density
  //@{
  /// (unitless)
  double qmc_alpha;
  /// (unitless)
  double qmc_beta;
  /// In MeV
  double qmc_a;
  /// In MeV
  double qmc_b;
  /** \brief The saturation density of the QMC EOS, equal to 
      \f$ 0.16~\mathrm{fm}^{-3} \f$
  */
  double qmc_n0;
  //@}
  
  /** \brief a1 a2 c1 c2 in new ns eos
  */
  double a1,a2,c1,c2;

  double temp_nbstar,temp_Tstar,temp_sonbstar;
  
  /** \brief The speed of sound in neutron star matter at 2.0 fm^{-3}
   */
  double phi;

  /// The symmetry energy (set by \ref random() )
  double eos_S;

  /// The slope of the symmetry energy (set by \ref random() )
  double eos_L;
  
  /// The binding energy per particle
  double eos_EoA;
  
  /// The incompressibility
  double eos_K;
  
  /// The saturation density
  double eos_n0;

  /// The chi-squared for the neutron star fit
  double chi2_ns;

  /// The index of the neutron star model
  int i_ns;

  /// The index of the Skyrme model
  int i_skyrme;

  /// The virial equation solver
  virial_solver acl;

  /// The table which stores the neutron star EOS results
  o2scl::table_units<> nstar_tab;

  /// The table which stores the Skyrme fits
  o2scl::table_units<> UNEDF_tab;
  
  /// Random number generator
  o2scl::rng_gsl r;
  //@}
  
  eos();

  /** \brief Compute the energy density (in \f$ \mathrm{fm}^{-4} \f$)
      of neutron matter from quantum Monte Carlo (without the rest
      mass contribution)
  */
  double energy_density_qmc(double nn, double pn);
  
  /** \brief Compute the energy density (in \f$ \mathrm{fm}^{-4} \f$)
      of neutron matter at high density from the neutron star data
      using the most recent fit (without the rest mass contribution)

      \note Currently this just returns the value of
      \ref ed_fit() .
  */
  double energy_density_ns(double nn);

  /** \brief Alternate form of free_energy_density() for
      computing derivatives

      This function does not include electrons or photons.
  */
  double free_energy_density_alt(o2scl::fermion &n, o2scl::fermion &p,
				 double nn, double np, double T,
				 o2scl::thermo &th);

  /** \brief Alternate form of free_energy_density() for
      computing derivatives

      This function does include electrons, positrons, and photons.
  */
  double free_energy_density_ep(double nn, double np, double T);
  
  /** \brief Return the total free energy density of matter
      (without the rest mass contribution)
   */
  double free_energy_density
    (o2scl::fermion &n, o2scl::fermion &p, double T,
     o2scl::thermo &th);

  /** \brief Compute the free energy density using the virial 
      expansion including derivative information
  */
  double free_energy_density_virial
    (o2scl::fermion &n, o2scl::fermion &p, double T,
     o2scl::thermo &th, double &dmundnn, double &dmundpn,
     double &dmupdnn, double &dmupdpn, double &dmundT,
     double &dmupdT);    

  /** \brief Compute the free energy density using the virial 
      expansion
  */
  double free_energy_density_virial
    (o2scl::fermion &n, o2scl::fermion &p, double T,
     o2scl::thermo &th) {
    double x1, x2, x3, x4, x5, x6;
    return free_energy_density_virial(n,p,T,th,x1,x2,x3,x4,x5,x6);
  }
  
  /** \brief Construct a table at fixed electron fraction
   */
  int table_Ye(std::vector<std::string> &sv,
		  bool itive_com);

  /** \brief Construct a full table 
   */
  int table_full(std::vector<std::string> &sv, bool itive_com);

  /** \brief solve a1 a2, if cs_ns(2.0)>cs_ns(1.28)
  */
  int solve_coeff_big(size_t nv, const ubvector &x, ubvector &y, 
        double nb_last, double cs_ns_2, double cs_ns_last);

  /** \brief solve a1 a2, if cs_ns(2.0)<cs_ns(1.28)
  */
  int solve_coeff_small(size_t nv, const ubvector &x, ubvector &y, 
         double nb_last, double cs_ns_2, double cs_ns_last);

  /** \brief Test the code
   */
  int test_deriv(std::vector<std::string> &sv, bool itive_com);

  /** \brief Desc
   */
  int test_cs2(double nb, double ye, double T);

  /** \brief Select a model by specifying the parameters
   */
  int select_model(std::vector<std::string> &sv, bool itive_com);

  /** \brief Internal select function
   */
  int select_internal(int i_ns_loc, int i_skyrme_loc,
		      double qmc_alpha_loc, double qmc_a_loc,
		      double eos_L_loc, double eos_S_loc,
		      double phi_loc);

  /** \brief Compare the full free energy with the free energy
      from the virial expansion
  */
  int vir_comp(std::vector<std::string> &sv, bool itive_com);

  /** \brief Evaluate the EOS at one point
   */
  int point(std::vector<std::string> &sv, bool itive_com);

  /** \brief Select a random model
   */
  int random(std::vector<std::string> &sv, bool itive_com);

  /** \brief Compute the data for the comparison figures
   */
  int comp_figs(std::vector<std::string> &sv, bool itive_com);

  /** \brief Compute the data for the Monte Carlo figures
   */
  int mcarlo_data(std::vector<std::string> &sv, bool itive_com);

  /** \brief Perform the virial fit
   */
  int vir_fit(std::vector<std::string> &sv, bool itive_com);

  /** \brief Test the electron and photon contribution
   */
  int test_eg(std::vector<std::string> &sv, bool itive_com);

  /** \brief Compute the EOS from previously generated EOS
      tables at several points
   */
  int eos_sn(std::vector<std::string> &sv, bool itive_com);
  
};
