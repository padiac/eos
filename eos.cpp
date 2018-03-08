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
#include "eos.h"

using namespace std;
using namespace o2scl;
using namespace o2scl_const;
using namespace o2scl_hdf;

double eos_sn_oo1::compute_eg_point(double nb1, double ye1, double T1) {

  double deg=nb1/pow(T1/hc_mev_fm,3.0);
  photon.massless_calc(T1/hc_mev_fm);
  electron.n=nb1*ye1;
  
  electron.mu=electron.m;
  if (ye1==0.0 || electron.n==0.0) {
    electron.ed=0.0;
    electron.mu=0.0;
    electron.en=0.0;
    electron.pr=0.0;
  } else {
    relf.pair_density(electron,T1/hc_mev_fm);
  }
  
  if (include_muons) {
    muon.mu=electron.mu;
    if (ye1==0.0 || muon.mu==0.0) { 
      muon.ed=0.0;
      muon.n=0.0;
      muon.en=0.0;
      muon.pr=0.0;
    } else {
      relf.pair_mu(muon,T1/hc_mev_fm);
    }
  }
  
  double E_eg=(electron.ed+photon.ed)/nb1*hc_mev_fm;
  double P_eg=(electron.pr+photon.pr)*hc_mev_fm;
  double S_eg=(electron.en+photon.en)/nb1;
  double F_eg=E_eg-T1*S_eg;
  
  if (include_muons) {
    E_eg+=muon.ed/nb1*hc_mev_fm;
    P_eg+=muon.pr*hc_mev_fm;
    S_eg+=muon.en/nb1;
    F_eg+=muon.ed/nb1*hc_mev_fm-T1*muon.en/nb1;
  }

  if (false) {
    double F_eg_electron=electron.ed/nb1*hc_mev_fm-T1*electron.en/nb1;
    double F_eg_photon=photon.ed/nb1*hc_mev_fm-T1*photon.en/nb1;
    cout << "F_eg: " << F_eg << " " << F_eg_electron << " "
	 << F_eg_photon << endl;
  }
  
  return F_eg;
}

eos_crust_virial_v2::eos_crust_virial_v2() {
  bn_params.resize(10);
  bn_params[0]=2.874487202922e-01;
  bn_params[1]=2.200575070883e-03;
  bn_params[2]=-2.621025627694e-05;
  bn_params[3]=-6.061665959200e-08;
  bn_params[4]=1.059451872186e-02;
  bn_params[5]=5.673374476876e-02;
  bn_params[6]=3.492489364849e+00;
  bn_params[7]=-2.710552654167e-03;
  bn_params[8]=3.140521199464e+00;
  bn_params[9]=1.200987113605e+00;
  bpn_params.resize(6);
  bpn_params[0]=1.527316309589e+00;
  bpn_params[1]=1.748834077357e-04;
  bpn_params[2]=1.754991542102e+01;
  bpn_params[3]=4.510380054238e-01;
  bpn_params[4]=2.751333759925e-01;
  bpn_params[5]=-1.125035495140e+00;

}

double eos_crust_virial_v2::bn_func(size_t np, const vector<double> &par,
				    double T) {
  return par[0]+par[1]*T+par[2]*T*T+par[3]*T*T*T+par[4]*
    exp(-par[5]*pow(T-par[6],2.0))+par[7]*exp(-par[8]*(T-par[9]));
}

double eos_crust_virial_v2::bpn_func(size_t np, const vector<double> &par,
				     double T) {
  return par[0]*exp(-par[1]*(T+par[2])*(T+par[2]))+
    par[3]*exp(-par[4]*(T+par[5]));
}

double eos_crust_virial_v2::bn_f(double T) {
  return bn_func(bn_params.size(),bn_params,T);
}

double eos_crust_virial_v2::bpn_f(double T) {
  return bpn_func(bpn_params.size(),bpn_params,T);
}

double eos_crust_virial_v2::dbndT_f(double T) {
  return bn_params[1]+2.0*bn_params[2]*T+3.0*bn_params[3]*T*T-
    2.0*bn_params[4]*bn_params[5]*(T-bn_params[6])*
    exp(-bn_params[5]*pow(T-bn_params[6],2.0))-
    bn_params[7]*bn_params[8]*exp(-bn_params[8]*(T-bn_params[9]));
}

double eos_crust_virial_v2::dbpndT_f(double T) {
  return -bpn_params[0]*bpn_params[1]*2.0*(bpn_params[2]+T)*
    exp(-bpn_params[1]*(T+bpn_params[2])*(T+bpn_params[2]))-
    bpn_params[3]*bpn_params[4]*exp(-bpn_params[4]*(T+bpn_params[5]));
}

void eos_crust_virial_v2::fit(bool show_fit) {

  // Chi squared
  double chi2;

  // Fitter class
  fit_nonlin<chi_fit_funct<vector<double>,ubmatrix,std::function<
    double(size_t,const std::vector<double> &, double)> >,
	     vector<double>,ubmatrix> fitter;

  // --------------------------------------------
  // Fit neutron virial coefficient

  // Use the data from Horowitz and Schwenk (2006), PLB
  
  static const size_t neut_data=26;
  std::vector<double> Tv_neut(neut_data);
  Tv_neut[0]=0.1;
  Tv_neut[1]=0.5;
  Tv_neut[2]=1.0;
  Tv_neut[3]=2.0;
  Tv_neut[4]=3.0;
  Tv_neut[5]=4.0;
  Tv_neut[6]=5.0;
  Tv_neut[7]=6.0;
  Tv_neut[8]=7.0;
  Tv_neut[9]=8.0;
  Tv_neut[10]=9.0;
  Tv_neut[11]=10.0;
  Tv_neut[12]=12.0;
  Tv_neut[13]=14.0;
  Tv_neut[14]=16.0;
  Tv_neut[15]=18.0;
  Tv_neut[16]=20.0;
  Tv_neut[17]=22.0;
  Tv_neut[18]=24.0;
  Tv_neut[19]=25.0;
  Tv_neut[20]=30.0;
  Tv_neut[21]=35.0;
  Tv_neut[22]=40.0;
  Tv_neut[23]=45.0;
  Tv_neut[24]=50.0;
  Tv_neut[25]=150.0;

  bnv.resize(neut_data);
  bnv[0]=0.207;
  bnv[1]=0.272;
  bnv[2]=0.288;
  bnv[3]=0.303;
  bnv[4]=0.306;
  bnv[5]=0.306;
  bnv[6]=0.306;
  bnv[7]=0.306;
  bnv[8]=0.307;
  bnv[9]=0.307;
  bnv[10]=0.308;
  bnv[11]=0.309;
  bnv[12]=0.310;
  bnv[13]=0.313;
  bnv[14]=0.315;
  bnv[15]=0.318;
  bnv[16]=0.320;
  bnv[17]=0.322;
  bnv[18]=0.324;
  bnv[19]=0.325;
  bnv[20]=0.329;
  bnv[21]=0.330;
  bnv[22]=0.330;
  bnv[23]=0.328;
  bnv[24]=0.324;
  bnv[25]=-pow(2.0,-5.0/2.0);
  
  vector<double> bn_err(neut_data);
  for(size_t i=0;i<neut_data;i++) {
    bn_err[i]=fabs(bnv[i])/1.0e2;
  }

  // Fitting function
  std::function<
    double(size_t,const std::vector<double> &, double)> ff_neutron=
    std::bind(std::mem_fn<double(size_t,const vector<double> &,double)>
	      (&eos_crust_virial_v2::bn_func),
	      this,std::placeholders::_1,std::placeholders::_2,
	      std::placeholders::_3);
  
  // Chi-squared and fitting data
  chi_fit_funct<vector<double>,ubmatrix,std::function<
    double(size_t,const std::vector<double> &, double)> > 
    cff(neut_data,Tv_neut,bnv,bn_err,ff_neutron);
  
  cout << "Neutron virial coefficient:\n" << endl;
  
  ubmatrix covar(bn_np,bn_np);

  cout << "Initial chi-squared: " << cff.chi2(bn_np,bn_params) << endl;
  
  fitter.fit(bn_np,bn_params,covar,chi2,cff);

  if (show_fit) {
    cout << "Final chi-squared: " << chi2 << endl;
    cout << "params: " << endl;
    cout.precision(12);
    for(size_t j=0;j<bn_np;j++) cout << "bn_params[" << j << "]="
				     << bn_params[j] << ";" << endl;
    cout.precision(6);
    cout << endl;
    
    table<> t;
    t.line_of_names("T bn bn_err bn_fit");
    for(size_t j=0;j<neut_data;j++) {
      cout << Tv_neut[j] << " " << bnv[j] << " " << bn_err[j] << " "
	   << ff_neutron(bn_np,bn_params,Tv_neut[j]) << endl;
      double line[4]={Tv_neut[j],bnv[j],bn_err[j],
		      ff_neutron(bn_np,bn_params,Tv_neut[j])};
      t.line_of_data(4,line);
    }
    cout << endl;

    hdf_file hf;
    hf.open_or_create("fit_neut.o2");
    hdf_output(hf,t,"fit_neut");
    hf.close();
  }
  
  // --------------------------------------------
  // Fit neutron-proton virial coefficient
  
  cout << "Neutron-proton virial coefficient:\n" << endl;

  static const size_t nuc_data=17;

  std::vector<double> Tv_nuc(nuc_data);
  Tv_nuc[0]=0.1;
  Tv_nuc[1]=1.0;
  Tv_nuc[2]=2.0;
  Tv_nuc[3]=3.0;
  Tv_nuc[4]=4.0;
  Tv_nuc[5]=5.0;
  Tv_nuc[6]=6.0;
  Tv_nuc[7]=7.0;
  Tv_nuc[8]=8.0;
  Tv_nuc[9]=9.0;
  Tv_nuc[10]=10.0;
  Tv_nuc[11]=12.0;
  Tv_nuc[12]=14.0;
  Tv_nuc[13]=16.0;
  Tv_nuc[14]=18.0;
  Tv_nuc[15]=20.0;
  Tv_nuc[16]=150.0;
  
  ubmatrix covar2(bpn_np,bpn_np);

  // T=0.1 MeV point from effective range expansion
  bpnv[0]=2.046;

  // Data from Horowitz paper
  bpnv[1]=19.4;
  bpnv[2]=6.1;
  bpnv[3]=4.018;
  bpnv[4]=3.19;
  bpnv[5]=2.74;
  bpnv[6]=2.46;
  bpnv[7]=2.26;
  bpnv[8]=2.11;
  bpnv[9]=2.00;
  bpnv[10]=1.91;
  bpnv[11]=1.76;
  bpnv[12]=1.66;
  bpnv[13]=1.57;
  bpnv[14]=1.51;
  bpnv[15]=1.45;

  // T=150 MeV point setting virial coefficient to zero
  bpnv[16]=0.0;

  // Subtract off deuteron contribution
  for(size_t i=1;i<16;i++) {
    bpnv[i]-=3.0/sqrt(2.0)*(exp(2.224/Tv_nuc[i])-1.0);
  }

  // Determine uncertainties
  std::vector<double> bpn_err(nuc_data);
  for(size_t i=0;i<16;i++) {
    bpn_err[i]=fabs(bpnv[i])/1.0e2;
  }
  bpn_err[16]=0.04;

  // Fitting function
  std::function<
    double(size_t,const std::vector<double> &, double)> ff_nuc=
    std::bind(std::mem_fn<double(size_t,const vector<double> &,double)>
	      (&eos_crust_virial_v2::bpn_func),
	      this,std::placeholders::_1,std::placeholders::_2,
	      std::placeholders::_3);
  
  // Chi-squared and fitting data
  chi_fit_funct<vector<double>,ubmatrix,std::function<
    double(size_t,const std::vector<double> &, double)> > 
    cff_nuc(nuc_data,Tv_nuc,bpnv,bpn_err,ff_nuc);

  cout << "Initial chi-squared: " << cff_nuc.chi2(bpn_np,bpn_params) << endl;

  fitter.fit(bpn_np,bpn_params,covar2,chi2,cff_nuc);

  if(show_fit) {
    cout << "Final chi-squared: " << chi2 << endl;
    cout << "params: " << endl;
    cout.precision(12);
    for(size_t j=0;j<bpn_np;j++) cout << "bpn_params[" << j << "]="
				      << bpn_params[j] << ";" << endl;
    cout.precision(6);
    cout << endl;
    
    table<> t;
    t.line_of_names("T bpn bpn_err bpn_fit");
    for(size_t j=0;j<nuc_data;j++) {
      cout << Tv_nuc[j] << " " << bpnv[j] << " " << bpn_err[j] << " "
	   << ff_nuc(bpn_np,bpn_params,Tv_nuc[j]) << endl;
      double line[4]={Tv_nuc[j],bpnv[j],bpn_err[j],
		      ff_nuc(bpn_np,bpn_params,Tv_nuc[j])};
      t.line_of_data(4,line);
    }

    hdf_file hf;
    hf.open_or_create("fit_nuc.o2");
    hdf_output(hf,t,"fit_nuc");
    hf.close();
  }
  
  // End of eos_crust_virial_v2::fit()
  return;
}

double eos::fit_fun(size_t np, const std::vector<double> &parms,
			     double nb) {
  if (old_ns_fit) {
    return (sqrt(nb)*parms[0]+nb*parms[1]+
	    nb*sqrt(nb)*parms[2]+nb*nb*parms[3]+
	    nb*nb*nb*parms[4]);
  }
  return (nb*parms[0]+nb*nb*parms[1]+nb*nb*nb*parms[2]+
	  nb*nb*nb*nb*parms[3]+nb*nb*nb*nb*nb*parms[4]);
}

