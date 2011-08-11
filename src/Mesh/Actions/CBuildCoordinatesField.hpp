// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_Mesh_CBuildCoordinatesField_hpp
#define CF_Mesh_CBuildCoordinatesField_hpp

////////////////////////////////////////////////////////////////////////////////

#include "Mesh/CMeshTransformer.hpp"

#include "Mesh/Actions/LibActions.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Mesh {
namespace Actions {
  
//////////////////////////////////////////////////////////////////////////////

/// This class defines a mesh transformer
/// that builds the coordinates field
/// @author Willem Deconinck
class Mesh_Actions_API CBuildCoordinatesField : public CMeshTransformer
{
public: // typedefs

    typedef boost::shared_ptr<CBuildCoordinatesField> Ptr;
    typedef boost::shared_ptr<CBuildCoordinatesField const> ConstPtr;

private: // typedefs
  
public: // functions
  
  /// constructor
  CBuildCoordinatesField( const std::string& name );
  
  /// Gets the Class name
  static std::string type_name() { return "CBuildCoordinatesField"; }

  virtual void execute();
  
  /// brief description, typically one line
  virtual std::string brief_description() const;
  
  /// extended help that user can query
  virtual std::string help() const;
    
}; // CBuildCoordinatesField


////////////////////////////////////////////////////////////////////////////////

} // Actions
} // Mesh
} // CF

////////////////////////////////////////////////////////////////////////////////

#endif // CF_Mesh_CBuildCoordinatesField_hpp
