﻿/*!
 * \file flow_sources.cpp
 * \brief Implementation of numerics classes for integration
 *        of source terms in fluid flow problems.
 * \author F. Palacios, T. Economon
 * \version 7.2.1 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2021, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../../include/numerics/flow/flow_sources.hpp"
#include "../../../../Common/include/toolboxes/geometry_toolbox.hpp"

CSourceBase_Flow::CSourceBase_Flow(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                                   CNumerics(val_nDim, val_nVar, config) {
  residual = new su2double [nVar]();
  jacobian = new su2double* [nVar];
  for(unsigned short iVar = 0; iVar < nVar; ++iVar)
    jacobian[iVar] = new su2double [nVar]();
}

CSourceBase_Flow::~CSourceBase_Flow() {
  delete [] residual;
  if(jacobian) {
    for(unsigned short iVar = 0; iVar < nVar; ++iVar)
      delete [] jacobian[iVar];
    delete [] jacobian;
  }
}

CSourceAxisymmetric_Flow::CSourceAxisymmetric_Flow(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                          CSourceBase_Flow(val_nDim, val_nVar, config) {

  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  viscous = config->GetViscous();
  rans = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);

}

CNumerics::ResidualType<> CSourceAxisymmetric_Flow::ComputeResidual(const CConfig* config) {

  su2double Pressure_i, Enthalpy_i, Velocity_i, sq_vel;
  unsigned short iDim, iVar, jVar;

  if (Coord_i[1] > EPS) {

    yinv = 1.0/Coord_i[1];

    sq_vel = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) {
      Velocity_i = U_i[iDim+1] / U_i[0];
      sq_vel += Velocity_i *Velocity_i;
    }

    Pressure_i = (Gamma-1.0)*U_i[0]*(U_i[nDim+1]/U_i[0]-0.5*sq_vel);
    Enthalpy_i = (U_i[nDim+1] + Pressure_i) / U_i[0];

    residual[0] = yinv*Volume*U_i[2];
    residual[1] = yinv*Volume*U_i[1]*U_i[2]/U_i[0];
    residual[2] = yinv*Volume*(U_i[2]*U_i[2]/U_i[0]);
    residual[3] = yinv*Volume*Enthalpy_i*U_i[2];

    /*--- Inviscid component of the source term. ---*/

    if (implicit) {
      jacobian[0][0] = 0.0;
      jacobian[0][1] = 0.0;
      jacobian[0][2] = 1.0;
      jacobian[0][3] = 0.0;

      jacobian[1][0] = -U_i[1]*U_i[2]/(U_i[0]*U_i[0]);
      jacobian[1][1] = U_i[2]/U_i[0];
      jacobian[1][2] = U_i[1]/U_i[0];
      jacobian[1][3] = 0.0;

      jacobian[2][0] = -U_i[2]*U_i[2]/(U_i[0]*U_i[0]);
      jacobian[2][1] = 0.0;
      jacobian[2][2] = 2*U_i[2]/U_i[0];
      jacobian[2][3] = 0.0;

      jacobian[3][0] = -Gamma*U_i[2]*U_i[3]/(U_i[0]*U_i[0]) + (Gamma-1)*U_i[2]*(U_i[1]*U_i[1]+U_i[2]*U_i[2])/(U_i[0]*U_i[0]*U_i[0]);
      jacobian[3][1] = -(Gamma-1)*U_i[2]*U_i[1]/(U_i[0]*U_i[0]);
      jacobian[3][2] = Gamma*U_i[3]/U_i[0] - 1/2*(Gamma-1)*( (U_i[1]*U_i[1]+U_i[2]*U_i[2])/(U_i[0]*U_i[0]) + 2*U_i[2]*U_i[2]/(U_i[0]*U_i[0]) );
      jacobian[3][3] = Gamma*U_i[2]/U_i[0];

      for (iVar=0; iVar < nVar; iVar++)
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] *= yinv*Volume;

    }

    /*--- Add the viscous terms if necessary. ---*/

    if (viscous) ResidualDiffusion();

  }

  else {

    for (iVar=0; iVar < nVar; iVar++)
      residual[iVar] = 0.0;

    if (implicit) {
      for (iVar=0; iVar < nVar; iVar++) {
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] = 0.0;
      }
    }

  }

  return ResidualType<>(residual, jacobian, nullptr);
}