double eos::ed_fit(double nb) {
  return fit_fun(5,ns_fit_parms,nb)*nb/hc_mev_fm;
}

double eos::mu_fit(double nb) {
  if (old_ns_fit) {
    return (1.5*sqrt(nb)*ns_fit_parms[0]+2.0*nb*ns_fit_parms[1]+
	    2.5*nb*sqrt(nb)*ns_fit_parms[2]+3.0*nb*nb*ns_fit_parms[3]+
	    4.0*nb*nb*nb*ns_fit_parms[4])/hc_mev_fm;
  }
  return (2.0*nb*ns_fit_parms[0]+3.0*nb*nb*ns_fit_parms[1]+
	  4.0*nb*nb*nb*ns_fit_parms[2]+
	  5.0*nb*nb*nb*nb*ns_fit_parms[3]+
	  6.0*nb*nb*nb*nb*nb*ns_fit_parms[4])
    /hc_mev_fm;
}

double eos::dmudn_fit(double nb) {
  if (old_ns_fit) {
    return (0.75/sqrt(nb)*ns_fit_parms[0]+2.0*ns_fit_parms[1]+
	    3.75*sqrt(nb)*ns_fit_parms[2]+6.0*nb*ns_fit_parms[3]+
	    12.0*nb*nb*ns_fit_parms[4])/hc_mev_fm;
  }
  return (2.0*ns_fit_parms[0]+6.0*nb*ns_fit_parms[1]+
	  12.0*nb*nb*ns_fit_parms[2]+20.0*nb*nb*nb*
	  ns_fit_parms[3]+30.0*nb*nb*nb*nb*ns_fit_parms[4])/hc_mev_fm;
}

double eos::cs2_fit(double nb) {
  return dmudn_fit(nb)*nb/(mu_fit(nb)+939.565/hc_mev_fm);
}

void eos::min_max_cs2(double &cs2_min, double &cs2_max) {
  double nb=0.08;
  double cs2=cs2_fit(nb);
  cs2_min=cs2;
  cs2_max=cs2;
  for(nb=0.04;nb<ns_nb_max;nb+=0.02) {
    cs2=cs2_fit(nb);
    if (cs2<cs2_min) cs2_min=cs2;
    if (cs2>cs2_max) cs2_max=cs2;
  }
  return;
}

void eos::ns_fit(int row) {

  if (row>=nstar_tab.get_nlines()) {
    O2SCL_ERR("Row not allowed in ns_fit().",
	      exc_efailed);
  }

  // Set the class data member to the appropriate row
  i_ns=row;
  
  // Initial values of the parameters
  ns_fit_parms.resize(5);
  ns_fit_parms[0]=-6.102748e3;
  ns_fit_parms[1]=3.053497e3;
  ns_fit_parms[2]=4.662834e3;
  ns_fit_parms[3]=-8.371958e2;
  ns_fit_parms[4]=-5.228209e2;

  // This table stores the neutron star EOS in a form that
  // can be used by the fitting object 
  table_units<> nstar_high;  
  nstar_high.line_of_names("nb EoA");
  ns_nb_max=nstar_tab.get((string)"nb_max",i_ns);
  for(size_t i=0;i<100 && (0.04+((double)i)*0.012)<(ns_nb_max+0.000001);i++) {
    double EoA=nstar_tab.get(((string)"EoA_")+szttos(i),i_ns)*hc_mev_fm;
    if (fabs(nstar_tab.get(((string)"EoA_")+szttos(i),i_ns))>1.0e-2) {
      double line[2]={0.04+((double)i)*0.012,
		      nstar_tab.get(((string)"EoA_")+szttos(i),i_ns)*
		      hc_mev_fm};
      nstar_high.line_of_data(2,line);
    }
  }
  
  // Fit the energy per baryon 
  nstar_high.function_column("abs(EoA)/100","Eerr");
  typedef std::function<
    double(size_t,const std::vector<double> &, double)> fit_funct2;
  fit_funct2 ff=std::bind
    (std::mem_fn<double(size_t,const std::vector<double> &,double)>
     (&eos::fit_fun),this,std::placeholders::_1,
     std::placeholders::_2,std::placeholders::_3);

  // Collect vector references to specify the data as
  // a fit function
  size_t ndat=nstar_high.get_nlines();

  if (false) {
    //plot nstarhigh 
    hdf_file nshf;
    nshf.open_or_create("nbcs2.o2");
    hdf_output(nshf,nstar_high,"nbcs2");
    nshf.close();
    //int sret=system("o2graph -read nbcs2.o2 -plot nb EoA -show");
  }

  const std::vector<double> &xdat=nstar_high["nb"];
  const std::vector<double> &ydat=nstar_high["EoA"];
  const std::vector<double> &yerr=nstar_high["Eerr"];

  // Set up the fitting function (just a simple chi^2 fit)
  size_t nparms=5;
  chi_fit_funct<std::vector<double>,ubmatrix,fit_funct2> 
    cff(ndat,xdat,ydat,yerr,ff);
  
  // The fit covariance matrix
  ubmatrix covar(nparms,nparms);

  // The fit chi-squared
  if (verbose>0) {
    cout << "ns_fit_parms[0]=" << ns_fit_parms[0] << ";" << endl;
    cout << "ns_fit_parms[1]=" << ns_fit_parms[1] << ";" << endl;
    cout << "ns_fit_parms[2]=" << ns_fit_parms[2] << ";" << endl;
    cout << "ns_fit_parms[3]=" << ns_fit_parms[3] << ";" << endl;
    cout << "ns_fit_parms[4]=" << ns_fit_parms[4] << ";" << endl;
  }

  // Perform the fit
  fit_nonlin<chi_fit_funct<std::vector<double>,ubmatrix,fit_funct2>,
	     std::vector<double>,ubmatrix> fn;
  fn.fit(nparms,ns_fit_parms,covar,chi2_ns,cff);
  
  // Store the results of the fit in column named "EoA_fit", then compute
  // energy density and chemical potential//
  nstar_high.new_column("EoA_fit");
  nstar_high.new_column("ed_fit");
  nstar_high.new_column("mu_fit");
  nstar_high.new_column("cs2_fit");
  for(size_t i=0;i<ndat;i++) {
    double nb=nstar_high.get("nb",i);
    double EoA=ff(nparms,ns_fit_parms,nb);
    nstar_high.set("EoA_fit",i,EoA);
    nstar_high.set("ed_fit",i,(EoA+939.0)/hc_mev_fm*nstar_high.get("nb",i));
    double mu=mu_fit(nb)+939.0/hc_mev_fm;
    nstar_high.set("mu_fit",i,mu);
    double cs2=cs2_fit(nb);
    nstar_high.set("cs2_fit",i,cs2);
  }
  nstar_high.function_column("(EoA+939)/197.33*nb","ed");
  nstar_high.deriv("nb","ed","mu");
  nstar_high.deriv2("nb","ed","dmudn");
  nstar_high.function_column("nb/mu*dmudn","cs2");

  // Readjust ns_nb_max to ensure it's lower than the point
  // at which c_s^2 becomes superluminal
  double nb_new=0.0;
  for(size_t j=0;j<nstar_high.get_nlines()-1;j++) {
    if (nstar_high.get("cs2_fit",j)<1.0 &&
	nstar_high.get("cs2_fit",j+1)>1.0) {
      nb_new=nstar_high.get("nb",j)+
	(nstar_high.get("nb",j+1)-nstar_high.get("nb",j))*
	(1.0-nstar_high.get("cs2_fit",j))/
	(nstar_high.get("cs2_fit",j+1)-nstar_high.get("cs2_fit",j));
    }
  }
  if (nb_new>0.01) ns_nb_max=nb_new;
  
  // Output the fit results to the screen
  if (verbose>0) {
    cout << "Parameters: " << endl;
    cout << "ns_fit_parms[0]=" << ns_fit_parms[0] << ";" << endl;
    cout << "ns_fit_parms[1]=" << ns_fit_parms[1] << ";" << endl;
    cout << "ns_fit_parms[2]=" << ns_fit_parms[2] << ";" << endl;
    cout << "ns_fit_parms[3]=" << ns_fit_parms[3] << ";" << endl;
    cout << "ns_fit_parms[4]=" << ns_fit_parms[4] << ";" << endl;
    cout << "chi2: " << chi2_ns << endl;
  }

  // If true, record the results of the fit
  if (ns_record) {
    table_units<> tab2=nstar_high;
    // If ns_record is true, this file stores the original neutron star
    // EOS and the results of the fit
    o2scl_hdf::hdf_file hf;
    hf.open_or_create("ns_fit.o2");
    hdf_output(hf,tab2,"ns_fit");
    hf.close();
    exit(-1);
  }
  
  return;
}

eos::eos() {

  test_ns_cs2=false;
  include_muons=false;
  
  // Ensure that this works without GNU units
  o2scl_settings.get_convert_units().use_gnu_units=false;

  neutron.init(o2scl_settings.get_convert_units().convert
	       ("kg","1/fm",o2scl_mks::mass_neutron),2.0);
  proton.init(o2scl_settings.get_convert_units().convert
	      ("kg","1/fm",o2scl_mks::mass_proton),2.0);
  neutron.non_interacting=false;
  proton.non_interacting=false;
  neutron.inc_rest_mass=false;
  proton.inc_rest_mass=false; 

  electron.init(o2scl_settings.get_convert_units().convert
		("kg","1/fm",o2scl_mks::mass_electron),2.0);

  n_chiral.init(o2scl_settings.get_convert_units().convert
		("kg","1/fm",o2scl_mks::mass_neutron),2.0);
  p_chiral.init(o2scl_settings.get_convert_units().convert
		("kg","1/fm",o2scl_mks::mass_proton),2.0);
  n_chiral.non_interacting=false;
  p_chiral.non_interacting=false;
  n_chiral.inc_rest_mass=false;
  p_chiral.inc_rest_mass=false;
  
  verbose=0;

  output_files=true; 
  file_prefix="skyrme_";

  i_ns=-1;
  i_skyrme=-1;
  qmc_alpha=0.48;
  qmc_beta=3.45;
  qmc_a=12.7;
  qmc_b=2.12;
  qmc_n0=0.16;

  string name;

  // Open the neutron star data file
  std::string ns_file="qmc_twop_10_0_out";
  o2scl_hdf::hdf_file hf;
  hf.open(ns_file);
  o2scl_hdf::hdf_input(hf,nstar_tab,name);
  hf.close();

  // Skyrme data file
  std::string UNEDF_file="thetaANL-1002x12.o2";
  hf.open(UNEDF_file);
  o2scl_hdf::hdf_input(hf,UNEDF_tab,name);
  hf.close();

  r.clock_seed();

  old_ns_fit=true;
  ns_record=false;

  sk_chiral.t0=5.067286719233e+03;
  sk_chiral.t1=1.749251370992e+00;
  sk_chiral.t2=-4.721193938990e-01;
  sk_chiral.t3=-1.945964529505e+05;
  sk_chiral.x0=4.197555064408e+01;
  sk_chiral.x1=-6.947915483747e-02;
  sk_chiral.x2=4.192016722695e-01;
  sk_chiral.x3=-2.877974634128e+01;

  if (false) {
    sk_chiral.t0=4.963340606238e+03;
    sk_chiral.t1=1.743504113657e+00;
    sk_chiral.t2=-4.753127626586e-01;
    sk_chiral.t3=-1.111074832977e+05;
    sk_chiral.x0=3.532141718900e+01;
    sk_chiral.x1=-6.276020465331e-02;
    sk_chiral.x2=4.224427559993e-01;
    sk_chiral.x3=-3.025925006559e+01;    
  }
  
  sk_chiral.alpha=0.144165;

  model_selected=false;
  select_cs2_test=true;

  a_virial=3.0;
  b_virial=0.0;
}

double eos::energy_density_qmc
(double nn, double np) {

  double e_qmc=(qmc_a*pow((nn+np)/qmc_n0,qmc_alpha)+
		qmc_b*pow((nn+np)/qmc_n0,qmc_beta))*
    (nn+np)/hc_mev_fm;
  
  // This is the energy density in fm^{-4}
  return e_qmc;
}
  
double eos::energy_density_ns(double nn) {
  return ed_fit(nn);
}    

