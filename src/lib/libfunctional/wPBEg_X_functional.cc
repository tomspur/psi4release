/**********************************************************
* wPBEg_X_functional.cc: definitions for wPBEg_X_functional for KS-DFT
* Robert Parrish, robparrish@gmail.com
* Autogenerated by MATLAB Script on 07-Mar-2012
*
***********************************************************/
#include <libmints/mints.h>
#include <libfock/points.h>
#include "wPBEg_X_functional.h"
#include <stdlib.h>
#include <cmath>
#include <string>
#include <string>
#include <vector>

using namespace psi;
using namespace boost;
using namespace std;

namespace psi { namespace functional {

wPBEg_X_Functional::wPBEg_X_Functional(int npoints, int deriv) : Functional(npoints, deriv)
{

    name_ = "wPBEg_X";
    description_ = "Fake SR PBE Exchange Functional II";
    citation_ = "J.P. Perdew et. al., Phys. Rev. Lett., 77(18), 3865-3868, 1996";

    double k = 3.0936677262801355E+00;
    params_.push_back(make_pair("k",k));
    double e = -2.3873241463784300E-01;
    params_.push_back(make_pair("e",e));
    double kp = 8.0400000000000005E-01;
    params_.push_back(make_pair("kp",kp));
    double mu_ = 2.1951497276451709E-01;
    params_.push_back(make_pair("mu_",mu_));

    is_gga_ = true;
    is_meta_ = false;

    //Required allocateion
    allocate();
}
wPBEg_X_Functional::~wPBEg_X_Functional()
{
}
void wPBEg_X_Functional::computeRKSFunctional(boost::shared_ptr<RKSFunctions> prop)
{
    int ntrue = prop->npoints();

    const double* restrict rho_a;
    const double* restrict gamma_aa;
    const double* restrict tau_a;

    rho_a = prop->property_value("RHO_A")->pointer();
    if (is_gga_) {
        gamma_aa = prop->property_value("GAMMA_AA")->pointer();
    }
    if (is_meta_) {
        tau_a = prop->property_value("TAU_A")->pointer();
    }

    double k = params_[0].second;
    double e = params_[1].second;
    double kp = params_[2].second;
    double mu_ = params_[3].second;

    //Functional
    for (int index = 0; index < ntrue; index++) {

        //Functional Value
        if (rho_a[index] > cutoff_) {
            double t169105 = rho_a[index]*2.0;
            functional_[index] = e*k*rho_a[index]*pow(t169105,1.0/3.0)*(kp-kp/((gamma_aa[index]*1.0/(k*k)*mu_* \
               1.0/pow(t169105,8.0/3.0))/kp+1.0)+1.0)*2.0;
        } else {
            functional_[index] = 0.0;
        } 

    }
    //First Partials
    for (int index = 0; index < ntrue && deriv_ >= 1; index++) {

        //V_rho_a
        if (rho_a[index] > cutoff_) {
            double t169107 = rho_a[index]*2.0;
            double t169108 = 1.0/(k*k);
            double t169109 = 1.0/kp;
            double t169110 = 1.0/pow(t169107,8.0/3.0);
            double t169111 = gamma_aa[index]*mu_*t169110*t169108*t169109;
            double t169112 = t169111+1.0;
            double t169113 = 1.0/t169112;
            double t169114 = kp-kp*t169113+1.0;
            v_rho_a_[index] = e*k*t169114*pow(t169107,1.0/3.0)+e*k*rho_a[index]*t169114*1.0/pow(t169107,2.0/3.0) \
               *(2.0/3.0)-(e*gamma_aa[index]*mu_*rho_a[index]*1.0/(t169112*t169112)*1.0/pow(t169107,1.0E1/3.0)*(1.6E1/ \
               3.0))/k;
        } else {
            v_rho_a_[index] = 0.0;
        } 

        if (is_gga_) {

            if (rho_a[index] > cutoff_) {
                double t169116 = rho_a[index]*2.0;
                v_gamma_aa_[index] = (e*mu_*rho_a[index]*1.0/pow(t169116,7.0/3.0)*1.0/pow((gamma_aa[index]*1.0/(k* \
                   k)*mu_*1.0/pow(t169116,8.0/3.0))/kp+1.0,2.0)*2.0)/k;
            } else {
                v_gamma_aa_[index] = 0.0;
            } 

        }
        if (is_meta_) {

            //V_tau_a
            if (rho_a[index] > cutoff_) {
                v_tau_a_[index] = 0.0;
            } else {
                v_tau_a_[index] = 0.0;
            } 

        }
    }
    //Second Partials
    for (int index = 0; index < ntrue && deriv_ >= 2; index++) {

        //V_rho_a_rho_a
        if (rho_a[index] > cutoff_) {
            double t169119 = rho_a[index]*2.0;
            double t169120 = 1.0/(k*k);
            double t169121 = 1.0/kp;
            double t169122 = 1.0/pow(t169119,8.0/3.0);
            double t169123 = gamma_aa[index]*mu_*t169120*t169121*t169122;
            double t169124 = t169123+1.0;
            double t169125 = 1.0/t169124;
            double t169126 = kp-kp*t169125+1.0;
            double t169127 = 1.0/k;
            double t169128 = 1.0/(t169124*t169124);
            v_rho_a_rho_a_[index] = e*k*t169126*1.0/pow(t169119,2.0/3.0)*(4.0/3.0)-e*k*rho_a[index]*t169126*1.0/ \
               pow(t169119,5.0/3.0)*(8.0/9.0)-e*gamma_aa[index]*mu_*t169127*1.0/pow(t169119,1.0E1/3.0)*t169128*(3.2E1/ \
               3.0)-e*(gamma_aa[index]*gamma_aa[index])*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
               rho_a[index]*rho_a[index]*rho_a[index])*t169121*1.0/(t169124*t169124*t169124)*(4.0/9.0)+e*gamma_aa[index]* \
               mu_*rho_a[index]*t169127*1.0/pow(t169119,1.3E1/3.0)*t169128*3.2E1;
        } else {
            v_rho_a_rho_a_[index] = 0.0;
        } 

        if (is_gga_) {

            //V_rho_a_gamma_aa
            if (rho_a[index] > cutoff_) {
                double t169130 = rho_a[index]*2.0;
                double t169131 = 1.0/k;
                double t169132 = 1.0/(k*k);
                double t169133 = 1.0/kp;
                double t169134 = 1.0/pow(t169130,8.0/3.0);
                double t169135 = gamma_aa[index]*mu_*t169132*t169133*t169134;
                double t169136 = t169135+1.0;
                double t169137 = 1.0/(t169136*t169136);
                v_rho_a_gamma_aa_[index] = e*mu_*1.0/pow(t169130,7.0/3.0)*t169131*t169137-e*mu_*rho_a[index]*1.0/ \
                   pow(t169130,1.0E1/3.0)*t169131*t169137*(1.4E1/3.0)+e*gamma_aa[index]*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]* \
                   rho_a[index]*rho_a[index]*rho_a[index]*rho_a[index])*t169133*1.0/(t169136*t169136*t169136)*(1.0/6.0) \
                   ;
            } else {
                v_rho_a_gamma_aa_[index] = 0.0;
            } 