void CSourceAxisymmetric_Flow::ResidualDiffusion(){

  if (!rans){ turb_ke_i = 0.0; }

  su2double laminar_viscosity_i    = V_i[nDim+5];
  su2double eddy_viscosity_i       = V_i[nDim+6];
  su2double thermal_conductivity_i = V_i[nDim+7];
  su2double heat_capacity_cp_i     = V_i[nDim+8];

  su2double total_viscosity_i = laminar_viscosity_i + eddy_viscosity_i;
  su2double total_conductivity_i = thermal_conductivity_i + heat_capacity_cp_i*eddy_viscosity_i/Prandtl_Turb;

  su2double u = U_i[1]/U_i[0];
  su2double v = U_i[2]/U_i[0];

  residual[0] -= 0.0;
  residual[1] -= Volume*(yinv*total_viscosity_i*(PrimVar_Grad_i[1][1]+PrimVar_Grad_i[2][0])
                         -TWO3*AuxVar_Grad_i[0][0]);
  residual[2] -= Volume*(yinv*total_viscosity_i*2*(PrimVar_Grad_i[2][1]-v*yinv)
                         -TWO3*AuxVar_Grad_i[0][1]);
  residual[3] -= Volume*(yinv*(total_viscosity_i*(u*(PrimVar_Grad_i[2][0]+PrimVar_Grad_i[1][1])
                                                 +v*TWO3*(2*PrimVar_Grad_i[2][1]-PrimVar_Grad_i[1][0]
                                                 -v*yinv+U_i[0]*turb_ke_i))
                                                 +total_conductivity_i*PrimVar_Grad_i[0][1])
                                                 -TWO3*(AuxVar_Grad_i[1][1]+AuxVar_Grad_i[2][0]));
}