double eos::free_energy_density_virial
(fermion &n, fermion &p, double T, thermo &th, double &dmundnn,
 double &dmundpn, double &dmupdnn, double &dmupdpn, double &dmundT,
 double &dmupdT) {
 
  double nn=n.n;
  double pn=p.n;
    
  double T_MeV=T*hc_mev_fm;
  
  double b_n=ecv.bn_f(T_MeV);
  double dbndT=ecv.dbndT_f(T_MeV)*hc_mev_fm;
  double b_pn=ecv.bpn_f(T_MeV);
  double dbpndT=ecv.dbpndT_f(T_MeV)*hc_mev_fm;

  double lambda=sqrt(4.0*o2scl_const::pi/(n.m+p.m)/T);
  double dlambdadT=-sqrt(o2scl_const::pi/(n.m+p.m))/pow(sqrt(T),3.0);

  acl.nn=nn;
  acl.pn=pn;
  acl.T=T;
  acl.b_n=b_n;
  acl.b_pn=b_pn; 
  acl.lambda=lambda;

  // If the densities are large enough, then compute the
  // virial result
  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5) {
    ubvector x(2);
    acl.solve_fugacity(x);
    n.mu=x[0];
    p.mu=x[1];
  } else {
    // Otherwise, the virial correction is negligable, so
    // just use the classical result
    n.mu=log(nn*pow(lambda,3.0)/2.0)*T;
    p.mu=log(pn*pow(lambda,3.0)/2.0)*T;
    acl.zn=exp(n.mu/T);
    acl.zp=exp(p.mu/T);
  }
  double zn=exp(n.mu/T), zp=exp(p.mu/T);
  
  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5) {
  
    th.pr=2.0*T/pow(lambda,3.0)*(zn+zp+(zn*zn+zp*zp)*b_n+
				 2.0*zp*zn*b_pn);
  } else {

    th.pr=2.0*T/pow(lambda,3.0)*(zn+zp);

  }

  double f_vir=n.mu*nn+p.mu*pn-th.pr;
  
  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5) {
    
    th.en=5.0*th.pr/2.0/T-nn*log(zn)-pn*log(zp)+
      2.0*T/pow(lambda,3.0)*((zn*zn+zp*zp)*dbndT
			     +2.0*zp*zn*dbpndT);
  } else {

    th.en=5.0*th.pr/2.0/T-nn*log(zn)-pn*log(zp);

  }

  th.ed=f_vir+T*th.en;

  // Use linear solver to obtain derivative of mu_n_vir and
  // mu_p_vir with respect to neutron number density

  acl.mfn2_mu_p=p.mu;
  acl.mfn2_mu_n=n.mu;

  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5){

    ubvector x2(2);
    acl.mfn21(x2);
    dmundnn=x2[0];
    dmupdnn=x2[1];
    
  } else {
  
    dmundnn=T*pow(lambda,3.0)/2.0/zn;
    dmupdnn=0;
  }
	
  // Use linear solver to obtain derivative of mu_n_virial and
  // mu_p_virial with respect to proton number density

  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5){

    ubvector x3(2);
    acl.mfn31(x3);
    dmundpn=x3[0]; 
    dmupdpn=x3[1];

  } else {
  
    dmupdpn=T*pow(lambda,3.0)/2.0/zp;
    dmundpn=0;
  }
    	  
  // Use linear solver to obtain derivative of mu_n_virial and
  // mu_p_virial with respect to temperature

  acl.dbndT=dbndT;
  acl.dbpndT=dbpndT;
  acl.dlambdadT=dlambdadT;

  if (nn*pow(lambda,3.0)>1.0e-5 || pn*pow(lambda,3.0)>1.0e-5) {
    ubvector x4(2);
    acl.mfn41(x4);
    dmundT=x4[0];
    dmupdT=x4[1];
  } else {
    dmundT=n.mu/T;
    dmupdT=p.mu/T;
  }
  if (verbose>=2) {
    cout << "bn= " << b_n << endl;
    cout << "bpn= " << b_pn << endl;
  } 

  return f_vir;
}

int eos::solve_coeff_big(size_t nv, const ubvector &x, ubvector &y, 
                                  double ns_nb_max_l, double cs_ns_2,
                                  double cs_ns_last) {
  double a1l=x[0];
  double a2l=x[1];
  y[0]=1.0-a1l+(a1l*a2l*pow(ns_nb_max_l,a1l))/
    (1.0+a2l*pow(ns_nb_max_l,a1l))-cs_ns_last;
  y[1]=1.0-a1l+(a1l*a2l*pow(2.0,a1l))/(1.0+a2l*pow(2.0,a1l))-cs_ns_2;
  
  return 0;
}

int eos::solve_coeff_small(size_t nv, const ubvector &x,
                                    ubvector &y, double ns_nb_max_l,
                                    double cs_ns_2, double cs_ns_last) {
                                    
  double a1l=x[0];
  double a2l=x[1];
  y[0]=(a1l-a1l*a2l*pow(ns_nb_max_l,a1l)/
        (1.0+a2l*pow(ns_nb_max_l,a1l)))-cs_ns_last;
  y[1]=(a1l-a1l*a2l*pow(2.0,a1l)/(1.0+a2l*pow(2.0,a1l)))-cs_ns_2;
  
  return 0;
}

int eos::new_ns_eos(double nb, fermion &n,
			     double &e_ns, double &densdnn) {
  
  double cs_ns_last;
  double cs_ns_2;
  double a1l, a2l;
  double c1l, c2l;

  double e_ns_last=ed_fit(ns_nb_max);
  double p_ns_last=mu_fit(ns_nb_max)*ns_nb_max-e_ns_last;

  //solve a1l,a2l 
  mroot_hybrids<> mh;

  // Initial guess for a1l and a2l
  ubvector mx(2), my(2);
  
  if (nb<(ns_nb_max-1.0e-6)) {
    
    // If we're in the region that the neutron star
    // EOS is causal, just use that result
    e_ns=energy_density_ns(nb);
    
    densdnn=mu_fit(nb);
    
  } else {

    // If the speed of sound is increasing
    // at high densities
    
    cs_ns_last=cs2_fit(ns_nb_max);
    cs_ns_2=phi;
    
    if (cs_ns_2>cs_ns_last) {
      mx[0]=1.0;
      mx[1]=1.0;
      mm_funct mfbig=std::bind
	(std::mem_fn<int(size_t,const ubvector &,
			 ubvector &, double, double, double)>
	 (&eos::solve_coeff_big),
	 this,std::placeholders::_1,
	 std::placeholders::_2,
	 std::placeholders::_3,ns_nb_max,cs_ns_2,cs_ns_last);
      mh.msolve(2,mx,mfbig);
      a1l=mx[0];
      a2l=mx[1];
      
      // solve for c1, c2
      c1l=(e_ns_last+n.m*ns_nb_max+p_ns_last)/((ns_nb_max*ns_nb_max)
					       *(a2l+pow(ns_nb_max,-a1l)));
      c2l=0.5*(e_ns_last+n.m*ns_nb_max-p_ns_last+
	       a1l*(e_ns_last+n.m*ns_nb_max+p_ns_last)
	       /((a1l-2.0)*(1.0+a2l*pow(ns_nb_max,a1l))));
      e_ns=-n.m*nb+(a2l*nb*nb/2.0+pow(nb,2.0-a1l)/(2.0-a1l))*c1l+c2l;
      densdnn=-n.m+c1l*(a2l*nb+pow(nb,1.0-a1l));

    } else if (cs_ns_2<cs_ns_last) {

      // If the speed of sound is decreasing
      // at high densities

      mx[0]=2.5;
      mx[1]=1.0;
      mm_funct mfsmall=std::bind
	(std::mem_fn<int(size_t,const ubvector &,
			 ubvector &, double, double ,double)>
	 (&eos::solve_coeff_small),
	 this,std::placeholders::_1,
	 std::placeholders::_2,
	 std::placeholders::_3,ns_nb_max,cs_ns_2,cs_ns_last);
      mh.msolve(2,mx,mfsmall);
      a1l=mx[0];
      a2l=mx[1];
      
      double hyperg=gsl_sf_hyperg_2F1
	(1.0,1.0,1.0-1.0/a1l,1.0/a2l*pow(nb,-a1l)/(1.0/a2l*pow(nb,-a1l)+1.0));
      double hyperg_max=gsl_sf_hyperg_2F1
	(1.0,1.0,1.0-1.0/a1l,1.0/a2l*pow(ns_nb_max,-a1l)/
	 (1.0/a2l*pow(ns_nb_max,-a1l)+1.0));
      
      // Transform hyperg to hyperg_new see notes based on Pfaff
      // transformation
      double hyperg_new=hyperg*(1.0/(1.0+1.0/a2l*pow(nb,-a1l)));
      double hyperg_max_new=hyperg_max*(1.0/(1.0+1.0/a2l*pow(ns_nb_max,-a1l)));

      // solve for c1l, c2l
      c1l=pow(ns_nb_max,-a1l-1.0)*(a2l*pow(ns_nb_max,a1l)+1.0)*
	        (e_ns_last+n.m*ns_nb_max+p_ns_last);
      c2l=pow(ns_nb_max,-a1l)*
	        (a2l*pow(ns_nb_max,a1l)*(e_ns_last+n.m*ns_nb_max)-
         	(a2l*pow(ns_nb_max,a1l)+1.0)
	        *hyperg_max_new*(e_ns_last+n.m*ns_nb_max+p_ns_last))/a2l;
      e_ns=(c1l*nb*hyperg_new)/a2l+c2l-n.m*nb;
      densdnn=-(a2l*n.m*pow(nb,a1l)-c1l*pow(nb,a1l)+n.m)/(a2l*pow(nb,a1l)+1.0);

    } else if (cs_ns_2==cs_ns_last) {

      // If the speed of sound is independent of density at
      // high densities
      
      e_ns=-n.m*nb+(e_ns_last+n.m*ns_nb_max+p_ns_last)
	          /(1.0+cs_ns_last)*pow((nb/ns_nb_max),cs_ns_last+1.0)
	          +(cs_ns_last*(e_ns_last+n.m*ns_nb_max)-p_ns_last)
	          /(1.0+cs_ns_last);
      densdnn=-n.m+(e_ns_last+n.m*ns_nb_max+p_ns_last)
	             *pow((nb/ns_nb_max),cs_ns_last)/ns_nb_max;
    }
  }

  return 0;
}

