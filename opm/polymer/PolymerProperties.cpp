/*
  Copyright 2012 SINTEF ICT, Applied Mathematics.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/polymer/PolymerProperties.hpp>
#include <cmath>
#include <vector>
#include <opm/core/utility/linearInterpolation.hpp>

namespace Opm
{
    double PolymerProperties::cMax() const
    {
        return c_max_;
    }

    double PolymerProperties::mixParam() const
    {
        return mix_param_;
    }

    double PolymerProperties::rockDensity() const
    {
        return rock_density_;
    }

    double PolymerProperties::deadPoreVol() const
    {
        return dead_pore_vol_;
    }

    double PolymerProperties::resFactor() const
    {
        return res_factor_;
    }

    double PolymerProperties::cMaxAds() const
    {
        return c_max_ads_;
    }

    int PolymerProperties::adsIndex() const
    {
        return ads_index_;
    }

    double PolymerProperties::viscMult(double c) const
    {
        return Opm::linearInterpolation(c_vals_visc_, visc_mult_vals_, c);
    }

    double PolymerProperties::viscMultWithDer(double c, double* der) const
    {
        *der = Opm::linearInterpolationDerivative(c_vals_visc_, visc_mult_vals_, c);
        return Opm::linearInterpolation(c_vals_visc_, visc_mult_vals_, c);
    }

    void PolymerProperties::simpleAdsorption(double c, double& c_ads) const
    {
        double dummy;
        simpleAdsorptionBoth(c, c_ads, dummy, false);
    }

    void PolymerProperties::simpleAdsorptionWithDer(double c, double& c_ads, 
                                                    double& dc_ads_dc) const
    {
        simpleAdsorptionBoth(c, c_ads, dc_ads_dc, true);
    }

    void PolymerProperties::simpleAdsorptionBoth(double c, double& c_ads, 
                                                 double& dc_ads_dc, bool if_with_der) const
    {
        c_ads = Opm::linearInterpolation(c_vals_ads_, ads_vals_, c);;
        if (if_with_der) {
            dc_ads_dc = Opm::linearInterpolationDerivative(c_vals_ads_, ads_vals_, c);
        } else {
            dc_ads_dc = 0.;
        }
    }

    void PolymerProperties::adsorption(double c, double cmax, double& c_ads) const
    {
        double dummy;
        adsorptionBoth(c, cmax, c_ads, dummy, false);
    }

    void PolymerProperties::adsorptionWithDer(double c, double cmax, 
                                              double& c_ads, double& dc_ads_dc) const
    {
        adsorptionBoth(c, cmax, c_ads, dc_ads_dc, true);
    }

    void PolymerProperties::adsorptionBoth(double c, double cmax, 
                                           double& c_ads, double& dc_ads_dc, 
                                           bool if_with_der) const
    {
        if (ads_index_ == Desorption) {
            simpleAdsorptionBoth(c, c_ads, dc_ads_dc, if_with_der);
        } else if (ads_index_ == NoDesorption) {
            if (c < cmax) {
                simpleAdsorption(cmax, c_ads);
                dc_ads_dc = 0.;
            } else {
                simpleAdsorptionBoth(c, c_ads, dc_ads_dc, if_with_der);
            }
        } else {
            THROW("Invalid Adsoption index");
        }
    }


    void PolymerProperties::effectiveVisc(const double c, const double* visc, double* visc_eff) const {
        effectiveViscBoth(c, visc, visc_eff, 0, false);
    }

    void PolymerProperties::effectiveInvVisc(const double c, const double* visc, double* inv_visc_eff) const
    {
        effectiveViscBoth(c, visc, inv_visc_eff, 0, false);
        inv_visc_eff[0] = 1./inv_visc_eff[0];
        inv_visc_eff[1] = 1./inv_visc_eff[1];
    }

    void PolymerProperties::effectiveViscWithDer(const double c, const double* visc, double* visc_eff,
                                                 double* dvisc_eff_dc) const {
        effectiveViscBoth(c, visc, visc_eff, dvisc_eff_dc, true);
    }

    void PolymerProperties::effectiveViscBoth(const double c, const double* visc, double* visc_eff,
                                              double* dvisc_eff_dc, bool if_with_der) const
    {
        double cbar = c/c_max_;
        double mu_w = visc[0];
        double mu_m;
        double omega = mix_param_;
        double mu_m_dc;
        if (if_with_der) {
            mu_m = viscMultWithDer(c, &mu_m_dc)*mu_w;
            mu_m_dc *= mu_w;
        } else { 
            mu_m = viscMult(c)*mu_w;
        }
        double mu_p = viscMult(c_max_)*mu_w;
        double mu_m_omega = std::pow(mu_m, mix_param_);
        double mu_w_e   = mu_m_omega*std::pow(mu_w, 1.0 - mix_param_);
        double mu_p_eff = mu_m_omega*std::pow(mu_p, 1.0 - mix_param_);
        double mu_w_eff = 1./((1.0 - cbar)/mu_w_e + cbar/mu_p_eff);
        visc_eff[0] = mu_w_eff;
        visc_eff[1] = visc[1];
        if (if_with_der) {
            double mu_w_e_dc = omega*mu_m_dc*std::pow(mu_m, omega - 1)*std::pow(mu_w, 1 - omega);
            double mu_p_eff_dc = omega*mu_m_dc*std::pow(mu_m, omega - 1)*std::pow(mu_p, 1 - omega);
            dvisc_eff_dc[0] = -1./c_max_*mu_w_eff*mu_w_eff*(1./mu_p_eff - 1./mu_w_e) + (1-cbar)*(mu_w_eff*mu_w_eff/(mu_w_e*mu_w_e))*mu_w_e_dc + cbar*(mu_w_eff*mu_w_eff/(mu_p_eff*mu_p_eff))*mu_p_eff_dc;
            dvisc_eff_dc[1] = 0.;
        } else {
            dvisc_eff_dc = 0;
        }
    }
        
    void PolymerProperties::effectiveRelperm(const double c,
                                             const double cmax,
                                             const double* relperm,
                                             double& eff_relperm_wat) const {
        double dummy;
        effectiveRelpermBoth(c, cmax, relperm, 0, eff_relperm_wat,
                             dummy, dummy, false);
    }

    void PolymerProperties::effectiveRelpermWithDer (const double c,
                                                     const double cmax,
                                                     const double* relperm,
                                                     const double* drelperm_ds,
                                                     double& eff_relperm_wat,
                                                     double& deff_relperm_wat_ds,
                                                     double& deff_relperm_wat_dc) const {
         effectiveRelpermBoth(c, cmax, relperm,
                              drelperm_ds, eff_relperm_wat,
                              deff_relperm_wat_ds, deff_relperm_wat_dc,
                              true);
    }

    void PolymerProperties::effectiveRelpermBoth(const double c,
                                                 const double cmax,
                                                 const double* relperm,
                                                 const double* drelperm_ds,
                                                 double& eff_relperm_wat,
                                                 double& deff_relperm_wat_ds,
                                                 double& deff_relperm_wat_dc,
                                                 bool if_with_der) const {
        double c_ads;
        double dc_ads_dc;
        adsorptionBoth(c, cmax, c_ads, dc_ads_dc, if_with_der);
        double rk = 1 + (res_factor_ - 1)*c_ads/c_max_ads_;
        eff_relperm_wat = relperm[0]/rk;
        if (if_with_der) {
            deff_relperm_wat_ds = drelperm_ds[0]/rk;
            deff_relperm_wat_dc = dc_ads_dc*relperm[0]/(rk*rk*c_max_ads_);
        } else {
            deff_relperm_wat_ds = -1.0;
            deff_relperm_wat_dc = -1.0;
        }
    }

    void PolymerProperties::effectiveMobilities(const double c,
                                                const double cmax,
                                                const double* visc,
                                                const double* relperm,
                                                std::vector<double>& mob) const 
    { 
        double dummy1;
        double dummy2[2];
        std::vector<double> dummy3;
        effectiveMobilitiesBoth(c, cmax, visc, relperm,
                                dummy2, mob, dummy3, dummy1, false);
    }

    void PolymerProperties::effectiveMobilitiesWithDer(const double c,
                                                       const double cmax,
                                                       const double* visc,
                                                       const double* relperm,
                                                       const double* drelpermds,
                                                       std::vector<double>& mob, 
                                                       std::vector<double>& dmobds,
                                                       double& dmobwatdc) const 
    {
        effectiveMobilitiesBoth(c, cmax, visc,
                                relperm, drelpermds, mob,dmobds,
                                dmobwatdc, true);
    }

    void PolymerProperties::effectiveMobilitiesBoth(const double c,
                                                    const double cmax,
                                                    const double* visc,
                                                    const double* relperm,
                                                    const double* drelperm_ds,
                                                    std::vector<double>& mob, 
                                                    std::vector<double>& dmob_ds,
                                                    double& dmobwat_dc,
                                                    bool if_with_der) const 
    {
        double visc_eff[2];
        double dvisc_eff_dc[2];
        effectiveViscBoth(c, visc, visc_eff, dvisc_eff_dc, if_with_der);
        double mu_w_eff = visc_eff[0];
        double mu_w_eff_dc = dvisc_eff_dc[0];
        double eff_relperm_wat;
        double deff_relperm_wat_ds;
        double deff_relperm_wat_dc;
        effectiveRelpermBoth(c, cmax, relperm,
                             drelperm_ds, eff_relperm_wat,
                             deff_relperm_wat_ds, deff_relperm_wat_dc,
                             if_with_der);
        mob[0] = eff_relperm_wat/visc_eff[0];
        mob[1] = relperm[1]/visc_eff[1];
            
        if (if_with_der) {
            dmobwat_dc = - mob[0]*mu_w_eff_dc/(mu_w_eff*mu_w_eff) 
                + deff_relperm_wat_dc/mu_w_eff;
            dmob_ds[0*2 + 0] = deff_relperm_wat_ds/visc_eff[0];
            dmob_ds[0*2 + 1] = -dmob_ds[0*2 + 0];
            dmob_ds[1*2 + 0] = drelperm_ds[1*2 + 0]/visc_eff[1];
            dmob_ds[1*2 + 1] = drelperm_ds[1*2 + 1]/visc_eff[1];
        } else {
            mob.clear();
            dmob_ds.clear();
        }
    }
        
    void PolymerProperties::computeMcWithDer(const double& c, double& mc) const 
    {
        double dummy;
        computeMcBoth(c, mc, dummy, false);
    }

    void PolymerProperties::computeMcWithDer(const double& c, double& mc,
                                          double& dmc_dc) const 
    {
        computeMcBoth(c, mc, dmc_dc, true);
    }

    void PolymerProperties::computeMcBoth(const double& c, double& mc,
                                          double& dmc_dc, bool if_with_der) const 
    {
        double cbar = c/c_max_;
        double omega = mix_param_;
        double r = std::pow(viscMult(c_max_), 1 - omega); // viscMult(c_max_)=mu_p/mu_w
        mc = c/(cbar + (1 - cbar)*r);
        if (if_with_der) {
            dmc_dc = r/std::pow(cbar + (1 - cbar)*r, 2);
        } else {
            dmc_dc = 0.;
        }
    }
}