CNumerics::ResidualType<> CSourceGeneralAxisymmetric_Flow::ComputeResidual(const CConfig* config) {
  unsigned short iVar, jVar;

  if (Coord_i[1] > EPS) {

    yinv = 1.0/Coord_i[1];

    su2double Density_i = U_i[0];
    su2double Velocity1_i = U_i[1]/U_i[0];
    su2double Velocity2_i = U_i[2]/U_i[0];
    su2double Energy_i = U_i[3]/U_i[0];

    su2double Pressure_i = V_j[3];
    su2double Enthalpy_i = Energy_i + Pressure_i/Density_i;

    /*--- Inviscid component of the source term. ---*/

    residual[0] = yinv*Volume*U_i[2];
    residual[1] = yinv*Volume*U_i[1]*Velocity2_i;
    residual[2] = yinv*Volume*U_i[2]*Velocity2_i;
    residual[3] = yinv*Volume*U_i[2]*Enthalpy_i;

    if (implicit) {

      su2double dPdrho_e_i = S_i[0];
      su2double dPde_rho_i = S_i[1];

      jacobian[0][0] = 0.0;
      jacobian[0][1] = 0.0;
      jacobian[0][2] = 1.0;
      jacobian[0][3] = 0.0;

      jacobian[1][0] = -Velocity1_i*Velocity2_i;
      jacobian[1][1] = Velocity2_i;
      jacobian[1][2] = Velocity1_i;
      jacobian[1][3] = 0.0;

      jacobian[2][0] = -Velocity2_i*Velocity2_i;
      jacobian[2][1] = 0.0;
      jacobian[2][2] = 2*Velocity2_i;
      jacobian[2][3] = 0.0;

      jacobian[3][0] = Velocity2_i*(dPdrho_e_i + dPde_rho_i/Density_i*(Velocity1_i*Velocity1_i
                                                                        + Velocity2_i*Velocity2_i
                                                                        - Energy_i) - Enthalpy_i);
      jacobian[3][1] = -Velocity1_i*Velocity2_i/Density_i *dPde_rho_i;
      jacobian[3][2] = Enthalpy_i - Velocity2_i*Velocity2_i/Density_i *dPde_rho_i;
      jacobian[3][3] = Velocity2_i + Velocity2_i/Density_i *dPde_rho_i;

      for (iVar=0; iVar < nVar; iVar++)
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] *= yinv*Volume;

    }

    /*--- Add the viscous terms if necessary. ---*/

    if (viscous) ResidualDiffusion();

  }

  else {

    for (iVar=0; iVar < nVar; iVar++)
      residual[iVar] = 0.0;

    if (implicit) {
      for (iVar=0; iVar < nVar; iVar++) {
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] = 0.0;
      }
    }

  }

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceIncAxisymmetric_Flow::CSourceIncAxisymmetric_Flow(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                             CSourceBase_Flow(val_nDim, val_nVar, config) {

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  energy   = config->GetEnergy_Equation();
  viscous  = config->GetViscous();

}

CNumerics::ResidualType<> CSourceIncAxisymmetric_Flow::ComputeResidual(const CConfig* config) {

  su2double yinv, Velocity_i[3];
  unsigned short iDim, iVar, jVar;

  if (Coord_i[1] > EPS) {

    yinv = 1.0/Coord_i[1];

    /*--- Set primitive variables at points iPoint. ---*/

    Pressure_i    = V_i[0];
    Temp_i        = V_i[nDim+1];
    DensityInc_i  = V_i[nDim+2];
    BetaInc2_i    = V_i[nDim+3];
    Cp_i          = V_i[nDim+7];
    Enthalpy_i    = Cp_i*Temp_i;

    for (iDim = 0; iDim < nDim; iDim++)
      Velocity_i[iDim] = V_i[iDim+1];

    /*--- Inviscid component of the source term. ---*/

    residual[0] = yinv*Volume*DensityInc_i*Velocity_i[1];
    residual[1] = yinv*Volume*DensityInc_i*Velocity_i[0]*Velocity_i[1];
    residual[2] = yinv*Volume*DensityInc_i*Velocity_i[1]*Velocity_i[1];
    residual[3] = yinv*Volume*DensityInc_i*Enthalpy_i*Velocity_i[1];

    if (implicit) {

      jacobian[0][0] = 0.0;
      jacobian[0][1] = 0.0;
      jacobian[0][2] = 1.0;
      jacobian[0][3] = 0.0;

      jacobian[1][0] = 0.0;
      jacobian[1][1] = Velocity_i[1];
      jacobian[1][2] = Velocity_i[0];
      jacobian[1][3] = 0.0;

      jacobian[2][0] = 0.0;
      jacobian[2][1] = 0.0;
      jacobian[2][2] = 2.0*Velocity_i[1];
      jacobian[2][3] = 0.0;

      jacobian[3][0] = 0.0;
      jacobian[3][1] = 0.0;
      jacobian[3][2] = Enthalpy_i;
      jacobian[3][3] = Cp_i*Velocity_i[1];

      for (iVar=0; iVar < nVar; iVar++)
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] *= yinv*Volume*DensityInc_i;

    }

    /*--- Add the viscous terms if necessary. ---*/

    if (viscous) {

      Laminar_Viscosity_i    = V_i[nDim+4];
      Eddy_Viscosity_i       = V_i[nDim+5];
      Thermal_Conductivity_i = V_i[nDim+6];

      su2double total_viscosity;

      total_viscosity = (Laminar_Viscosity_i + Eddy_Viscosity_i);

      /*--- The full stress tensor is needed for variable density ---*/
      ComputeStressTensor(nDim, tau, PrimVar_Grad_i+1, total_viscosity);

      /*--- Viscous terms. ---*/

      residual[0] -= 0.0;
      residual[1] -= Volume*(yinv*tau[0][1] - TWO3*AuxVar_Grad_i[0][0]);
      residual[2] -= Volume*(yinv*2.0*total_viscosity*PrimVar_Grad_i[2][1] -
                             yinv* yinv*2.0*total_viscosity*Velocity_i[1] -
                             TWO3*AuxVar_Grad_i[0][1]);
      residual[3] -= Volume*yinv*Thermal_Conductivity_i*PrimVar_Grad_i[nDim+1][1];

    }

  } else {

    for (iVar=0; iVar < nVar; iVar++)
      residual[iVar] = 0.0;

    if (implicit) {
      for (iVar=0; iVar < nVar; iVar++) {
        for (jVar=0; jVar < nVar; jVar++)
          jacobian[iVar][jVar] = 0.0;
      }
    }

  }

  if (!energy) {
    residual[nDim+1] = 0.0;
    if (implicit) {
      for (iVar = 0; iVar < nVar; iVar++) {
        jacobian[iVar][nDim+1] = 0.0;
        jacobian[nDim+1][iVar] = 0.0;
      }
    }
  }

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceBodyForce::CSourceBodyForce(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                  CSourceBase_Flow(val_nDim, val_nVar, config) {

  /*--- Store the pointer to the constant body force vector. ---*/

  for (unsigned short iDim = 0; iDim < nDim; iDim++)
    Body_Force_Vector[iDim] = config->GetBody_Force_Vector()[iDim];

}

CNumerics::ResidualType<> CSourceBodyForce::ComputeResidual(const CConfig* config) {

  unsigned short iDim;
  su2double Force_Ref = config->GetForce_Ref();

  /*--- Zero the continuity contribution ---*/

  residual[0] = 0.0;

  /*--- Momentum contribution ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    residual[iDim+1] = -Volume * U_i[0] * Body_Force_Vector[iDim] / Force_Ref;

  /*--- Energy contribution ---*/

  residual[nDim+1] = 0.0;
  for (iDim = 0; iDim < nDim; iDim++)
    residual[nDim+1] += -Volume * U_i[iDim+1] * Body_Force_Vector[iDim] / Force_Ref;

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceIncBodyForce::CSourceIncBodyForce(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                     CSourceBase_Flow(val_nDim, val_nVar, config) {

  /*--- Store the pointer to the constant body force vector. ---*/

  for (unsigned short iDim = 0; iDim < nDim; iDim++)
    Body_Force_Vector[iDim] = config->GetBody_Force_Vector()[iDim];

}

CNumerics::ResidualType<> CSourceIncBodyForce::ComputeResidual(const CConfig* config) {

  unsigned short iDim;
  su2double DensityInc_0 = 0.0;
  su2double Force_Ref    = config->GetForce_Ref();
  bool variable_density  = (config->GetKind_DensityModel() == INC_DENSITYMODEL::VARIABLE);

  /*--- Check for variable density. If we have a variable density
   problem, we should subtract out the hydrostatic pressure component. ---*/

  if (variable_density) DensityInc_0 = config->GetDensity_FreeStreamND();

  /*--- Zero the continuity contribution ---*/

  residual[0] = 0.0;

  /*--- Momentum contribution. Note that this form assumes we have
   subtracted the operating density * gravity, i.e., removed the
   hydrostatic pressure component (important for pressure BCs). ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    residual[iDim+1] = -Volume * (DensityInc_i - DensityInc_0) * Body_Force_Vector[iDim] / Force_Ref;

  /*--- Zero the temperature contribution ---*/

  residual[nDim+1] = 0.0;

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceBoussinesq::CSourceBoussinesq(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                   CSourceBase_Flow(val_nDim, val_nVar, config) {

  /*--- Store the pointer to the constant body force vector. ---*/

  for (unsigned short iDim = 0; iDim < nDim; iDim++)
    Gravity_Vector[iDim] = 0.0;

  /*--- Gravity is downward in y-dir for 2D and downward z-dir for 3D. ---*/

  Gravity_Vector[nDim-1] = -STANDARD_GRAVITY;

}

CNumerics::ResidualType<> CSourceBoussinesq::ComputeResidual(const CConfig* config) {

  unsigned short iDim;
  su2double Force_Ref = config->GetForce_Ref();
  su2double T0        = config->GetTemperature_FreeStreamND();
  su2double Beta      = config->GetThermal_Expansion_CoeffND();

  /*--- Zero the continuity contribution ---*/

  residual[0] = 0.0;

  /*--- Momentum contribution. Note that this form assumes we have
   subtracted the operating density * gravity, i.e., removed the
   hydrostatic pressure component (important for pressure BCs). ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    residual[iDim+1] = Volume * DensityInc_i * ( Beta * (U_i[nDim+1] - T0)) * Gravity_Vector[iDim] / Force_Ref;

  /*--- Zero the energy contribution ---*/

  residual[nDim+1] = 0.0;

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceGravity::CSourceGravity(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                CSourceBase_Flow(val_nDim, val_nVar, config) {
                  Force_Ref = config->GetForce_Ref();
                }

CNumerics::ResidualType<> CSourceGravity::ComputeResidual(const CConfig* config) {

  unsigned short iVar;

  for (iVar = 0; iVar < nVar; iVar++)
    residual[iVar] = 0.0;

  /*--- Evaluate the source term  ---*/
  residual[nDim] = Volume * U_i[0] * STANDARD_GRAVITY / Force_Ref;

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceRotatingFrame_Flow::CSourceRotatingFrame_Flow(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                           CSourceBase_Flow(val_nDim, val_nVar, config) {

  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;

}

CNumerics::ResidualType<> CSourceRotatingFrame_Flow::ComputeResidual(const CConfig* config) {

  unsigned short iDim, iVar, jVar;
  su2double Omega[MAXNDIM] = {0}, Momentum[MAXNDIM] = {0};

  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  /*--- Retrieve the angular velocity vector from config. ---*/

  for (iDim = 0; iDim < 3; iDim++){
    Omega[iDim] = config->GetRotation_Rate(iDim)/config->GetOmega_Ref();
  }

  /*--- Get the momentum vector at the current node. ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    Momentum[iDim] = U_i[iDim+1];

  /*--- Calculate rotating frame source term as ( Omega X Rho-U ) ---*/

  if (nDim == 2) {
    residual[0] = 0.0;
    residual[1] = (Omega[1]*Momentum[2] - Omega[2]*Momentum[1])*Volume;
    residual[2] = (Omega[2]*Momentum[0] - Omega[0]*Momentum[2])*Volume;
    residual[3] = 0.0;
  } else {
    residual[0] = 0.0;
    residual[1] = (Omega[1]*Momentum[2] - Omega[2]*Momentum[1])*Volume;
    residual[2] = (Omega[2]*Momentum[0] - Omega[0]*Momentum[2])*Volume;
    residual[3] = (Omega[0]*Momentum[1] - Omega[1]*Momentum[0])*Volume;
    residual[4] = 0.0;
  }

  /*--- Calculate the source term Jacobian ---*/

  if (implicit) {
    for (iVar = 0; iVar < nVar; iVar++)
      for (jVar = 0; jVar < nVar; jVar++)
        jacobian[iVar][jVar] = 0.0;
    if (nDim == 2) {
      jacobian[1][2] = -Omega[2]*Volume;
      jacobian[2][1] =  Omega[2]*Volume;
    } else {
      jacobian[1][2] = -Omega[2]*Volume;
      jacobian[1][3] =  Omega[1]*Volume;
      jacobian[2][1] =  Omega[2]*Volume;
      jacobian[2][3] = -Omega[0]*Volume;
      jacobian[3][1] = -Omega[1]*Volume;
      jacobian[3][2] =  Omega[0]*Volume;
    }
  }

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceIncRotatingFrame_Flow::CSourceIncRotatingFrame_Flow(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                              CSourceBase_Flow(val_nDim, val_nVar, config) {

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;

  /*--- Retrieve the angular velocity vector from config. ---*/
  for (unsigned short iDim = 0; iDim < 3; iDim++)
    Omega[iDim] = config->GetRotation_Rate(iDim)/config->GetOmega_Ref();

}

CNumerics::ResidualType<> CSourceIncRotatingFrame_Flow::ComputeResidual(const CConfig* config) {

  unsigned short iDim, iVar, jVar;
  su2double Momentum[MAXNDIM] = {0},
            Velocity_i[MAXNDIM] = {0};

  /*--- Primitive variables plus momentum at the node (point i) ---*/

  DensityInc_i  = V_i[nDim+2];

  for (iDim = 0; iDim < nDim; iDim++) {
    Velocity_i[iDim] = V_i[iDim+1];
    Momentum[iDim] = DensityInc_i*Velocity_i[iDim];
  }

  /*--- Calculate rotating frame source term residual as ( Omega X Rho-U ) ---*/

  if (nDim == 2) {
    residual[0] = 0.0;
    residual[1] = (Omega[1]*Momentum[2] - Omega[2]*Momentum[1])*Volume;
    residual[2] = (Omega[2]*Momentum[0] - Omega[0]*Momentum[2])*Volume;
    residual[3] = 0.0;
  } else {
    residual[0] = 0.0;
    residual[1] = (Omega[1]*Momentum[2] - Omega[2]*Momentum[1])*Volume;
    residual[2] = (Omega[2]*Momentum[0] - Omega[0]*Momentum[2])*Volume;
    residual[3] = (Omega[0]*Momentum[1] - Omega[1]*Momentum[0])*Volume;
    residual[4] = 0.0;
  }

  /*--- Calculate the source term Jacobian ---*/

  if (implicit) {
    for (iVar = 0; iVar < nVar; iVar++)
      for (jVar = 0; jVar < nVar; jVar++)
        jacobian[iVar][jVar] = 0.0;
    if (nDim == 2) {
      jacobian[1][2] = -DensityInc_i*Omega[2]*Volume;
      jacobian[2][1] =  DensityInc_i*Omega[2]*Volume;
    } else {
      jacobian[1][2] = -DensityInc_i*Omega[2]*Volume;
      jacobian[1][3] =  DensityInc_i*Omega[1]*Volume;
      jacobian[2][1] =  DensityInc_i*Omega[2]*Volume;
      jacobian[2][3] = -DensityInc_i*Omega[0]*Volume;
      jacobian[3][1] = -DensityInc_i*Omega[1]*Volume;
      jacobian[3][2] =  DensityInc_i*Omega[0]*Volume;
    }
  }

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceWindGust::CSourceWindGust(unsigned short val_nDim, unsigned short val_nVar, const CConfig* config) :
                 CSourceBase_Flow(val_nDim, val_nVar, config) { }

CNumerics::ResidualType<> CSourceWindGust::ComputeResidual(const CConfig* config) {

  su2double u_gust, v_gust, du_gust_dx, du_gust_dy, du_gust_dt, dv_gust_dx, dv_gust_dy, dv_gust_dt;
  su2double smx, smy, se, rho, u, v, p;
  unsigned short GustDir = config->GetGust_Dir(); //Gust direction

  u_gust = WindGust_i[0];
  v_gust = WindGust_i[1];
// w_gust = WindGust_i[2];

  if (GustDir == X_DIR) {
    du_gust_dx = WindGustDer_i[0];
    du_gust_dy = WindGustDer_i[1];
    //du_gust_dz = WindGustDer_i[2];
    du_gust_dt = WindGustDer_i[2];

    dv_gust_dx = 0.0;
    dv_gust_dy = 0.0;
    //dv_gust_dz = 0.0;
    dv_gust_dt = 0.0;

    //dw_gust_dx = 0.0;
    //dw_gust_dy = 0.0;
    //dw_gust_dz = 0.0;
    //dw_gust_dt = 0.0;
  } else {
    du_gust_dx = 0.0;
    du_gust_dy = 0.0;
    //du_gust_dz = 0.0;
    du_gust_dt = 0.0;
    dv_gust_dx = WindGustDer_i[0];
    dv_gust_dy = WindGustDer_i[1];
    //dv_gust_dz = WindGustDer_i[2]
    dv_gust_dt = WindGustDer_i[2];

    //dw_gust_dx = 0.0;
    //dw_gust_dy = 0.0;
    //dw_gust_dz = 0.0;
    //dw_gust_dt = 0.0;
    //

  }

  /*--- Primitive variables at point i ---*/
  u = V_i[1];
  v = V_i[2];
  // w = V_i[3]

  p = V_i[nDim+1];
  rho = V_i[nDim+2];

  /*--- Source terms ---*/
  smx = rho*(du_gust_dt + (u+u_gust)*du_gust_dx + (v+v_gust)*du_gust_dy);
  smy = rho*(dv_gust_dt + (u+u_gust)*dv_gust_dx + (v+v_gust)*dv_gust_dy);
  //smz = rho*(dw_gust_dt + (u+u_gust)*dw_gust_dx + (v+v_gust)*dw_gust_dy) + (w+w_gust)*dw_gust_dz;

  se = u*smx + v*smy + p*(du_gust_dx + dv_gust_dy);
  //se = u*smx + v*smy + w*smz + p*(du_gust_dx + dv_gust_dy + dw_gust_dz);

  if (nDim == 2) {
    residual[0] = 0.0;
    residual[1] = smx*Volume;
    residual[2] = smy*Volume;
    //residual[3] = smz*Volume;
    residual[3] = se*Volume;
  } else {
    SU2_MPI::Error("You should only be in the gust source term in two dimensions", CURRENT_FUNCTION);
  }

  /*--- For now the source term Jacobian is just set to zero ---*/
  //bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  return ResidualType<>(residual, jacobian, nullptr);
}


CSourceIncStreamwise_Periodic::CSourceIncStreamwise_Periodic(unsigned short val_nDim,
                                                             unsigned short val_nVar,
                                                             CConfig        *config) :
                               CSourceBase_Flow(val_nDim, val_nVar, config) {

  turbulent = (config->GetKind_Turb_Model() != TURB_MODEL::NONE);
  energy    = config->GetEnergy_Equation();
  streamwisePeriodic_temperature = config->GetStreamwise_Periodic_Temperature();

  for (unsigned short iDim = 0; iDim < nDim; iDim++)
    Streamwise_Coord_Vector[iDim] = config->GetPeriodic_Translation(0)[iDim];

  /*--- Compute square of the distance between the 2 periodic surfaces via inner product with itself:
        dot_prod(t*t) = (|t|_2)^2  ---*/
  norm2_translation = GeometryToolbox::SquaredNorm(nDim, Streamwise_Coord_Vector);

}

CNumerics::ResidualType<> CSourceIncStreamwise_Periodic::ComputeResidual(const CConfig *config) {

  /* Value of prescribed pressure drop which results in an artificial body force vector. */
  const su2double delta_p = SPvals.Streamwise_Periodic_PressureDrop;

  for (unsigned short iVar = 0; iVar < nVar; iVar++) residual[iVar] = 0.0;

  /*--- Compute the momentum equation source based on the prescribed (or computed if massflow) delta pressure ---*/
  for (unsigned short iDim = 0; iDim < nDim; iDim++) {
    scalar_factor = delta_p / norm2_translation * Streamwise_Coord_Vector[iDim];
    residual[iDim+1] = -Volume * scalar_factor;
  }

  /*--- Compute the periodic temperature contribution to the energy equation, if energy equation is considered ---*/
  if (energy && streamwisePeriodic_temperature) {

    scalar_factor = SPvals.Streamwise_Periodic_IntegratedHeatFlow * DensityInc_i / (SPvals.Streamwise_Periodic_MassFlow * norm2_translation);

    /*--- Compute scalar-product dot_prod(v*t) ---*/
    dot_product = GeometryToolbox::DotProduct(nDim, Streamwise_Coord_Vector, &V_i[1]);

    residual[nDim+1] = Volume * scalar_factor * dot_product;

    /*--- If a RANS turbulence model is used, an additional source term, based on the eddy viscosity gradient is added. ---*/
    if(turbulent) {

      /*--- Compute a scalar factor ---*/
      scalar_factor = SPvals.Streamwise_Periodic_IntegratedHeatFlow / (SPvals.Streamwise_Periodic_MassFlow * sqrt(norm2_translation) * Prandtl_Turb);

      /*--- Compute scalar product between periodic translation vector and eddy viscosity gradient. ---*/
      dot_product = GeometryToolbox::DotProduct(nDim, Streamwise_Coord_Vector, AuxVar_Grad_i[0]);

      residual[nDim+1] -= Volume * scalar_factor * dot_product;
    } // if turbulent
  } // if energy

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceIncStreamwisePeriodic_Outlet::CSourceIncStreamwisePeriodic_Outlet(unsigned short val_nDim,
                                                                         unsigned short val_nVar,
                                                                         CConfig        *config) :
                                     CSourceBase_Flow(val_nDim, val_nVar, config) { }

CNumerics::ResidualType<> CSourceIncStreamwisePeriodic_Outlet::ComputeResidual(const CConfig *config) {

  for (unsigned short iVar = 0; iVar < nVar; iVar++) residual[iVar] = 0.0;

  /*--- m_dot_local = rho * dot_prod(n_A*v), with n_A beeing the area-normal ---*/
  const su2double local_Massflow = DensityInc_i * GeometryToolbox::DotProduct(nDim, Normal, &V_i[1]);

  // Massflow weighted heat sink, which takes out
  // a) the integrated amount over the Heatflux marker
  // b) a user provided quantity, especially the case for CHT cases
  su2double factor;
  if (config->GetStreamwise_Periodic_OutletHeat() == 0.0)
    factor = SPvals.Streamwise_Periodic_IntegratedHeatFlow;
  else
    factor = config->GetStreamwise_Periodic_OutletHeat() / config->GetHeat_Flux_Ref();

  residual[nDim+1] -= abs(local_Massflow/SPvals.Streamwise_Periodic_MassFlow) * factor;

  /*--- Force the area avg inlet Temp to match the Inc_Temperature_Init with additional residual contribution ---*/
  const su2double delta_T = SPvals.Streamwise_Periodic_InletTemperature - config->GetInc_Temperature_Init()/config->GetTemperature_Ref();
  residual[nDim+1] += 0.5 * abs(local_Massflow) * Cp_i * delta_T;

  return ResidualType<>(residual, jacobian, nullptr);
}

CSourceRadiation::CSourceRadiation(unsigned short val_nDim, unsigned short val_nVar, const CConfig *config) :
                  CSourceBase_Flow(val_nDim, val_nVar, config) {

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
}

CNumerics::ResidualType<> CSourceRadiation::ComputeResidual(const CConfig *config) {

  unsigned short iDim;

  /*--- Zero the continuity contribution. ---*/

  residual[0] = 0.0;

  /*--- Zero the momentum contribution. ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    residual[iDim+1] = 0.0;

  /*--- Set the energy contribution ---*/

  residual[nDim+1] = -RadVar_Source[0]*Volume;

  /*--- Set the energy contribution to the Jacobian. ---*/

  if (implicit) {

    /*--- Jacobian is set to zero on initialization. ---*/

    jacobian[nDim+1][nDim+1] = -RadVar_Source[1]*Volume;

  }

  return ResidualType<>(residual, jacobian, nullptr);
}