double eos::free_energy_density
(fermion &n, fermion &p, double T, thermo &th) {

  if (model_selected==false) {
    O2SCL_ERR("No model selected in free_energy_density().",
	      o2scl::exc_einval);
  }

  double nn=n.n;
  double pn=p.n;
  double nb=nn+pn;
  double ye=pn/nb;

  double n0=0.16;

  // ----------------------------------------------------------------
  // Compute the virial EOS
  
  double dmundT, dmupdT, dmundnn, dmupdnn, dmundpn, dmupdpn;
  f_virial=free_energy_density_virial
    (n,p,T,th,dmundnn,dmundpn,dmupdnn,dmupdpn,dmundT,dmupdT);
  s_virial=th.en;

  double dfvirialdT=-th.en;
  double P_virial=th.pr;
  double mu_n_virial=n.mu;
  double mu_p_virial=p.mu;
  double zn=exp(mu_n_virial/T);
  double zp=exp(mu_p_virial/T);
  g_virial=1.0/(a_virial*zn*zn+a_virial*zp*zp+b_virial*zn*zp+1.0);

  // ----------------------------------------------------------------
  // Compute the Skyrme EOS in nuclear matter at T=0

  n.n=(nn+pn)/2.0;
  p.n=(nn+pn)/2.0;
  
  sk.calc_e(n,p,th);
  
  double mu_n_skyrme_eqdenT0=n.mu;
  double mu_p_skyrme_eqdenT0=p.mu;
  double P_skyrme_eqdenT0=-th.ed+mu_n_skyrme_eqdenT0*n.n+
    mu_p_skyrme_eqdenT0*p.n;
  double f_skyrme_eqdenT0=th.ed;

  // ----------------------------------------------------------------
  // Next, compute the Skyrme EOS at the specified density, proton
  // fraction, and temperature

  n_chiral.n=(nn+pn)/2.0;
  p_chiral.n=(nn+pn)/2.0;

  sk_chiral.calc_temp_e(n_chiral,p_chiral,T,th_chiral);

  double f_skyrme_eqden_T=th_chiral.ed-T*th_chiral.en; 
  double mu_p_eqden_T=p_chiral.mu;
  double mu_n_eqden_T=n_chiral.mu;
  double s_eqden_T=th_chiral.en;

  sk_chiral.calc_e(n_chiral,p_chiral,th_chiral);

  double f_skyrme_eqden_T0=th_chiral.ed;
  double mu_p_eqden_T0=p_chiral.mu;
  double mu_n_eqden_T0=n_chiral.mu;
  
  n_chiral.n=nn+pn;
  p_chiral.n=0.0;

  sk_chiral.calc_temp_e(n_chiral,p_chiral,T,th_chiral);

  double f_skyrme_neut_T=th_chiral.ed-T*th_chiral.en; 
  double mu_p_neut_T=p_chiral.mu;
  double mu_n_neut_T=n_chiral.mu;
  double s_neut_T=th_chiral.en;
  
  sk_chiral.calc_e(n_chiral,p_chiral,th_chiral);

  double f_skyrme_neut_T0=th_chiral.ed;
  double mu_p_neut_T0=p_chiral.mu;
  double mu_n_neut_T0=n_chiral.mu;

  // ----------------------------------------------------------------
  // QMC EOS
  
  double e_qmc=energy_density_qmc(nn,pn);   

  // ----------------------------------------------------------------
  // Neutron star EOS
  
  double e_ns;
  double densdnn;

  new_ns_eos(nb,n,e_ns,densdnn);

  if (test_ns_cs2) {
    cout << "ns_nb_max: " << ns_nb_max << endl;
    cout << "phi: " << phi << endl;
    table_units<> tx;
    tx.line_of_names("nb ed mu");
    for(nb=0.08;nb<2.0;nb+=0.01) {
      new_ns_eos(nb,n,e_ns,densdnn);
      double line[3]={nb,e_ns,densdnn};
      tx.line_of_data(3,line);
    }

    tx.function_column("ed+939.0/197.33*nb","edf");
    tx.function_column("mu+939.0/197.33","muf");
    tx.deriv("nb","ed","mu2");
    tx.deriv("nb","edf","muf2");
    tx.deriv("nb","muf2","dmufdn");
    tx.function_column("nb*dmufdn/muf","cs2");

    hdf_file hf;
    hf.open_or_create("ns2test.o2");
    hdf_output(hf,tx,"ns2test");
    hf.close();

    int sret=system("o2graph -read ns2test.o2 -plot nb cs2 -show");
  }
  
  // ----------------------------------------------------------------
  // Combine all the results to get the full free energy density
  // and put it in f_total

  double T_MeV=T*hc_mev_fm;
  double gamma=20.0;
  double h=1.0/(1.0+exp(gamma*(nn+pn-n0*1.5)));
  double e_combine=e_qmc*h+e_ns*(1.0-h);
  double e_sym=e_combine-f_skyrme_eqdenT0;
  double dyednn=-pn/nb/nb;
  double dyedpn=nn/nb/nb;
  double delta2=(1.0-2.0*ye)*(1.0-2.0*ye);
  double ddelta2dnn=2.0*(1.0-2.0*ye)*(-2.0*dyednn);
  double ddelta2dpn=2.0*(1.0-2.0*ye)*(-2.0*dyedpn);
  f_deg=f_skyrme_eqdenT0+delta2*e_sym+
    delta2*(f_skyrme_neut_T-f_skyrme_neut_T0)+
    (1.0-delta2)*(f_skyrme_eqden_T-f_skyrme_eqden_T0);
  double f_total=f_virial*g_virial+f_deg*(1.0-g_virial);
    
  // -------------------------------------------------------------
  // Compute derivatives for chemical potentials
 

  double dgvirialdnn=-(1.0/pow(a_virial*zn*zn+a_virial*zp*zp
    +b_virial*zn*zp+1.0,2.0))*(2.0*a_virial*zn*zn/T*dmundnn
    +2.0*a_virial*zp*zp/T*dmupdnn+b_virial*zn*zp/T*dmundnn
    +b_virial*zn*zp/T*dmupdnn);
  double dgvirialdpn=-(1.0/pow(a_virial*zn*zn+a_virial*zp*zp
    +b_virial*zn*zp+1.0,2.0))*(2.0*a_virial*zn*zn/T*dmundpn
    +2.0*a_virial*zp*zp/T*dmupdpn+b_virial*zn*zp/T*dmundpn
    +b_virial*zn*zp/T*dmupdpn);
      
  double dfvirialdnn=mu_n_virial;
  double dfvirialdpn=mu_p_virial;
  double dfskyrme_eqden_T0dnn=(mu_n_skyrme_eqdenT0+mu_p_skyrme_eqdenT0)/2.0;
  double dfskyrme_eqden_T0dpn=dfskyrme_eqden_T0dnn;
	
  double dhdnn=-gamma*exp(gamma*(nn+pn-1.5*n0))/
    (1.0+exp(gamma*(nn+pn-1.5*n0)))/
    (1.0+exp(gamma*(nn+pn-1.5*n0)));
  
  double desymdnn=((qmc_a*pow((nn+pn)/qmc_n0,qmc_alpha)*(qmc_alpha+1.0)+
		qmc_b*pow((nn+pn)/qmc_n0,qmc_beta)*(qmc_beta+1.0))/
		hc_mev_fm)*h+e_qmc*dhdnn+
    densdnn*(1.0-h)-e_ns*dhdnn-dfskyrme_eqden_T0dpn/2.0-
    dfskyrme_eqden_T0dnn/2.0;
  double desymdpn=desymdnn;
  
  double dfdegdnn=dfskyrme_eqden_T0dnn+(1.0-2.0*ye)
    *(1.0-2.0*ye)*desymdnn+ddelta2dnn*e_sym
    +delta2*(mu_n_neut_T-mu_n_neut_T0)
    +ddelta2dnn*(f_skyrme_neut_T-f_skyrme_neut_T0)
    +(1.0-delta2)*(mu_n_eqden_T/2.0+mu_p_eqden_T/2.0-
		mu_n_eqden_T0/2.0-mu_p_eqden_T0/2.0)-
    ddelta2dnn*(f_skyrme_eqden_T-f_skyrme_eqden_T0);
  double dfdegdpn=dfskyrme_eqden_T0dpn+
    (1.0-2.0*ye)*(1.0-2.0*ye)*desymdpn+
    ddelta2dpn*e_sym
    +delta2*(mu_n_neut_T-mu_n_neut_T0)
    +ddelta2dpn*(f_skyrme_neut_T-f_skyrme_neut_T0)
    +(1.0-delta2)*(mu_p_eqden_T/2.0+mu_n_eqden_T/2.0-
		mu_p_eqden_T0/2.0-mu_n_eqden_T0/2.0)-
    ddelta2dpn*(f_skyrme_eqden_T-f_skyrme_eqden_T0);

  n.mu=dfvirialdnn*g_virial+f_virial*dgvirialdnn+dfdegdnn*(1-g_virial)
         +f_deg*(-dgvirialdnn);
  p.mu=dfvirialdpn*g_virial+f_virial*dgvirialdpn+dfdegdpn*(1-g_virial)
         +f_deg*(-dgvirialdpn);


  // -------------------------------------------------------------
  // Compute derivatives for entropy

  dgvirialdT=-(1.0/pow(1.0+a_virial*zn*zn+a_virial*zp*zp+b_virial*zn*zp,2.0))*
    (2.0*a_virial*zn*zn*dmundT/T-2.0*a_virial*zn*zn*mu_n_virial/T/T
     +2.0*a_virial*zp*zp*dmupdT/T-2.0*a_virial*zp*zp*mu_p_virial/T/T
     +b_virial*zn*zp*dmundT/T+b_virial*zn*zp*dmupdT/T-
     b_virial*zn*zp*mu_n_virial/T/T-b_virial*zn*zp*mu_p_virial/T/T);
  
  // Restore p.n and n.n 
  n.n=nn;
  p.n=pn;
 
  double dfdegdT=delta2*(-s_neut_T)+(1.0-delta2)*(-s_eqden_T);
  
  th.en=-(dfvirialdT*g_virial+f_virial*dgvirialdT+dfdegdT*(1-g_virial)
          +f_deg*(-dgvirialdT));
  th.pr=-f_total+n.n*n.mu+p.n*p.mu;
  th.ed=f_total+T*th.en;
  if (verbose>=1) {
    cout << "i_ns=" << i_ns << endl;
    cout << "i_skyrme=" << i_skyrme << endl;
    cout << "g_virial= " << g_virial << " (g=1 means full virial EOS) dgdT= "
	 << dgvirialdT << endl;
    cout << "h= " << h << " (h=1 means full QMC, h=0 means full NS)" << endl;
    cout << "f_virial= " << f_virial << " 1/fm^4" << endl;
    cout << "F_virial " << f_virial/nb*hc_mev_fm << " MeV" << endl;
    cout << "f_skyrme_eqdenT0= " << f_skyrme_eqdenT0 << " 1/fm^4" << endl;
    cout << "F_skyrme_eqdenT0= " << f_skyrme_eqdenT0/nb*hc_mev_fm
	       << " MeV" << endl;
    cout << "e_qmc= " << e_qmc << " 1/fm^4" << endl;
    cout << "E_qmc= " << e_qmc/nb*hc_mev_fm << " MeV" << endl;
    cout << "e_ns= " << e_ns << " " << e_ns/nb*hc_mev_fm << endl;
    cout << "f_deg= " << f_deg << " " << f_deg/nb*hc_mev_fm << endl;
    cout << "f_total= " << f_total << " " << f_total/nb*hc_mev_fm << endl;
    cout << "zn= " << zn << endl;
    cout << "zp= " << zp << endl;
    cout << "temp= " << s_neut_T << " " << s_eqden_T << endl;
    cout << "entropy= " << th.en << endl;
    cout << "s_virial= " << s_virial << endl;
    cout << "dg_virial_dT= " << dgvirialdT << endl;
    cout << -dfvirialdT*g_virial-f_virial*dgvirialdT << " "
	 << -dfdegdT*(1-g_virial)+f_deg*(dgvirialdT) << endl;
    cout << -dfdegdT << " " << (1-g_virial) << endl;
    cout << f_skyrme_eqdenT0+delta2*e_sym << " "
	 << delta2 << " " << (f_skyrme_neut_T-f_skyrme_neut_T0) << " "
	 << (1.0-delta2) << " "
	 << (f_skyrme_eqden_T-f_skyrme_eqden_T0) << endl;
    cout << endl;
  }

  return f_total;
}
  
double eos::free_energy_density_alt
(fermion &n, fermion &p, double nn, double np, double T, thermo &th) {
  n.n=nn;
  p.n=np;
  return free_energy_density(n,p,T,th);
}

double eos::free_energy_density_ep
(double nn, double np, double T) {
  neutron.n=nn;
  proton.n=np;
  electron.n=np;
  electron.mu=electron.m;
  relf.pair_density(electron,T);
  photon.massless_calc(T);
  double frnp=free_energy_density(neutron,proton,T,th2);
  th2.ed+=electron.ed+photon.ed;
  th2.pr+=electron.pr+photon.pr;
  th2.en+=electron.en+photon.en;
  return frnp+electron.ed-electron.en*T+photon.ed-T*photon.en;
}

double eos::entropy(fermion &n, fermion &p, double nn,
			     double pn, double T, thermo &th) {

  n.n=nn;
  p.n=pn;
  free_energy_density(n,p,T,th);
  electron.n=pn;
  electron.mu=electron.m;
  relf.pair_density(electron,T);
  photon.massless_calc(T);
  return th.en+electron.en+photon.en;
}

double eos::ed(fermion &n, fermion &p, double nn,
			double pn, double T, thermo &th) {
  n.n=nn;
  p.n=pn;
  free_energy_density(n,p,T,th);
  electron.n=pn;
  electron.mu=electron.m;
  relf.pair_density(electron,T);
  photon.massless_calc(T);
  return th.ed+electron.ed+photon.ed+n.m*nn+p.m*pn;
}

double eos::dfdnn_total(fermion &n, fermion &p, double nn, 
				 double pn, double T, thermo &th) {
  
  n.n=nn;
  p.n=pn;
  free_energy_density(n,p,T,th);
  electron.n=pn;
  electron.mu=electron.m;
  relf.pair_density(electron,T);
  return n.mu+n.m;
}

double eos::dfdpn_total(fermion &n, fermion &p, double nn, 
				 double pn, double T, thermo &th) {

  n.n=nn;
  p.n=pn;
  free_energy_density(n,p,T,th);
  electron.n=pn;
  electron.mu=electron.m;
  relf.pair_density(electron,T);
  return p.mu+electron.mu+p.m;
}

double eos::cs2_fixYe(fermion &n, fermion &p, double T, thermo &th) {
 
  double nn,pn;
  deriv_gsl<> gd;
  nn=n.n;
  pn=p.n;
  free_energy_density(n,p,T,th);
  thermo th1=th;
  fermion n1=n,p1=p;
  //double scale=1.0e03;
  // Numerically compute required second derivatives
  
  // d^2f/dnn^2
  std::function<double(double)> dfdnn_totaldnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,
	      pn,T,std::ref(th));
  //gd.h=nn/scale;
  double dfdnn_totaldnn=gd.deriv(nn,dfdnn_totaldnnfunc);
  
  // d^2f/dnn/dnp
  std::function<double(double)> dfdnn_totaldpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  //gd.h=pn/scale;
  double dfdnn_totaldpn=gd.deriv(pn,dfdnn_totaldpnfunc);

  // d^2f/dnn/dT
  std::function<double(double)> dfdnn_totaldTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  //gd.h=T/scale;
  double dfdnn_totaldT=gd.deriv(T,dfdnn_totaldTfunc);

  // d^2f/dnp/dnn
  std::function<double(double)> dfdpn_totaldnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,pn,T,
	      std::ref(th));
  //gd.h=nn/scale;
  double dfdpn_totaldnn=gd.deriv(nn,dfdpn_totaldnnfunc);

  // d^2f/dnp^2
  std::function<double(double)> dfdpn_totaldpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  //gd.h=pn/scale;
  double dfdpn_totaldpn=gd.deriv(pn,dfdpn_totaldpnfunc);

  // d^2f/dnp/dT
  std::function<double(double)> dfdpn_totaldTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  //gd.h=T/scale;
  double dfdpn_totaldT=gd.deriv(T,dfdpn_totaldTfunc);

  // d^2f/dT^2
  std::function<double(double)> dsdTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  //gd.h=T/scale;
  double dsdT=gd.deriv(T,dsdTfunc);

  // d^2f/dT/dnn
  std::function<double(double)> dsdnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,pn,T,
	      std::ref(th));
  //gd.h=nn/scale;
  double dsdnn=gd.deriv(nn,dsdnnfunc);
  
  // d^2f/dT/dnp
  std::function<double(double)> dsdpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  //gd.h=pn/scale;
  double dsdpn=gd.deriv(pn,dsdpnfunc);

  double dfdnbdT, dfdnedT, dfdnbdne, dfdTdT, dfdnbdnb, dfdnedne;
  dfdnbdT=dfdnn_totaldT;
  dfdnedT=dfdpn_totaldT-dfdnn_totaldT;
  dfdnbdne=dfdnn_totaldpn-dfdnn_totaldnn;
  dfdTdT=-dsdT;
  dfdnbdnb=dfdnn_totaldnn;
  dfdnedne=dfdpn_totaldpn+dfdnn_totaldnn-dfdpn_totaldnn-dfdnn_totaldpn;

  double dfdpn_total1=dfdpn_total(n,p,nn,pn,T,th);
  double dfdnn_total1=dfdnn_total(n,p,nn,pn,T,th);
  double mul,mub,s,ed1,nb;
  nb=n1.n+p1.n;
  mub=dfdnn_total1;
  mul=dfdpn_total1-dfdnn_total1;
  s=entropy(n1,p1,n1.n,p1.n,T,th1);
  ed1=ed(n1,p1,n1.n,p1.n,T,th1);

  double ne=p1.n;

  // Compute dPdNb
  double dPdnb=dfdnbdnb*nb+dfdnbdne*ne;

  // Compute dPdNe
  double dPdne=dfdnbdne*nb+dfdnedne*ne;

  // Compute dPdT
  double dPdT=dfdnbdT*nb+dfdnedT*ne+s;

  // Compute dPdVstYeNb*V

  //double dPdVstYeNb=(-nb*dPdnb*dfdTdT-ne*dPdne*dfdTdT+dPdT*(dfdnbdT*nb+dfdnedT*ne+s))/(dfdTdT);
  
  // Compute depsilondVstYeNb*V

  //double depsilondVstYeNb=-((-mub*nb-mul*ne-n.m*nb+dfdnbdT*T*nb+dfdnedT*T*ne)*(-dfdTdT)-(-T*dfdTdT)*(dfdnbdT*nb+dfdnedT*ne+s))/dfdTdT;
 
  //or double depsilondVstYeNb=-(mub*nb+mul*ne+T*s+n.m*nb);

  double pr=mul*ne+mub*nb+T*s-ed1;
  
  double cs_sq=(-nb*dPdnb*dfdTdT-ne*dPdne*dfdTdT+dPdT*(dfdnbdT*nb+dfdnedT*ne+s))/((pr+ed1)*(-dfdTdT));

  //cs_sq=dPdVstYeNb/depsilondVstYeNb;
  //cout<<pr+ed1<<" "<<dfdTdT<<" ";

  return cs_sq;
}