            //V_gamma_aa_gamma_aa
            if (rho_a[index] > cutoff_) {
                double t169139 = 1.0/kp;
                v_gamma_aa_gamma_aa_[index] = e*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
                   rho_a[index])*t169139*1.0/pow(gamma_aa[index]*1.0/(k*k)*mu_*t169139*1.0/pow(rho_a[index]*2.0,8.0/3.0) \
                   +1.0,3.0)*(-1.0/8.0);
            } else {
                v_gamma_aa_gamma_aa_[index] = 0.0;
            } 

        }
        if (is_meta_) {

            //V_rho_a_tau_a
            if (rho_a[index] > cutoff_) {
                v_rho_a_tau_a_[index] = 0.0;
            } else {
                v_rho_a_tau_a_[index] = 0.0;
            } 

            //V_tau_a_tau_a
            if (rho_a[index] > cutoff_) {
                v_tau_a_tau_a_[index] = 0.0;
            } else {
                v_tau_a_tau_a_[index] = 0.0;
            } 

            if (is_gga_) {

                //V_gamma_aa_tau_a
                if (rho_a[index] > cutoff_) {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } else {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } 

            }
        }
    }
}
void wPBEg_X_Functional::computeUKSFunctional(boost::shared_ptr<UKSFunctions> prop)
{
    int ntrue = prop->npoints();

    const double* restrict rho_a;
    const double* restrict rho_b;
    const double* restrict gamma_aa;
    const double* restrict gamma_ab;
    const double* restrict gamma_bb;
    const double* restrict tau_a;
    const double* restrict tau_b;

    rho_a = prop->property_value("RHO_A")->pointer();
    rho_b = prop->property_value("RHO_B")->pointer();
    if (is_gga_) {
        gamma_aa = prop->property_value("GAMMA_AA")->pointer();
        gamma_ab = prop->property_value("GAMMA_AB")->pointer();
        gamma_bb = prop->property_value("GAMMA_BB")->pointer();
    }
    if (is_meta_) {
        tau_a = prop->property_value("TAU_A")->pointer();
        tau_b = prop->property_value("TAU_B")->pointer();
    }

    double k = params_[0].second;
    double e = params_[1].second;
    double kp = params_[2].second;
    double mu_ = params_[3].second;

    //Functional
    for (int index = 0; index < ntrue; index++) {

        //Functional Value
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            double t168843 = rho_a[index]*2.0;
            double t168844 = 1.0/(k*k);
            double t168845 = 1.0/kp;
            double t168846 = rho_b[index]*2.0;
            functional_[index] = e*k*rho_a[index]*pow(t168843,1.0/3.0)*(kp-kp/(gamma_aa[index]*mu_*1.0/pow(t168843,8.0/ \
               3.0)*t168844*t168845+1.0)+1.0)+e*k*rho_b[index]*pow(t168846,1.0/3.0)*(kp-kp/(gamma_bb[index]*mu_*t168844* \
               t168845*1.0/pow(t168846,8.0/3.0)+1.0)+1.0);
        } else if (rho_a[index] > cutoff_) {
            double t169004 = rho_a[index]*2.0;
            functional_[index] = e*k*rho_a[index]*pow(t169004,1.0/3.0)*(kp-kp/((gamma_aa[index]*1.0/(k*k)*mu_* \
               1.0/pow(t169004,8.0/3.0))/kp+1.0)+1.0);
        } else if (rho_b[index] > cutoff_) {
            double t168939 = rho_b[index]*2.0;
            functional_[index] = e*k*rho_b[index]*pow(t168939,1.0/3.0)*(kp-kp/((gamma_bb[index]*1.0/(k*k)*mu_* \
               1.0/pow(t168939,8.0/3.0))/kp+1.0)+1.0);
        } else {
            functional_[index] = 0.0;
        } 

    }
    //First Partials
    for (int index = 0; index < ntrue && deriv_ >= 1; index++) {

        //V_rho_a
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            double t168848 = rho_a[index]*2.0;
            double t168849 = 1.0/(k*k);
            double t168850 = 1.0/kp;
            double t168851 = 1.0/pow(t168848,8.0/3.0);
            double t168852 = gamma_aa[index]*mu_*t168850*t168851*t168849;
            double t168853 = t168852+1.0;
            double t168854 = 1.0/t168853;
            double t168855 = kp-kp*t168854+1.0;
            v_rho_a_[index] = e*k*t168855*pow(t168848,1.0/3.0)+e*k*rho_a[index]*t168855*1.0/pow(t168848,2.0/3.0) \
               *(2.0/3.0)-(e*gamma_aa[index]*mu_*rho_a[index]*1.0/(t168853*t168853)*1.0/pow(t168848,1.0E1/3.0)*(1.6E1/ \
               3.0))/k;
        } else if (rho_a[index] > cutoff_) {
            double t169006 = rho_a[index]*2.0;
            double t169007 = 1.0/(k*k);
            double t169008 = 1.0/kp;
            double t169009 = 1.0/pow(t169006,8.0/3.0);
            double t169010 = gamma_aa[index]*mu_*t169007*t169008*t169009;
            double t169011 = t169010+1.0;
            double t169012 = 1.0/t169011;
            double t169013 = kp-kp*t169012+1.0;
            v_rho_a_[index] = e*k*t169013*pow(t169006,1.0/3.0)+e*k*rho_a[index]*t169013*1.0/pow(t169006,2.0/3.0) \
               *(2.0/3.0)-(e*gamma_aa[index]*mu_*rho_a[index]*1.0/(t169011*t169011)*1.0/pow(t169006,1.0E1/3.0)*(1.6E1/ \
               3.0))/k;
        } else if (rho_b[index] > cutoff_) {
            v_rho_a_[index] = 0.0;
        } else {
            v_rho_a_[index] = 0.0;
        } 

        //V_rho_b
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            double t168857 = rho_b[index]*2.0;
            double t168858 = 1.0/(k*k);
            double t168859 = 1.0/kp;
            double t168860 = 1.0/pow(t168857,8.0/3.0);
            double t168861 = gamma_bb[index]*mu_*t168860*t168858*t168859;
            double t168862 = t168861+1.0;
            double t168863 = 1.0/t168862;
            double t168864 = kp-kp*t168863+1.0;
            v_rho_b_[index] = e*k*t168864*pow(t168857,1.0/3.0)+e*k*rho_b[index]*t168864*1.0/pow(t168857,2.0/3.0) \
               *(2.0/3.0)-(e*gamma_bb[index]*mu_*rho_b[index]*1.0/(t168862*t168862)*1.0/pow(t168857,1.0E1/3.0)*(1.6E1/ \
               3.0))/k;
        } else if (rho_a[index] > cutoff_) {
            v_rho_b_[index] = 0.0;
        } else if (rho_b[index] > cutoff_) {
            double t168942 = rho_b[index]*2.0;
            double t168943 = 1.0/(k*k);
            double t168944 = 1.0/kp;
            double t168945 = 1.0/pow(t168942,8.0/3.0);
            double t168946 = gamma_bb[index]*mu_*t168943*t168944*t168945;
            double t168947 = t168946+1.0;
            double t168948 = 1.0/t168947;
            double t168949 = kp-kp*t168948+1.0;
            v_rho_b_[index] = e*k*pow(t168942,1.0/3.0)*t168949+e*k*rho_b[index]*1.0/pow(t168942,2.0/3.0)*t168949* \
               (2.0/3.0)-(e*gamma_bb[index]*mu_*rho_b[index]*1.0/pow(t168942,1.0E1/3.0)*1.0/(t168947*t168947)*(1.6E1/ \
               3.0))/k;
        } else {
            v_rho_b_[index] = 0.0;
        } 

        if (is_gga_) {

            //V_gamma_aa
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168866 = rho_a[index]*2.0;
                v_gamma_aa_[index] = (e*mu_*rho_a[index]*1.0/pow(t168866,7.0/3.0)*1.0/pow((gamma_aa[index]*1.0/(k* \
                   k)*mu_*1.0/pow(t168866,8.0/3.0))/kp+1.0,2.0))/k;
            } else if (rho_a[index] > cutoff_) {
                double t169016 = rho_a[index]*2.0;
                v_gamma_aa_[index] = (e*mu_*rho_a[index]*1.0/pow(t169016,7.0/3.0)*1.0/pow((gamma_aa[index]*1.0/(k* \
                   k)*mu_*1.0/pow(t169016,8.0/3.0))/kp+1.0,2.0))/k;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_aa_[index] = 0.0;
            } else {
                v_gamma_aa_[index] = 0.0;
            } 

            //V_gamma_ab
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_gamma_ab_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_ab_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_ab_[index] = 0.0;
            } else {
                v_gamma_ab_[index] = 0.0;
            } 

            //V_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168869 = rho_b[index]*2.0;
                v_gamma_bb_[index] = (e*mu_*rho_b[index]*1.0/pow(t168869,7.0/3.0)*1.0/pow((gamma_bb[index]*1.0/(k* \
                   k)*mu_*1.0/pow(t168869,8.0/3.0))/kp+1.0,2.0))/k;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                double t168953 = rho_b[index]*2.0;
                v_gamma_bb_[index] = (e*mu_*rho_b[index]*1.0/pow(t168953,7.0/3.0)*1.0/pow((gamma_bb[index]*1.0/(k* \
                   k)*mu_*1.0/pow(t168953,8.0/3.0))/kp+1.0,2.0))/k;
            } else {
                v_gamma_bb_[index] = 0.0;
            } 
        }
        if (is_meta_) {

            //V_tau_a
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_tau_a_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_tau_a_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_tau_a_[index] = 0.0;
            } else {
                v_tau_a_[index] = 0.0;
            } 

            //V_tau_a
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_tau_b_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_tau_b_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_tau_b_[index] = 0.0;
            } else {
                v_tau_b_[index] = 0.0;
            } 
        }
    }
    //Second Partials
    for (int index = 0; index < ntrue && deriv_ >= 2; index++) {

        //V_rho_a_rho_a
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            double t168873 = rho_a[index]*2.0;
            double t168874 = 1.0/(k*k);
            double t168875 = 1.0/kp;
            double t168876 = 1.0/pow(t168873,8.0/3.0);
            double t168877 = gamma_aa[index]*mu_*t168874*t168875*t168876;
            double t168878 = t168877+1.0;
            double t168879 = 1.0/t168878;
            double t168880 = kp-kp*t168879+1.0;
            double t168881 = 1.0/k;
            double t168882 = 1.0/(t168878*t168878);
            v_rho_a_rho_a_[index] = e*k*t168880*1.0/pow(t168873,2.0/3.0)*(4.0/3.0)-e*k*rho_a[index]*t168880*1.0/ \
               pow(t168873,5.0/3.0)*(8.0/9.0)-e*gamma_aa[index]*mu_*t168881*1.0/pow(t168873,1.0E1/3.0)*t168882*(3.2E1/ \
               3.0)-e*(gamma_aa[index]*gamma_aa[index])*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
               rho_a[index]*rho_a[index]*rho_a[index])*t168875*1.0/(t168878*t168878*t168878)*(4.0/9.0)+e*gamma_aa[index]* \
               mu_*rho_a[index]*t168881*1.0/pow(t168873,1.3E1/3.0)*t168882*3.2E1;
        } else if (rho_a[index] > cutoff_) {
            double t169022 = rho_a[index]*2.0;
            double t169023 = 1.0/(k*k);
            double t169024 = 1.0/kp;
            double t169025 = 1.0/pow(t169022,8.0/3.0);
            double t169026 = gamma_aa[index]*mu_*t169023*t169024*t169025;
            double t169027 = t169026+1.0;
            double t169028 = 1.0/t169027;
            double t169029 = kp-kp*t169028+1.0;
            double t169030 = 1.0/k;
            double t169031 = 1.0/(t169027*t169027);
            v_rho_a_rho_a_[index] = e*k*1.0/pow(t169022,2.0/3.0)*t169029*(4.0/3.0)-e*k*rho_a[index]*1.0/pow(t169022,5.0/ \
               3.0)*t169029*(8.0/9.0)-e*gamma_aa[index]*mu_*t169030*1.0/pow(t169022,1.0E1/3.0)*t169031*(3.2E1/3.0) \
               -e*(gamma_aa[index]*gamma_aa[index])*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
               rho_a[index]*rho_a[index]*rho_a[index])*t169024*1.0/(t169027*t169027*t169027)*(4.0/9.0)+e*gamma_aa[index]* \
               mu_*rho_a[index]*t169030*1.0/pow(t169022,1.3E1/3.0)*t169031*3.2E1;
        } else if (rho_b[index] > cutoff_) {
            v_rho_a_rho_a_[index] = 0.0;
        } else {
            v_rho_a_rho_a_[index] = 0.0;
        } 

        //V_rho_a_rho_b
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            v_rho_a_rho_b_[index] = 0.0;
        } else if (rho_a[index] > cutoff_) {
            v_rho_a_rho_b_[index] = 0.0;
        } else if (rho_b[index] > cutoff_) {
            v_rho_a_rho_b_[index] = 0.0;
        } else {
            v_rho_a_rho_b_[index] = 0.0;
        } 

        //V_rho_b_rho_b
        if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
            double t168885 = rho_b[index]*2.0;
            double t168886 = 1.0/(k*k);
            double t168887 = 1.0/kp;
            double t168888 = 1.0/pow(t168885,8.0/3.0);
            double t168889 = gamma_bb[index]*mu_*t168886*t168887*t168888;
            double t168890 = t168889+1.0;
            double t168891 = 1.0/t168890;
            double t168892 = kp-kp*t168891+1.0;
            double t168893 = 1.0/k;
            double t168894 = 1.0/(t168890*t168890);
            v_rho_b_rho_b_[index] = e*k*t168892*1.0/pow(t168885,2.0/3.0)*(4.0/3.0)-e*k*rho_b[index]*t168892*1.0/ \
               pow(t168885,5.0/3.0)*(8.0/9.0)-e*gamma_bb[index]*mu_*t168893*1.0/pow(t168885,1.0E1/3.0)*t168894*(3.2E1/ \
               3.0)-e*(gamma_bb[index]*gamma_bb[index])*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]*rho_b[index]*rho_b[index]* \
               rho_b[index]*rho_b[index]*rho_b[index])*1.0/(t168890*t168890*t168890)*t168887*(4.0/9.0)+e*gamma_bb[index]* \
               mu_*rho_b[index]*t168893*1.0/pow(t168885,1.3E1/3.0)*t168894*3.2E1;
        } else if (rho_a[index] > cutoff_) {
            v_rho_b_rho_b_[index] = 0.0;
        } else if (rho_b[index] > cutoff_) {
            double t168959 = rho_b[index]*2.0;
            double t168960 = 1.0/(k*k);
            double t168961 = 1.0/kp;
            double t168962 = 1.0/pow(t168959,8.0/3.0);
            double t168963 = gamma_bb[index]*mu_*t168960*t168961*t168962;
            double t168964 = t168963+1.0;
            double t168965 = 1.0/t168964;
            double t168966 = kp-kp*t168965+1.0;
            double t168967 = 1.0/k;
            double t168968 = 1.0/(t168964*t168964);
            v_rho_b_rho_b_[index] = e*k*t168966*1.0/pow(t168959,2.0/3.0)*(4.0/3.0)-e*k*rho_b[index]*t168966*1.0/ \
               pow(t168959,5.0/3.0)*(8.0/9.0)-e*gamma_bb[index]*mu_*t168967*1.0/pow(t168959,1.0E1/3.0)*t168968*(3.2E1/ \
               3.0)-e*(gamma_bb[index]*gamma_bb[index])*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]*rho_b[index]*rho_b[index]* \
               rho_b[index]*rho_b[index]*rho_b[index])*t168961*1.0/(t168964*t168964*t168964)*(4.0/9.0)+e*gamma_bb[index]* \
               mu_*rho_b[index]*t168967*1.0/pow(t168959,1.3E1/3.0)*t168968*3.2E1;
        } else {
            v_rho_b_rho_b_[index] = 0.0;
        } 

        if (is_gga_) {

            //V_rho_a_gamma_aa
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168896 = rho_a[index]*2.0;
                double t168897 = 1.0/k;
                double t168898 = 1.0/(k*k);
                double t168899 = 1.0/kp;
                double t168900 = 1.0/pow(t168896,8.0/3.0);
                double t168901 = gamma_aa[index]*mu_*t168900*t168898*t168899;
                double t168902 = t168901+1.0;
                double t168903 = 1.0/(t168902*t168902);
                v_rho_a_gamma_aa_[index] = e*mu_*t168903*1.0/pow(t168896,7.0/3.0)*t168897-e*mu_*rho_a[index]*t168903* \
                   1.0/pow(t168896,1.0E1/3.0)*t168897*(1.4E1/3.0)+e*gamma_aa[index]*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]* \
                   rho_a[index]*rho_a[index]*rho_a[index]*rho_a[index])*1.0/(t168902*t168902*t168902)*t168899*(1.0/6.0) \
                   ;
            } else if (rho_a[index] > cutoff_) {
                double t169035 = rho_a[index]*2.0;
                double t169036 = 1.0/k;
                double t169037 = 1.0/(k*k);
                double t169038 = 1.0/kp;
                double t169039 = 1.0/pow(t169035,8.0/3.0);
                double t169040 = gamma_aa[index]*mu_*t169037*t169038*t169039;
                double t169041 = t169040+1.0;
                double t169042 = 1.0/(t169041*t169041);
                v_rho_a_gamma_aa_[index] = e*mu_*t169042*1.0/pow(t169035,7.0/3.0)*t169036-e*mu_*rho_a[index]*t169042* \
                   1.0/pow(t169035,1.0E1/3.0)*t169036*(1.4E1/3.0)+e*gamma_aa[index]*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]* \
                   rho_a[index]*rho_a[index]*rho_a[index]*rho_a[index])*1.0/(t169041*t169041*t169041)*t169038*(1.0/6.0) \
                   ;
            } else if (rho_b[index] > cutoff_) {
                v_rho_a_gamma_aa_[index] = 0.0;
            } else {
                v_rho_a_gamma_aa_[index] = 0.0;
            } 

            //V_rho_a_gamma_ab
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_a_gamma_ab_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_a_gamma_ab_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_a_gamma_ab_[index] = 0.0;
            } else {
                v_rho_a_gamma_ab_[index] = 0.0;
            } 

            //V_rho_a_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_a_gamma_bb_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_a_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_a_gamma_bb_[index] = 0.0;
            } else {
                v_rho_a_gamma_bb_[index] = 0.0;
            } 

            //V_rho_b_gamma_aa
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_b_gamma_aa_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_b_gamma_aa_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_b_gamma_aa_[index] = 0.0;
            } else {
                v_rho_b_gamma_aa_[index] = 0.0;
            } 

            //V_rho_b_gamma_ab
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_b_gamma_ab_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_b_gamma_ab_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_b_gamma_ab_[index] = 0.0;
            } else {
                v_rho_b_gamma_ab_[index] = 0.0;
            } 

            //V_rho_b_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168909 = rho_b[index]*2.0;
                double t168910 = 1.0/k;
                double t168911 = 1.0/(k*k);
                double t168912 = 1.0/kp;
                double t168913 = 1.0/pow(t168909,8.0/3.0);
                double t168914 = gamma_bb[index]*mu_*t168911*t168912*t168913;
                double t168915 = t168914+1.0;
                double t168916 = 1.0/(t168915*t168915);
                v_rho_b_gamma_bb_[index] = e*mu_*t168910*t168916*1.0/pow(t168909,7.0/3.0)-e*mu_*rho_b[index]*t168910* \
                   t168916*1.0/pow(t168909,1.0E1/3.0)*(1.4E1/3.0)+e*gamma_bb[index]*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]* \
                   rho_b[index]*rho_b[index]*rho_b[index]*rho_b[index])*t168912*1.0/(t168915*t168915*t168915)*(1.0/6.0) \
                   ;
            } else if (rho_a[index] > cutoff_) {
                v_rho_b_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                double t168975 = rho_b[index]*2.0;
                double t168976 = 1.0/k;
                double t168977 = 1.0/(k*k);
                double t168978 = 1.0/kp;
                double t168979 = 1.0/pow(t168975,8.0/3.0);
                double t168980 = gamma_bb[index]*mu_*t168977*t168978*t168979;
                double t168981 = t168980+1.0;
                double t168982 = 1.0/(t168981*t168981);
                v_rho_b_gamma_bb_[index] = e*mu_*t168982*1.0/pow(t168975,7.0/3.0)*t168976-e*mu_*rho_b[index]*t168982* \
                   1.0/pow(t168975,1.0E1/3.0)*t168976*(1.4E1/3.0)+e*gamma_bb[index]*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]* \
                   rho_b[index]*rho_b[index]*rho_b[index]*rho_b[index])*1.0/(t168981*t168981*t168981)*t168978*(1.0/6.0) \
                   ;
            } else {
                v_rho_b_gamma_bb_[index] = 0.0;
            } 

            //V_gamma_aa_gamma_aa
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168918 = 1.0/kp;
                v_gamma_aa_gamma_aa_[index] = e*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
                   rho_a[index])*t168918*1.0/pow(gamma_aa[index]*1.0/(k*k)*mu_*t168918*1.0/pow(rho_a[index]*2.0,8.0/3.0) \
                   +1.0,3.0)*(-1.0/1.6E1);
            } else if (rho_a[index] > cutoff_) {
                double t169049 = 1.0/kp;
                v_gamma_aa_gamma_aa_[index] = e*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_a[index]*rho_a[index]*rho_a[index]* \
                   rho_a[index])*t169049*1.0/pow(gamma_aa[index]*1.0/(k*k)*mu_*t169049*1.0/pow(rho_a[index]*2.0,8.0/3.0) \
                   +1.0,3.0)*(-1.0/1.6E1);
            } else if (rho_b[index] > cutoff_) {
                v_gamma_aa_gamma_aa_[index] = 0.0;
            } else {
                v_gamma_aa_gamma_aa_[index] = 0.0;
            } 

            //V_gamma_aa_gamma_ab
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_gamma_aa_gamma_ab_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_aa_gamma_ab_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_aa_gamma_ab_[index] = 0.0;
            } else {
                v_gamma_aa_gamma_ab_[index] = 0.0;
            } 

            //V_gamma_aa_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_gamma_aa_gamma_bb_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_aa_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_aa_gamma_bb_[index] = 0.0;
            } else {
                v_gamma_aa_gamma_bb_[index] = 0.0;
            } 

            //V_gamma_ab_gamma_ab
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_gamma_ab_gamma_ab_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_ab_gamma_ab_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_ab_gamma_ab_[index] = 0.0;
            } else {
                v_gamma_ab_gamma_ab_[index] = 0.0;
            } 

            //V_gamma_ab_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_gamma_ab_gamma_bb_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_gamma_ab_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_gamma_ab_gamma_bb_[index] = 0.0;
            } else {
                v_gamma_ab_gamma_bb_[index] = 0.0;
            } 

            //V_gamma_bb_gamma_bb
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                double t168924 = 1.0/kp;
                v_gamma_bb_gamma_bb_[index] = e*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]*rho_b[index]*rho_b[index]* \
                   rho_b[index])*t168924*1.0/pow(gamma_bb[index]*1.0/(k*k)*mu_*t168924*1.0/pow(rho_b[index]*2.0,8.0/3.0) \
                   +1.0,3.0)*(-1.0/1.6E1);
            } else if (rho_a[index] > cutoff_) {
                v_gamma_bb_gamma_bb_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                double t168989 = 1.0/kp;
                v_gamma_bb_gamma_bb_[index] = e*1.0/(k*k*k)*(mu_*mu_)*1.0/(rho_b[index]*rho_b[index]*rho_b[index]* \
                   rho_b[index])*t168989*1.0/pow(gamma_bb[index]*1.0/(k*k)*mu_*t168989*1.0/pow(rho_b[index]*2.0,8.0/3.0) \
                   +1.0,3.0)*(-1.0/1.6E1);
            } else {
                v_gamma_bb_gamma_bb_[index] = 0.0;
            } 

        }
        if (is_meta_) {

            //V_rho_a_tau_a
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_a_tau_a_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_a_tau_a_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_a_tau_a_[index] = 0.0;
            } else {
                v_rho_a_tau_a_[index] = 0.0;
            } 

            //V_rho_a_tau_b
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_a_tau_b_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_a_tau_b_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_a_tau_b_[index] = 0.0;
            } else {
                v_rho_a_tau_b_[index] = 0.0;
            } 

            //V_rho_b_tau_a
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_b_tau_a_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_b_tau_a_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_b_tau_a_[index] = 0.0;
            } else {
                v_rho_b_tau_a_[index] = 0.0;
            } 

            //V_rho_b_tau_b
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_rho_b_tau_b_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_rho_b_tau_b_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_rho_b_tau_b_[index] = 0.0;
            } else {
                v_rho_b_tau_b_[index] = 0.0;
            } 

            //V_tau_a_tau_a
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_tau_a_tau_a_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_tau_a_tau_a_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_tau_a_tau_a_[index] = 0.0;
            } else {
                v_tau_a_tau_a_[index] = 0.0;
            } 

            //V_tau_a_tau_b
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_tau_a_tau_b_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_tau_a_tau_b_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_tau_a_tau_b_[index] = 0.0;
            } else {
                v_tau_a_tau_b_[index] = 0.0;
            } 

            //V_tau_b_tau_b
            if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                v_tau_b_tau_b_[index] = 0.0;
            } else if (rho_a[index] > cutoff_) {
                v_tau_b_tau_b_[index] = 0.0;
            } else if (rho_b[index] > cutoff_) {
                v_tau_b_tau_b_[index] = 0.0;
            } else {
                v_tau_b_tau_b_[index] = 0.0;
            } 

            if (is_gga_) {

                //V_gamma_aa_tau_a
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } else {
                    v_gamma_aa_tau_a_[index] = 0.0;
                } 

                //V_gamma_aa_tau_b
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_aa_tau_b_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_aa_tau_b_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_aa_tau_b_[index] = 0.0;
                } else {
                    v_gamma_aa_tau_b_[index] = 0.0;
                } 

                //V_gamma_ab_tau_a
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_ab_tau_a_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_ab_tau_a_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_ab_tau_a_[index] = 0.0;
                } else {
                    v_gamma_ab_tau_a_[index] = 0.0;
                } 

                //V_gamma_ab_tau_b
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_ab_tau_b_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_ab_tau_b_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_ab_tau_b_[index] = 0.0;
                } else {
                    v_gamma_ab_tau_b_[index] = 0.0;
                } 

                //V_gamma_bb_tau_a
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_bb_tau_a_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_bb_tau_a_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_bb_tau_a_[index] = 0.0;
                } else {
                    v_gamma_bb_tau_a_[index] = 0.0;
                } 

                //V_gamma_bb_tau_b
                if (rho_a[index] > cutoff_ && rho_b[index] > cutoff_) {
                    v_gamma_bb_tau_b_[index] = 0.0;
                } else if (rho_a[index] > cutoff_) {
                    v_gamma_bb_tau_b_[index] = 0.0;
                } else if (rho_b[index] > cutoff_) {
                    v_gamma_bb_tau_b_[index] = 0.0;
                } else {
                    v_gamma_bb_tau_b_[index] = 0.0;
                } 

            }
        }
    }
}

}}
