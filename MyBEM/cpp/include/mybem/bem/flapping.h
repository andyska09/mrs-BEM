#pragma once

#include "mybem/prop_state.h"

namespace mybem {

/* Coning and flapping angles. Generated from Maple/BEM_Derivation.mw; the
 * lift/drag coefficients are baked into the numeric constants, so retuning
 * cl/cd cannot move these angles. */
double coning(const PropState&);
double longitudinalFlapping(const PropState&);
double lateralFlapping(const PropState&);

}  // namespace mybem