int eos::cs2_fixYe_mod(size_t nv, const ubvector &x,
				ubvector &y,double Ye) {
  double nb=x[0];
  double T=x[1];
  neutron.n=nb*(1.0-Ye);
  proton.n=nb*Ye;
  double cs_sq=cs2_fixYe(neutron,proton,T,th2);
  y[0]=cs_sq;
  y[1]=0.0;

  return 0;
}

double eos::cs2(fermion &n, fermion &p, double T, thermo &th) {
 
  double nn,pn;
  deriv_gsl<> gd;
  nn=n.n;
  pn=p.n;
  free_energy_density(n,p,T,th);
  thermo th1=th;
  fermion n1=n, p1=p;

  // Numerically compute required second derivatives
  
  // d^2f/dnn^2
  std::function<double(double)> dfdnn_totaldnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,
	      pn,T,std::ref(th));
  double dfdnn_totaldnn=gd.deriv(nn,dfdnn_totaldnnfunc);
  
  // d^2f/dnn/dnp
  std::function<double(double)> dfdnn_totaldpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  double dfdnn_totaldpn=gd.deriv(pn,dfdnn_totaldpnfunc);

  // d^2f/dnn/dT
  std::function<double(double)> dfdnn_totaldTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdnn_total),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  double dfdnn_totaldT=gd.deriv(T,dfdnn_totaldTfunc);

  // d^2f/dnp/dnn
  std::function<double(double)> dfdpn_totaldnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,pn,T,
	      std::ref(th));
  double dfdpn_totaldnn=gd.deriv(nn,dfdpn_totaldnnfunc);

  // d^2f/dnp^2
  std::function<double(double)> dfdpn_totaldpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  double dfdpn_totaldpn=gd.deriv(pn,dfdpn_totaldpnfunc);

  // d^2f/dnp/dT
  std::function<double(double)> dfdpn_totaldTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::dfdpn_total),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  double dfdpn_totaldT=gd.deriv(T,dfdpn_totaldTfunc);

  // d^2f/dT^2
  std::function<double(double)> dsdTfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),nn,pn,std::placeholders::_1,
	      std::ref(th));
  double dsdT=gd.deriv(T,dsdTfunc);

  // d^2f/dT/dnn
  std::function<double(double)> dsdnnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),std::placeholders::_1,pn,T,
	      std::ref(th));
  double dsdnn=gd.deriv(nn,dsdnnfunc);
  
  // d^2f/dT/dnp
  std::function<double(double)> dsdpnfunc=
    std::bind(std::mem_fn<double(fermion &, fermion &, double,
				 double, double, thermo &)>
	      (&eos::entropy),
	      this,std::ref(n),std::ref(p),nn,std::placeholders::_1,T,
	      std::ref(th));
  double dsdpn=gd.deriv(pn,dsdpnfunc);

  double dfdnbdT, dfdnedT, dfdnbdne, dfdTdT, dfdnbdnb, dfdnedne;
  dfdnbdT=dfdnn_totaldT;
  dfdnedT=dfdpn_totaldT-dfdnn_totaldT;
  dfdnbdne=dfdnn_totaldpn-dfdnn_totaldnn;
  dfdTdT=-dsdT;
  dfdnbdnb=dfdnn_totaldnn;
  dfdnedne=dfdpn_totaldpn+dfdnn_totaldnn-dfdpn_totaldnn-dfdnn_totaldpn;

  double dfdpn_total1=dfdpn_total(n,p,nn,pn,T,th);
  double dfdnn_total1=dfdnn_total(n,p,nn,pn,T,th);
  double mul,mub,s,ed1,nb;
  nb=n1.n+p1.n;
  mub=dfdnn_total1;
  mul=dfdpn_total1-dfdnn_total1;
  s=entropy(n1,p1,n1.n,p1.n,T,th1);
  ed1=ed(n1,p1,n1.n,p1.n,T,th1);
  
  double ne=p1.n;

  // Compute dSdTVmulNb/V
  double dSdTVmulNb=(-dfdTdT*dfdnedne+dfdnedT*dfdnedT)/dfdnedne;
  
  // Compute dSdVTmulNb
  double dSdVTmulNb=(s*dfdnedne+dfdnedT*dfdnedne*ne+dfdnbdT*dfdnedne*nb-
		     dfdnedT*dfdnedne*ne-dfdnedT*dfdnbdne*nb)/dfdnedne;
 
  // Compute dpdVTmulNb*V
  double dpdVTmulNb=(-dfdnbdnb*dfdnedne+dfdnbdne*dfdnbdne)*nb*nb/dfdnedne;

  // Compute dpdTVmulNb
  double dpdTVmulNb=dSdVTmulNb;

  // Compute dpdVSmulNb*V;
  double dpdVSmulNb=(dpdVTmulNb*dSdTVmulNb-dpdTVmulNb*dSdVTmulNb)/dSdTVmulNb;
 
  // Compute dNedVSmulNb
  double dNedVSmulNb=(dfdTdT*(dfdnedne*ne+dfdnbdne*nb)-
		      (dfdnedT*ne+dfdnbdT*nb)*dfdnedT)/
    (-dfdnedT*dfdnedT+dfdTdT*dfdnedne);

  double pr=mul*ne+mub*nb+T*s-ed1;
  
  // Compute depsilondVsmulNb*V
  double depsilondVSmulNb=-pr-ed1+mul*dNedVSmulNb;
  
  double cs_sq=dpdVSmulNb/depsilondVSmulNb;
  
  return cs_sq;
}

int eos::table_Ye(std::vector<std::string> &sv, bool itive_com) {

  std::string fname=sv[1];
  double Ye=o2scl::stod(sv[2]);

  size_t n_nB=301;
  size_t n_Ye=60;
  size_t n_T=160;
    
  std::string nB_grid_spec="10^(i*0.04-12)*2.0";
  std::string Ye_grid_spec="0.01*(i+1)";
  std::string T_grid_spec="0.2+0.81*i";

  vector<double> nB_grid, T_grid;
  
  calculator calc;
  std::map<std::string,double> vars;
  
  calc.compile(nB_grid_spec.c_str());
  for(size_t i=0;i<n_nB;i++) {
    vars["i"]=((double)i);
    nB_grid.push_back(calc.eval(&vars));
  }
  
  calc.compile(T_grid_spec.c_str());
  for(size_t i=0;i<n_T;i++) {
    vars["i"]=((double)i);
    T_grid.push_back(calc.eval(&vars));
  }

  table3d t;
  t.set_xy("nB",n_nB,nB_grid,"T",n_T,T_grid);
  t.new_slice("F");
  t.new_slice("s");
  t.new_slice("g");
  t.new_slice("dgdT");
  t.new_slice("msn");
  t.new_slice("msp");
  t.new_slice("pr");
  t.new_slice("f_deg");
  t.new_slice("f_virial");
  t.new_slice("s_virial");
  t.new_slice("f_total");
  t.new_slice("s_sign");
  t.new_slice("pr_sign");
  for(int i=n_nB-1;i>=0;i--) {
    cout << i << "/" << n_nB << endl;
    for(size_t j=0;j<n_T;j++) {
      neutron.n=nB_grid[i]*(1.0-Ye);
      proton.n=nB_grid[i]*Ye;
      double t1, t2, t3, t4, t5;
      free_energy_density(neutron,proton,T_grid[j]/hc_mev_fm,th2);
      double foa_hc=hc_mev_fm*(th2.ed-T_grid[j]/hc_mev_fm*th2.en)/
	(neutron.n+proton.n);
      t.set(i,j,"F",foa_hc);
      t.set(i,j,"f_total",th2.ed-T_grid[j]/hc_mev_fm*th2.en);
      t.set(i,j,"s",th2.en);
      t.set(i,j,"g",g_virial);
      t.set(i,j,"f_virial",f_virial);
      t.set(i,j,"s_virial",s_virial);
      t.set(i,j,"f_deg",f_deg);
      t.set(i,j,"dgdT",dgvirialdT);
      t.set(i,j,"pr",th2.pr);
      if (th2.pr>0.0) {
	t.set(i,j,"pr_sign",1.0);
      } else if (th2.pr<0.0) {
	t.set(i,j,"pr_sign",-1.0);
      }
      if (th2.en>0.0) {
	t.set(i,j,"s_sign",1.0);
      } else if (th2.en<0.0) {
	t.set(i,j,"s_sign",-1.0);
      }
      sk.calc_e(neutron,proton,th2);
      t.set(i,j,"msn",neutron.ms);
      t.set(i,j,"msp",proton.ms);
    }
  }

  for(size_t k=0;k<t.get_nslices();k++) {
    string sl_name=t.get_slice_name(k);
    for(size_t i=0;i<t.get_nx();i++) {
      for(size_t j=0;j<t.get_ny();j++) {
	if (!std::isfinite(t.get(i,j,sl_name))) {
	  cout << sl_name << " not finite at " << nB_grid[i] << " "
	       << T_grid[j] << endl;
	  exit(-1);
	}
      }
    }
  }

  hdf_file hf;
  hf.open_or_create(fname);
  hdf_output(hf,t,"table_Ye");
  hf.close();
  
  return 0;
}

