// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "Common/CBuilder.hpp"

#include "RDM/Schemes/SUPG.hpp"

#include "RDM/SupportedCells.hpp" // supported cells

#include "Physics/Scalar/LinearAdv2D.hpp"       // supported physics
#include "Physics/Scalar/LinearAdvSys2D.hpp"    // supported physics
#include "Physics/Scalar/RotationAdv2D.hpp"     // supported physics

#include "RDM/Scalar/LibScalar.hpp"

using namespace CF::Common;

namespace CF {
namespace RDM {

////////////////////////////////////////////////////////////////////////////////

Common::ComponentBuilder < CellLoopT<SUPG, Physics::Scalar::LinearAdv2D> , RDM::CellLoop, LibScalar > SUPG_LinearAdv2D_Builder;

Common::ComponentBuilder < CellLoopT<SUPG, Physics::Scalar::LinearAdvSys2D> , RDM::CellLoop, LibScalar > SUPG_LinearAdvSys2D_Builder;

Common::ComponentBuilder < CellLoopT<SUPG, Physics::Scalar::RotationAdv2D> , RDM::CellLoop, LibScalar > SUPG_RotationAdv2D_Builder;

////////////////////////////////////////////////////////////////////////////////

} // RDM
} // CF
