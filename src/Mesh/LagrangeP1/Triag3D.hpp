// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_Mesh_LagrangeP1_Triag3D_hpp
#define CF_Mesh_LagrangeP1_Triag3D_hpp

#include "Mesh/ElementType.hpp"
#include "Mesh/LagrangeP1/Triag.hpp"

namespace CF {
namespace Mesh {
  template <typename SF> class ShapeFunctionT;
namespace LagrangeP1 {

////////////////////////////////////////////////////////////////////////////////

/// @brief 3D Lagrange P1 Triangular Element type
/// This class provides the lagrangian shape function describing the
/// representation of the solution and/or the geometry in a P1 (linear)
/// triangular element.
/// @see ElementType for documentation of undocumented functions
/// @author Willem Deconinck
/// @author Tiago Quintino
/// @author Bart Janssens
struct Mesh_LagrangeP1_API Triag3D
{
public: // typedefs

  typedef boost::shared_ptr<Triag3D>       Ptr;
  typedef boost::shared_ptr<Triag3D const> ConstPtr;

  /// The shape function of this element
  typedef Triag SF;

  /// @name Element definitions
  //  -------------------------
  //@{
  enum { shape          = SF::shape          };
  enum { dimensionality = SF::dimensionality };
  enum { nb_nodes       = SF::nb_nodes       };
  enum { order          = SF::order          };

  enum { dimension      = 3 };
  enum { nb_faces       = 1 };
  enum { nb_edges       = 3 };
  //@}

  /// @name Matrix Types
  //  --------------------------------
  //@{
  typedef SF::MappedCoordsT                              MappedCoordsT;
  typedef Eigen::Matrix<Real, dimension, 1>              CoordsT;
  typedef Eigen::Matrix<Real, nb_nodes, dimension>       NodesT;
  typedef Eigen::Matrix<Real, dimensionality, dimension> JacobianT;
  //@}

public: // functions

  /// @name Constructor / Destructor / Type name
  //  ------------------------------------------
  //@{

  Triag3D() {}
  ~Triag3D() {}
  static std::string type_name() { return "Triag3D"; }

  //@}

  /// @name Accessor functions
  //  ------------------------
  //@{

  static const ShapeFunctionT<SF>& shape_function();
  static const ElementType::FaceConnectivity& faces();
  static const ElementType& face_type(const Uint face);

  //@}

  /// @name Computation functions
  //  ---------------------------
  //@{

  static JacobianT jacobian(const MappedCoordsT& mapped_coord, const NodesT& nodes);
  static void compute_jacobian(const MappedCoordsT& mapped_coord, const NodesT& nodes, JacobianT& jacobian);
  static Real volume(const NodesT& nodes);
  static Real area(const NodesT& nodes);
  static void compute_centroid(const NodesT& nodes , CoordsT& centroid);
  static void compute_normal(const NodesT& nodes , CoordsT& normal);

  //@}

};

////////////////////////////////////////////////////////////////////////////////

} // LagrangeP1
} // Mesh
} // CF

#endif // CF_Mesh_LagrangeP1_Triag3D_hpp