int eos::table_full(std::vector<std::string> &sv, bool itive_com) {

  std::string fname=sv[1];

  size_t n_nB=301;
  size_t n_Ye=99;
  size_t n_T=160;
    
  std::string nB_grid_spec="10^(i*0.04-12)*2.0";
  std::string Ye_grid_spec="0.01*(i+1)";
  std::string T_grid_spec="0.2+0.81*i";

  vector<double> nB_grid, T_grid, Ye_grid;
  
  calculator calc;
  std::map<std::string,double> vars;
  
  calc.compile(nB_grid_spec.c_str());
  for(size_t i=0;i<n_nB;i++) {
    vars["i"]=((double)i);
    nB_grid.push_back(calc.eval(&vars));
  }
  
  calc.compile(Ye_grid_spec.c_str());
  for(size_t i=0;i<n_Ye;i++) {
    vars["i"]=((double)i);
    Ye_grid.push_back(calc.eval(&vars));
  }
  
  calc.compile(T_grid_spec.c_str());
  for(size_t i=0;i<n_T;i++) {
    vars["i"]=((double)i);
    T_grid.push_back(calc.eval(&vars));
  }

  size_t size_arr[3]={n_nB,n_Ye,n_T};
  vector<vector<double> > grid_arr={nB_grid,Ye_grid,T_grid};

  tensor_grid3<> t_F(n_nB,n_Ye,n_T);
  t_F.set_grid(grid_arr);
  tensor_grid3<> t_Fint(n_nB,n_Ye,n_T);
  t_Fint.set_grid(grid_arr);
  tensor_grid3<> t_E(n_nB,n_Ye,n_T);
  t_E.set_grid(grid_arr);
  tensor_grid3<> t_Eint(n_nB,n_Ye,n_T);
  t_Eint.set_grid(grid_arr);
  tensor_grid3<> t_P(n_nB,n_Ye,n_T);
  t_P.set_grid(grid_arr);
  tensor_grid3<> t_Pint(n_nB,n_Ye,n_T);
  t_Pint.set_grid(grid_arr);
  tensor_grid3<> t_S(n_nB,n_Ye,n_T);
  t_S.set_grid(grid_arr);
  tensor_grid3<> t_Sint(n_nB,n_Ye,n_T);
  t_Sint.set_grid(grid_arr);
  tensor_grid3<> t_mun(n_nB,n_Ye,n_T);
  t_mun.set_grid(grid_arr);
  tensor_grid3<> t_mup(n_nB,n_Ye,n_T);
  t_mup.set_grid(grid_arr);
  //tensor_grid3<> t_cs2(n_nB,n_Ye,n_T);
  //t_cs2.set_grid(grid_arr);

  eos_sn_oo eso;
  eso.include_muons=include_muons;
  
  for(int i=n_nB-1;i>=0;i--) {
    cout << "i_nB,n_nB: " << n_nB-1-i << " " << n_nB << endl;
    for(size_t j=0;j<n_Ye;j++) {
      for(size_t k=0;k<n_T;k++) {

	// Hadronic part
	neutron.n=nB_grid[i]*(1.0-Ye_grid[j]);
	proton.n=nB_grid[i]*Ye_grid[j];
	free_energy_density(neutron,proton,T_grid[k]/hc_mev_fm,th2);

	thermo lep;
	eso.compute_eg_point(nB_grid[i],Ye_grid[j],T_grid[k],lep);
	
	t_Fint.set(i,j,k,hc_mev_fm*(th2.ed-T_grid[k]/hc_mev_fm*th2.en)/
		   (neutron.n+proton.n));
	t_F.set(i,j,k,hc_mev_fm*(th2.ed+lep.ed-T_grid[k]/hc_mev_fm*th2.en)/
		(neutron.n+proton.n));
	t_Eint.set(i,j,k,hc_mev_fm*(th2.ed)/(neutron.n+proton.n));
	t_E.set(i,j,k,hc_mev_fm*(th2.ed+lep.ed)/(neutron.n+proton.n));
	t_Pint.set(i,j,k,hc_mev_fm*(th2.pr));
	t_P.set(i,j,k,hc_mev_fm*(th2.pr+lep.pr));
	t_Sint.set(i,j,k,hc_mev_fm*(th2.en)/(neutron.n+proton.n));
	t_S.set(i,j,k,hc_mev_fm*(th2.en+lep.en)/(neutron.n+proton.n));
	t_mun.set(i,j,k,hc_mev_fm*neutron.mu);
	t_mup.set(i,j,k,hc_mev_fm*proton.mu);

	//double cs2=cs2_fixYe(neutron,proton,T_grid[k]/hc_mev_fm,th2);
	//t_cs2.set(i,j,k,cs2);

	if (!std::isfinite(th2.ed)) {
	  cout << "Hadronic energy density not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "hadrons ed: " << th2.ed << " pr: " << th2.pr
	       << " en: " << th2.en << endl;
	  exit(-1);
	}
	if (!std::isfinite(th2.pr)) {
	  cout << "Hadronic pressure not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "hadrons ed: " << th2.ed << " pr: " << th2.pr
	       << " en: " << th2.en << endl;
	  exit(-1);
	}
	if (!std::isfinite(th2.en)) {
	  cout << "Hadronic entropy density not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "hadrons ed: " << th2.ed << " pr: " << th2.pr
	       << " en: " << th2.en << endl;
	  exit(-1);
	}
	if (!std::isfinite(lep.ed)) {
	  cout << "Leptonic energy density not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "leptons ed: " << lep.ed << " pr: " << lep.pr
	       << " en: " << lep.en << endl;
	  exit(-1);
	}
	if (!std::isfinite(lep.pr)) {
	  cout << "Leptonic pressure not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "leptons ed: " << lep.ed << " pr: " << lep.pr
	       << " en: " << lep.en << endl;
	  exit(-1);
	}
	if (!std::isfinite(lep.en)) {
	  cout << "Leptonic entropy density not finite." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "leptons ed: " << lep.ed << " pr: " << lep.pr
	       << " en: " << lep.en << endl;
	  exit(-1);
	}
	if (th2.en+lep.en<0.0 && th2.pr>0.0) {
	  cout << "Entropy negative where pressure is positive." << endl;
	  cout << "n_B: " << nB_grid[i] << " Y_e: " << Ye_grid[j] << " T: "
	       << T_grid[k] << endl;
	  cout << "hadrons ed: " << th2.ed << " pr: " << th2.pr
	       << " en: " << th2.en << endl;
	  cout << "leptons ed: " << lep.ed << " pr: " << lep.pr
	       << " en: " << lep.en << endl;
	  exit(-1);
	}

      }
    }
  }

  hdf_file hf;
  hf.open_or_create(fname);
  hf.set_szt("n_nB",n_nB);
  hf.set_szt("n_Ye",n_Ye);
  hf.set_szt("n_T",n_T);
  hf.setd_vec("nB_grid",nB_grid);
  hf.setd_vec("Ye_grid",Ye_grid);
  hf.setd_vec("T_grid",T_grid);
  hdf_output(hf,t_Fint,"Fint");
  hdf_output(hf,t_F,"F");
  hdf_output(hf,t_Eint,"Eint");
  hdf_output(hf,t_E,"E");
  hdf_output(hf,t_Pint,"Pint");
  hdf_output(hf,t_P,"P");
  hdf_output(hf,t_Sint,"Sint");
  hdf_output(hf,t_S,"S");
  hdf_output(hf,t_mun,"mun");
  hdf_output(hf,t_mup,"mup");
  //hdf_output(hf,t_cs2,"cs2");
  hf.close();
  
  return 0;
}

int eos::test_deriv(std::vector<std::string> &sv, bool itive_com) {

  if (false) {
    // Make the Skyrme EOSs more accurate
    sk.nrf.def_density_root.tol_rel/=1.0e2;
    sk.nrf.def_density_root.tol_abs/=1.0e2;
    sk_chiral.nrf.def_density_root.tol_rel/=1.0e2;
    sk_chiral.nrf.def_density_root.tol_abs/=1.0e2;
  }

  cout.precision(5);
  
  test_mgr t;
  t.set_output_level(1);

  if (model_selected==false) {
    cerr << "No model selected." << endl;
    return 1;
  }

  double err_fac;
    
  deriv_gsl<> gd;
  double mun_num, mup_num, en_num, mun_err, mup_err, en_err;
  double T, nn, np, nb, avg1, avg2, avg3, avg_sum=0.0;
  size_t count;

  // ---------------------------------------------------------------------

  cout << "T=0.1 MeV, Ye=0.01" << endl;

  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {

    // Neutron chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=0.1/hc_mev_fm;
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
    
    // Proton chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=0.1/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
    
    // Entropy
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=0.1/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;

    // Now compute analytical results from function
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=0.1/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);

    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;
    
    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=0.1, Ye=0.01");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=0.1, Ye=0.01");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=0.1, Ye=0.01");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;

  // ---------------------------------------------------------------------
  
  cout << "T=0.1 MeV, Ye=0.49" << endl;

  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=0.1/hc_mev_fm;
    
    // Neutron chemical potential
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
    
    // Proton chemical potential
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=0.1/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
    
    // Entropy
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=0.1/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;
    
    // Now compute analytical results from function
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=0.1/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);
    
    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;
    
    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=0.1, Ye=0.49");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=0.1, Ye=0.49");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=0.1, Ye=0.49");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;
  
  // ---------------------------------------------------------------------

  cout << "T=1 MeV, Ye=0.01" << endl;

  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {

    // Neutron chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
          
    // Proton chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
          
    // Entropy
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;

    // Now compute analytical results from function
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=1.0/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);
          
    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;

    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=1, Ye=0.01");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=1, Ye=0.01");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=1, Ye=0.01");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;

  // ---------------------------------------------------------------------

  cout << "T=1 MeV, Ye=0.49" << endl;

  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {

    // Neutron chemical potential
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
          
    // Proton chemical potential
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
          
    // Entropy
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=1.0/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;

    // Now compute analytical results from function
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=1.0/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);
          
    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;

    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=1, Ye=0.49");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=1, Ye=0.49");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=1, Ye=0.49");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;

  // ---------------------------------------------------------------------

  cout << "T=30 MeV, Ye=0.01" << endl;

  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {
          
    // Neutron chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
          
    // Proton chemical potential
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
          
    // Entropy
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;

    // Now compute analytical results from function
    neutron.n=nb*0.99;
    proton.n=nb*0.01;
    T=30.0/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);
          
    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;

    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=30, Ye=0.01");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=30, Ye=0.01");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=30, Ye=0.01");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;
  
  // ---------------------------------------------------------------------

  cout << "T=30 MeV, Ye=0.49" << endl;
  
  err_fac=1.0e4;
  avg1=0.0;
  avg2=0.0;
  avg3=0.0;
  for(count=0,nb=1.0e-10;nb<1.6;nb*=1.3,count++) {

    // Neutron chemical potential
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_nn=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),
		std::placeholders::_1,proton.n,T,
		std::ref(th2));
    gd.h=neutron.n/1.0e2;
    gd.deriv_err(neutron.n,fef_nn,mun_num,mun_err);
          
    // Proton chemical potential
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_np=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,
		std::placeholders::_1,T,std::ref(th2));
    gd.h=proton.n/1.0e2;
    gd.deriv_err(proton.n,fef_np,mup_num,mup_err);
          
    // Entropy
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=30.0/hc_mev_fm;
    std::function<double(double)> fef_en=
      std::bind(std::mem_fn<double(fermion &,fermion &,double,
				   double, double, thermo &)>
		(&eos::free_energy_density_alt),this,
		std::ref(neutron),std::ref(proton),neutron.n,proton.n,
		std::placeholders::_1,std::ref(th2));
    gd.h=T/1.0e2;
    gd.deriv_err(T,fef_en,en_num,en_err);
    en_num*=-1.0;

    // Now compute analytical results from function
    neutron.n=nb*0.51;
    proton.n=nb*0.49;
    T=30.0/hc_mev_fm;
    free_energy_density(neutron,proton,T,th2);
          
    double rat1=fabs((mun_num-neutron.mu)/mun_err);
    double rat2=fabs((mup_num-proton.mu)/mup_err);
    double rat3=fabs((en_num-th2.en)/en_err);
    if (count%10==0 || rat1>10.0 || rat2>10.0 || rat3>10.0) {
      cout << nb << " ";
      cout << rat1 << " ";
      cout << rat2 << " ";
      cout << rat3 << endl;
    }
    avg1+=rat1;
    avg2+=rat2;
    avg3+=rat3;
    avg_sum+=avg1+avg2+avg3;

    t.test_abs(neutron.mu,mun_num,mun_err*err_fac,"mun, T=30, Ye=0.49");
    t.test_abs(proton.mu,mup_num,mup_err*err_fac,"mup, T=30, Ye=0.49");
    t.test_abs(th2.en,en_num,en_err*err_fac,"en, T=30, Ye=0.49");
  }
  cout << avg1/((double)count) << " ";
  cout << avg2/((double)count) << " ";
  cout << avg3/((double)count) << endl;
  cout << endl;
  cout << "avg_sum=" << avg_sum << endl;
  cout << endl;

  // ---------------------------------------------------------------------
  // Summarize

  t.report();
  
  return 0;
}

int eos::eos_sn(std::vector<std::string> &sv, bool itive_com) {

  eos_sn_oo1 eso;
  eos_had_rmf rmf;
  eso.verbose=0;
  hdf_file hf;

  /* Attempt to download the EOS file from isospin if necessary
   */
  cloud_file cf;
  cf.verbose=2;
  std::string sha=((std::string)"d8c4d4f1315942a663e96fc6452f66d90fc")+
    "87f283e0ed552c8141d1ddba34c19";
  cf.hash_type=cloud_file::sha256;
  cf.hdf5_open_hash(hf,"LS220_234r_136t_50y_analmu_20091212_SVNr26.h5",
		    sha,((string)"https://isospin.roam.utk.edu/")+
		    "public/eos_tables/scollapse/LS220_234r_136t_50y_"+
		    "analmu_20091212_SVNr26.h5","data");
  
  vector<vector<double> > pts={{0.16,0.01,0.1},
			       {0.16,0.01,10.0},
			       {0.48,0.5,0.1},
			       {0.48,0.01,0.1},
			       {0.01,0.5,10.0},
			       {0.01,0.5,0.1},
			       {0.004,0.5,10.0},
			       {0.004,0.5,0.1},
			       {0.001,0.5,10.0},
			       {0.001,0.5,0.1},
			       {0.0004,0.5,10.0},
			       {0.0004,0.5,0.1},
			       {0.0001,0.5,10.0},
			       {0.0001,0.5,0.1}};
  
  // -----------------------------------------------------------------
  // LS220
  
  double f, F_eg, fint;

  eso.load("data/LS220_234r_136t_50y_analmu_20091212_SVNr26.h5",
	   eos_sn_oo::ls_mode);
  
  cout << endl;
  cout << "LS220: " << endl;
  cout << endl;

  for(size_t i=0;i<pts.size();i++) {
    double lnB=pts[i][0];
    double lYe=pts[i][1];
    double lT=pts[i][2];
    
    cout << "nB: " << lnB << " Ye: " << lYe
	 << " T: " << lT << endl;
    f=eso.F.interp_linear(lnB,lYe,lT);
    F_eg=eso.compute_eg_point(lnB,lYe,lT);
    cout << "F_full,F_eg,Xn,Xa: " 
	 << f << " " << F_eg << " "
	 << eso.Xn.interp_linear(lnB,lYe,lT) << " "
	 << eso.Xalpha.interp_linear(lnB,lYe,lT) << endl;
    cout << "F_int: " << f-F_eg << endl;
    cout << endl;
  }

  // -----------------------------------------------------------------
  // SFHo (from RMF EOS object)

  cout << endl;
  cout << "SFHo (eos_had_rmf): " << endl;
  cout << endl;

  rmf_load(rmf,"SFHo");
  
  for(size_t i=0;i<pts.size();i++) {
    if (i!=4 && i!=6 && i!=8) {
      double lnB=pts[i][0];
      double lYe=pts[i][1];
      double lT=pts[i][2];
      
      cout << i << " nB: " << lnB << " Ye: " << lYe
	   << " T: " << lT << endl;
      neutron.n=lnB*(1.0-lYe);
      proton.n=lnB*lYe;
      rmf.calc_temp_e(neutron,proton,lT/hc_mev_fm,th2);
      f=(th2.ed-lT/hc_mev_fm*th2.en)*hc_mev_fm/lnB;
      F_eg=eso.compute_eg_point(lnB,lYe,lT);
      cout << "F_full,F_eg,Xn,Xa: " 
	   << f+F_eg << " " << F_eg << " " << 0.0 << " "
	   << 0.0 << endl;
      cout << "F_int: " << f << endl;
      cout << endl;
    }
  }

  // -----------------------------------------------------------------
  // SFHo (O'Connor's table)

  eso.load("data/Hempel_SFHoEOS_rho222_temp180_ye60_version_1.1_20120817.h5",
	   eos_sn_oo::hfsl_mode);
  
  cout << endl;
  cout << "SFHo (O'Connor): " << endl;
  cout << endl;

  for(size_t i=0;i<pts.size();i++) {
    double lnB=pts[i][0];
    double lYe=pts[i][1];
    double lT=pts[i][2];
    
    cout << "nB: " << lnB << " Ye: " << lYe
	 << " T: " << lT << endl;
    f=eso.F.interp_linear(lnB,lYe,lT);
    F_eg=eso.compute_eg_point(lnB,lYe,lT);
    cout << "F_full,F_eg,Xn,Xa: " 
	 << f << " " << F_eg << " "
	 << eso.Xn.interp_linear(lnB,lYe,lT) << " "
	 << eso.Xalpha.interp_linear(lnB,lYe,lT) << endl;
    cout << "F_int: " << f-F_eg << endl;
    cout << endl;
  }

  // --------------------------------------------------------------------
  // SFHo (Hempel's table)
  
  eos_sn_hfsl esh;
  esh.verbose=0;
  esh.load("data/sfho_frdm_shen98_v1.03.tab");
  
  cout << endl;
  cout << "SFHo (Hempel): " << endl;
  cout << endl;

  for(size_t i=0;i<pts.size();i++) {
    double lnB=pts[i][0];
    double lYe=pts[i][1];
    double lT=pts[i][2];
    
    cout << "nB: " << lnB << " Ye: " << lYe
	 << " T: " << lT << endl;
    fint=eso.Fint.interp_linear(lnB,lYe,lT);
    F_eg=eso.compute_eg_point(lnB,lYe,lT);
    cout << "F_full,F_eg,Xn,Xa: " 
	 << fint+F_eg << " " << F_eg << " "
	 << eso.Xn.interp_linear(lnB,lYe,lT) << " "
	 << eso.Xalpha.interp_linear(lnB,lYe,lT) << endl;
    cout << "F_int: " << fint << endl;
    cout << endl;
  }

  // -----------------------------------------------------------------
  // SFHx (from RMF EOS object)

  cout << endl;
  cout << "SFHx (eos_had_rmf): " << endl;
  cout << endl;

  rmf_load(rmf,"SFHx");
  
  for(size_t i=0;i<pts.size();i++) {
    if (i!=12) {
      double lnB=pts[i][0];
      double lYe=pts[i][1];
      double lT=pts[i][2];
      
      cout << i << " nB: " << lnB << " Ye: " << lYe
	   << " T: " << lT << endl;
      neutron.n=lnB*(1.0-lYe);
      proton.n=lnB*lYe;
      rmf.calc_temp_e(neutron,proton,lT/hc_mev_fm,th2);
      f=(th2.ed-lT/hc_mev_fm*th2.en)*hc_mev_fm/lnB;
      F_eg=eso.compute_eg_point(lnB,lYe,lT);
      cout << "F_full,F_eg,Xn,Xa: " 
	   << f+F_eg << " " << F_eg << " " << 0.0 << " "
	   << 0.0 << endl;
      cout << "F_int: " << f << endl;
      cout << endl;
    }
  }

  // -----------------------------------------------------------------
  // SFHx

  eso.load("data/Hempel_SFHxEOS_rho234_temp180_ye60_version_1.1_20120817.h5",
	   eos_sn_oo::hfsl_mode);

  cout << endl;
  cout << "SFHx (O'Connor): " << endl;
  cout << endl;

  for(size_t i=0;i<pts.size();i++) {
    if (i!=12) {
      double lnB=pts[i][0];
      double lYe=pts[i][1];
      double lT=pts[i][2];
      
      cout << "nB: " << lnB << " Ye: " << lYe
	   << " T: " << lT << endl;
      fint=eso.Fint.interp_linear(lnB,lYe,lT);
      F_eg=eso.compute_eg_point(lnB,lYe,lT);
      cout << "F_full,F_eg,Xn,Xa: " 
	   << fint+F_eg << " " << F_eg << " "
	   << eso.Xn.interp_linear(lnB,lYe,lT) << " "
	   << eso.Xalpha.interp_linear(lnB,lYe,lT) << endl;
      cout << "F_int: " << fint << endl;
      cout << endl;
    }
  }

  // -----------------------------------------------------------------
  // IUFSU (from RMF EOS object)

  cout << endl;
  cout << "IUFSU (eos_had_rmf): " << endl;
  cout << endl;

  rmf_load(rmf,"IUFSU");
  
  for(size_t i=0;i<pts.size();i++) {
    if (i!=6) {
      double lnB=pts[i][0];
      double lYe=pts[i][1];
      double lT=pts[i][2];
      
      cout << i << " nB: " << lnB << " Ye: " << lYe
	   << " T: " << lT << endl;
      neutron.n=lnB*(1.0-lYe);
      proton.n=lnB*lYe;
      rmf.calc_temp_e(neutron,proton,lT/hc_mev_fm,th2);
      f=(th2.ed-lT/hc_mev_fm*th2.en)*hc_mev_fm/lnB;
      F_eg=eso.compute_eg_point(lnB,lYe,lT);
      cout << "F_full,F_eg,Xn,Xa: " 
	   << f+F_eg << " " << F_eg << " " << 0.0 << " "
	   << 0.0 << endl;
      cout << "F_int: " << f << endl;
      cout << endl;
    }
  }

  // -----------------------------------------------------------------
  // IUFSU

  eso.load("data/Hempel_IUFEOS_rho234_temp180_ye60_version_1.1_20140129.h5",
	   eos_sn_oo::hfsl_mode);

  cout << endl;
  cout << "IUFSU (O'Connor): " << endl;
  cout << endl;

  for(size_t i=0;i<pts.size();i++) {
    double lnB=pts[i][0];
    double lYe=pts[i][1];
    double lT=pts[i][2];
    
    cout << "nB: " << lnB << " Ye: " << lYe
	 << " T: " << lT << endl;
    fint=eso.Fint.interp_linear(lnB,lYe,lT);
    F_eg=eso.compute_eg_point(lnB,lYe,lT);
    cout << "F_full,F_eg,Xn,Xa: " 
	 << fint+F_eg << " " << F_eg << " "
	 << eso.Xn.interp_linear(lnB,lYe,lT) << " "
	 << eso.Xalpha.interp_linear(lnB,lYe,lT) << endl;
    cout << "F_int: " << fint << endl;
    cout << endl;
  }

  return 0;
}

int eos::solve_Ye(size_t nv, const ubvector &x, ubvector &y,
			   double nb, double T, double muL) {

  double Ye=x[0];
  // The temperature here is in 1/fm
  neutron.n=nb*(1.0-Ye);
  proton.n=nb*Ye;

  sk.eff_mass(neutron,proton);
  if (neutron.ms<0.0 || proton.ms<0.0) return 1;
  
  free_energy_density(neutron,proton,T,th2);
  
  photon.massless_calc(T);

  electron.n=proton.n;
  electron.mu=electron.m; 
  relf.pair_density(electron,T);
  
  y[0]=neutron.mu-proton.mu-electron.mu+muL+neutron.m-proton.m;
  return 0;
}

int eos::solve_T(size_t nv, const ubvector &x, ubvector &y,
			  double nb, double Ye,double sonb) {
  
  double T=x[0];
  // The temperature here is in 1/fm
  neutron.n=nb*(1.0-Ye);
  proton.n=nb*Ye;

  sk.eff_mass(neutron,proton);
  if (neutron.ms<0.0 || proton.ms<0.0) return 1;
  
  free_energy_density(neutron,proton,T,th2);
  
  photon.massless_calc(T);

  electron.n=proton.n;
  electron.mu=electron.m; 
  relf.pair_density(electron,T);

  y[0]=(th2.en+electron.en+photon.en)/nb-sonb;
  return 0;
}

int eos::mcarlo_data(std::vector<std::string> &sv, bool itive_com) {

  table<> t;
  t.line_of_names(((string)"index S L qmc_a qmc_b qmc_alpha ")+
		  "qmc_beta i_ns i_skyrme phi eos_n0 eos_EoA eos_K chi2_ns "+
		  "ns_fit0 ns_fit1 ns_fit2 ns_fit3 ns_fit4 "+
		  "F_0004_50_10 F_016_01_01 F_016_01_10 "+
		  "F_048_01_01 F_048_50_01 F_100_50_10 "+
		  "ns_min_cs2 ns_max_cs2");

  vector<double> nB_arr={0.004,0.16,0.16,0.48,0.48,1.0};
  vector<double> Ye_arr={0.5,0.01,0.01,0.01,0.5,0.5};
  vector<double> T_arr={10.0,0.1,10.0,0.1,0.1,10.0};

  static const int N=10000;
  for(int j=0;j<N;j++){
    
    std::vector<std::string> obj;
    random(obj,false);

    vector<double> line={((double)j),eos_S,eos_L,qmc_a,qmc_b,qmc_alpha,
			 qmc_beta,((double)i_ns),((double)i_skyrme),phi,
			 eos_n0,eos_EoA,eos_K,chi2_ns,ns_fit_parms[0],
			 ns_fit_parms[1],ns_fit_parms[2],ns_fit_parms[3],
			 ns_fit_parms[4]};
    
    for(size_t k=0;k<6;k++) {
      neutron.n=nB_arr[k]*(1.0-Ye_arr[k]);
      proton.n=nB_arr[k]*Ye_arr[k];
      double T=T_arr[k]/hc_mev_fm;
      line.push_back(free_energy_density(neutron,proton,T,th2)/
		     nB_arr[k]*hc_mev_fm);
    }

    double ns_min_cs2, ns_max_cs2;
    min_max_cs2(ns_min_cs2,ns_max_cs2);
    line.push_back(ns_min_cs2);
    line.push_back(ns_max_cs2);

    cout << "Line: ";
    for(size_t i=0;i<line.size();i++) {
      cout << line[i] << " ";
    }
    cout << endl;
    
    t.line_of_data(line.size(),line);
    if (line.size()!=t.get_ncolumns()) {
      O2SCL_ERR("Table sync error in mcarlo_data().",exc_esanity);
    }

    if (j%100==0 || j==N-1) {
      hdf_file hf1;
      std::string fname="mcarlo_data";
      if (sv.size()>1) fname+="_"+sv[1];
      fname+=".o2";
      hf1.open_or_create(fname);
      o2scl_hdf::hdf_output(hf1,t,"mcarlo");
      hf1.close();
    }    
  }
    
  return 0;
}

int eos::vir_comp(std::vector<std::string> &sv, bool itive_com) {

  table<> t, t2;
  t.line_of_names("log_nB F");
  t2.line_of_names("log_nB zn F_vir");

  for(int j=0;j<1000;j++){
    std::vector<std::string> obj;
    random(obj,false);

    for(double nb=1.0e-4;nb<4.001e-1;nb*=pow(4000.0,1.0/99.0)) {
      neutron.n=nb/2.0;
      proton.n=nb/2.0;
      double T=5.0/hc_mev_fm;
      double line[2]={log10(nb),free_energy_density(neutron,proton,T,th2)/
		      nb*hc_mev_fm};
      t.line_of_data(2,line);
      if (j==0) {
	double F_vir=free_energy_density_virial(neutron,proton,T,th2)/
	  nb*hc_mev_fm;
	double zn=exp(neutron.mu/T);
	double line2[3]={log10(nb),zn,F_vir};
	t2.line_of_data(3,line2);
      }
    }
  }
  
  hdf_file hf;
  hf.open_or_create("vir_comp.o2");
  hdf_output(hf,t,"vir_comp");
  hdf_output(hf,t2,"vir_comp2");
  hf.close();
  
  return 0;
}

int eos::select_model(std::vector<std::string> &sv, bool itive_com) {
 
  i_ns=o2scl::stod(sv[1]);
  i_skyrme=o2scl::stod(sv[2]);
  qmc_alpha=o2scl::stod(sv[3]);
  qmc_a=o2scl::stod(sv[4]);
  eos_L=o2scl::stod(sv[5]);
  eos_S=o2scl::stod(sv[6]);
  phi=o2scl::stod(sv[7]);

  int iret=select_internal(i_ns,i_skyrme,qmc_alpha,qmc_a,eos_L,eos_S,phi);
  if (iret!=0) {
    cerr << "Model is unphysical (iret=" << iret << ")." << endl;
    return 1;
  }
  
  return 0;
}

int eos::select_internal(int i_ns_loc, int i_skyrme_loc,
				  double qmc_alpha_loc, double qmc_a_loc,
				  double eos_L_loc, double eos_S_loc,
				  double phi_loc) {
  
  i_ns=i_ns_loc;
  i_skyrme=i_skyrme_loc;
  qmc_alpha=qmc_alpha_loc;
  qmc_a=qmc_a_loc;
  eos_L=eos_L_loc;
  eos_S=eos_S_loc;
  phi=phi_loc;
  
  model_selected=true;

  ns_fit(i_ns);
  
  double ns_min_cs2, ns_max_cs2;
  min_max_cs2(ns_min_cs2,ns_max_cs2);
  if (ns_min_cs2<0.0) {
    model_selected=false;
    return 1;
  }

  if (9.17*eos_S-266.0>eos_L || 14.3*eos_S-379.0<eos_L) {
    model_selected=false;
    return 2;
  }

  double rho0=UNEDF_tab.get(((string)"rho0"),i_skyrme);
  double Crdr0=UNEDF_tab.get(((string)"Crdr0"),i_skyrme);
  double Vp=UNEDF_tab.get(((string)"Vp"),i_skyrme);
  double EoA=UNEDF_tab.get(((string)"EoA"),i_skyrme); 
  double Crdr1=UNEDF_tab.get(((string)"Crdr1"),i_skyrme);
  double CrdJ0=UNEDF_tab.get(((string)"CrdJ0"),i_skyrme);
  double K=UNEDF_tab.get(((string)"K"),i_skyrme);
  double Ms_inv=UNEDF_tab.get(((string)"Ms_inv"),i_skyrme);
  double Vn=UNEDF_tab.get(((string)"Vn"),i_skyrme);
  double CrdJ1=UNEDF_tab.get(((string)"CrdJ1"),i_skyrme);

  // Store some of the nuclear matter parameters
  eos_n0=rho0;
  eos_EoA=EoA;
  eos_K=K;
    
  // Determine QMC coefficients
  qmc_b=eos_S+EoA-qmc_a;
  qmc_beta=(eos_L/3.0-qmc_a*qmc_alpha)/qmc_b;
    
  if (qmc_b<0.0 || qmc_beta>5.0) {
    model_selected=false;
    return 3;
  }
  
  double Ms_star=1/Ms_inv;
    
  sk.alt_params_saturation(rho0,EoA/hc_mev_fm,K/hc_mev_fm,Ms_star,
			   eos_S/hc_mev_fm,eos_L/hc_mev_fm,1.0/1.249,
			   Crdr0/hc_mev_fm,Crdr1/hc_mev_fm,
			   CrdJ0/hc_mev_fm,CrdJ1/hc_mev_fm);

  // Test to make sure dineutrons are not bound
  for(double nb=0.01;nb<0.16;nb+=0.001) {
    neutron.n=nb;
    proton.n=0.0;
    sk.calc_e(neutron,proton,th2);
    if (th2.ed/nb<0.0) {
      model_selected=false;
      return 4;
    }
  }

  // Ensure effective masses are positive
  
  fermion &n=neutron;
  fermion &p=proton;
  thermo &th=th2;
  double term, term2;
  
  n.n=1.0;
  p.n=1.0;
  
  term=0.25*(sk.t1*(1.0+sk.x1/2.0)+sk.t2*(1.0+sk.x2/2.0));
  term2=0.25*(sk.t2*(0.5+sk.x2)-sk.t1*(0.5+sk.x1));
  n.ms=n.m/(1.0+2.0*((n.n+p.n)*term+n.n*term2)*n.m);
  p.ms=p.m/(1.0+2.0*((n.n+p.n)*term+p.n*term2)*p.m);
  
  if (n.ms<0.0 || p.ms<0.0) {
    model_selected=false;
    return 5;
  }
  
  n.n=2.0;
  p.n=0.0;
  
  term=0.25*(sk.t1*(1.0+sk.x1/2.0)+sk.t2*(1.0+sk.x2/2.0));
  term2=0.25*(sk.t2*(0.5+sk.x2)-sk.t1*(0.5+sk.x1));
  n.ms=n.m/(1.0+2.0*((n.n+p.n)*term+n.n*term2)*n.m);
  p.ms=p.m/(1.0+2.0*((n.n+p.n)*term+p.n*term2)*p.m);
  
  if (n.ms<0.0 || p.ms<0.0) {
    model_selected=false;
    return 6;
  }
  
  n.n=0.0;
  p.n=2.0;
  
  term=0.25*(sk.t1*(1.0+sk.x1/2.0)+sk.t2*(1.0+sk.x2/2.0));
  term2=0.25*(sk.t2*(0.5+sk.x2)-sk.t1*(0.5+sk.x1));
  n.ms=n.m/(1.0+2.0*((n.n+p.n)*term+n.n*term2)*n.m);
  p.ms=p.m/(1.0+2.0*((n.n+p.n)*term+p.n*term2)*p.m);
  
  if (n.ms<0.0 || p.ms<0.0) {
    model_selected=false;
    return 7;
  }

  // --------------------------------------------------------
  // Test beta equilibrium
  
  // Loop over baryon densities
  cout << "Going to beta-eq test: " << endl;
  for(double nbx=0.1;nbx<2.00001;nbx+=0.05) {
    
    // Beta equilibrium at T=1 MeV
    mm_funct mf=std::bind
      (std::mem_fn<int(size_t,const ubvector &,ubvector &,
		       double, double,double)>
       (&eos::solve_Ye),this,std::placeholders::_1, 
       std::placeholders::_2,std::placeholders::_3,nbx,1.0/hc_mev_fm,0.0);
    
    int verbose_store=verbose;
    verbose=0;

    ubvector Ye_trial(1);
    Ye_trial[0]=0.05;
    mroot_hybrids<> mh;
    mh.err_nonconv=false;
    mh.def_jac.err_nonconv=false;
    int ret=mh.msolve(1,Ye_trial,mf);
    if (ret!=0) {
      model_selected=false;
      return 8;
    }
    
    double Ye=Ye_trial[0];
    
    verbose=verbose_store;
    
    if (Ye<0.0 || Ye>1.0) {
      model_selected=false;
      return 9;
    }
    
  }

  // --------------------------------------------------------
  // Test cs2
  
  model_selected=true;

  if (select_cs2_test) {
    cout << "Going to cs2 test: " << endl;
    for(double nbx=0.1;nbx<2.00001;nbx+=0.05) {
      for(double yex=0.05;yex<0.4501;yex+=0.1) {
	for(double Tx=1.0/hc_mev_fm;Tx<10.01/hc_mev_fm;Tx+=9.0/hc_mev_fm) {
	  neutron.n=nbx*(1.0-yex);
	  proton.n=nbx*yex;
	  double cs2x=cs2_fixYe(neutron,proton,Tx,th2);
	  if (cs2x<0.0) {
	    cout << "Negative speed of sound." << endl;
	    cout << nbx << " " << yex << " " << Tx*hc_mev_fm << endl;
	    exit(-1);
	  }
	}
      }
    }
  }

  return 0;
}

int eos::random(std::vector<std::string> &sv, bool itive_com) {

  // This function never fails, and it requires a call to
  // free_energy_density(), so we set this to true
  model_selected=true;

  r.clock_seed();
  
  bool done=false;
  while (done==false) {

    done=true;
    
    if (verbose>0) {
      cout << "Selecting random model." << endl;
    }

    // Select a random value for phi
    phi=r.random();

    // Random neutron star EOS
    i_ns=r.random_int(nstar_tab.get_nlines());

    // Select a random QMC two-body interaction
    qmc_alpha=(r.random()*0.06)+0.47;
    qmc_a=(r.random()*1.0)+12.5;
   
    // Select a random value of S and L according
    // to perscription in PRC 91, 015804 (2015)
    eos_L=r.random()*21.0+44.0;
    eos_S=r.random()*6.6+29.5;

    // Select a random Skyrme model
    i_skyrme=r.random_int(UNEDF_tab.get_nlines());
    
    if (true || verbose>1) {
      cout << "Trying random model: " << endl;
      cout << "i_ns= " << i_ns << endl;
      cout << "i_skyrme= " << i_skyrme << endl;
      cout << "alpha= " << qmc_alpha << endl;
      cout << "a= " << qmc_a << endl;
      cout << "eos_L= " << eos_L << endl;
      cout << "eos_S= " << eos_S << endl;
      cout << "phi= " << phi << endl;
    }

    int ret=select_internal(i_ns,i_skyrme,qmc_alpha,qmc_a,
			    eos_L,eos_S,phi);
    if (true || verbose>1) {
      if (ret==0) {
	cout << "Success." << endl;
      } else {
	cout << "Failed (" << ret << "). Selecting new random model." << endl;
      }
    }

    if (ret!=0) done=false;
  }
  
  if (true) {
    cout << "Function eos::random() selected parameters: " << endl;
    cout << i_ns << " " << i_skyrme << " " << qmc_alpha
	 << " " << qmc_a << " " << eos_L << " " << eos_S << " "
	 << phi << endl;
  }
  
  return 0;
}

int eos::point(std::vector<std::string> &sv, bool itive_com) {

  if (model_selected==false) {
    cerr << "No model selected." << endl;
    return 1;
  }

  double nB=o2scl::stod(sv[1]);
  double Ye=o2scl::stod(sv[2]);
  double T=o2scl::stod(sv[3])/hc_mev_fm;

  neutron.n=nB*(1.0-Ye);
  proton.n=nB*Ye;
  free_energy_density(neutron,proton,T,th2);

  return 0;
}

int eos::test_eg(std::vector<std::string> &sv,
			  bool itive_com) {
  int n_nB_init=326;
  int n_Ye_init=60;
  int n_T_init=80;
  
  eos_sn_oo1 eso;
  eso.include_muons=true;
  
  for(int i=0;i<326;i++) {
    double nB=pow(10.0,i*0.04-12.0);
    if (i%10==0) {
      cout << "i,nB: " << i << " " << nB << endl;
    }
    for(int j=0;j<61;j++) {
      double Ye=0.01*j;
      for(int k=0;k<81;k++) {
	double T_MeV;
	if (k==0) T_MeV=0.0;
	else T_MeV=pow(10.0,(k-1)*0.04-1.0);
	eso.compute_eg_point(nB,Ye,T_MeV);
      }
    }
  }
  return 0;
}

int eos::vir_fit(std::vector<std::string> &sv,
			  bool itive_com) {
  ecv.fit(true);
  return 0;
}

int main(int argc, char *argv[]) {

  cout.setf(ios::scientific);

  eos eph;
  cli cl;
  
  static const int nopt=11;
  o2scl::comm_option_s options[nopt]={
    {0,"test_deriv","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::test_deriv),o2scl::cli::comm_option_both},
    {0,"table_Ye","Desc.",2,2,"<fname> <Ye>","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::table_Ye),o2scl::cli::comm_option_both},
    {0,"table_full","Desc.",1,1,"<fname>","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::table_full),o2scl::cli::comm_option_both},
    {0,"vir_fit","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::vir_fit),o2scl::cli::comm_option_both},
    {0,"eos_sn","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::eos_sn),o2scl::cli::comm_option_both},
    {0,"mcarlo_data","Desc.",0,1,"Monte Carlo function","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::mcarlo_data),o2scl::cli::comm_option_both},
    {0,"point","Desc.",0,3,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::point),o2scl::cli::comm_option_both},
    {0,"random","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::random),o2scl::cli::comm_option_both},
    {0,"select_model","Desc.",7,7,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::select_model),o2scl::cli::comm_option_both},
    {0,"teg","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::test_eg),o2scl::cli::comm_option_both},
    {0,"vir_comp","Desc.",0,0,"","",
     new o2scl::comm_option_mfptr<eos>
     (&eph,&eos::vir_comp),o2scl::cli::comm_option_both}
  };
  cl.set_comm_option_vec(nopt,options);
  cl.gnu_intro=false;

  o2scl::cli::parameter_int p_verbose;
  p_verbose.i=&eph.verbose;
  p_verbose.help="Verbose parameter (default 1)";
  cl.par_list.insert(make_pair("verbose",&p_verbose));

  o2scl::cli::parameter_bool p_old_ns_fit;
  p_old_ns_fit.b=&eph.old_ns_fit;
  p_old_ns_fit.help="Old NS fit (default 0)";
  cl.par_list.insert(make_pair("old_ns_fit",&p_old_ns_fit));

  o2scl::cli::parameter_bool p_ns_record;
  p_ns_record.b=&eph.ns_record;
  p_ns_record.help="Record NS fit (default 0)";
  cl.par_list.insert(make_pair("ns_record",&p_ns_record));

  o2scl::cli::parameter_bool p_include_muons;
  p_include_muons.b=&eph.include_muons;
  p_include_muons.help="If true, include muons (default false)";
  cl.par_list.insert(make_pair("include_muons",&p_include_muons));

  o2scl::cli::parameter_bool p_select_cs2_test;
  p_select_cs2_test.b=&eph.select_cs2_test;
  p_select_cs2_test.help="Test cs2 in select_internal() (default 1)";
  cl.par_list.insert(make_pair("select_cs2_test",&p_select_cs2_test));

  o2scl::cli::parameter_bool p_test_ns_cs2;
  p_test_ns_cs2.b=&eph.test_ns_cs2;
  p_test_ns_cs2.help="Test neutron star cs2 (default 0)";
  cl.par_list.insert(make_pair("test_ns_cs2",&p_test_ns_cs2));

  o2scl::cli::parameter_double p_a_virial;
  p_a_virial.d=&eph.a_virial;
  p_a_virial.help="Virial coefficient a (default 3.0)";
  cl.par_list.insert(make_pair("a_virial",&p_a_virial));

  o2scl::cli::parameter_double p_b_virial;
  p_b_virial.d=&eph.b_virial;
  p_b_virial.help="Virial coefficient b (default 0.0)";
  cl.par_list.insert(make_pair("b_virial",&p_b_virial));

  cl.run_auto(argc,argv);
  
  return 0;
}