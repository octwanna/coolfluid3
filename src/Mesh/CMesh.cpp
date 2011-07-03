// Copyright (C) 2010 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "Common/CBuilder.hpp"
#include "Common/CLink.hpp"
#include "Common/Foreach.hpp"
#include "Common/FindComponents.hpp"
#include "Common/OptionT.hpp"
#include "Common/StringConversion.hpp"
#include "Common/Signal.hpp"

#include "Common/XML/SignalOptions.hpp"

#include "Mesh/LibMesh.hpp"

#include "Mesh/CMesh.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/CNodes.hpp"
#include "Mesh/CMeshElements.hpp"
#include "Mesh/ElementType.hpp"
#include "Mesh/CField.hpp"
#include "Mesh/WriteMesh.hpp"
#include "Mesh/MeshMetadata.hpp"

namespace CF {
namespace Mesh {

using namespace Common;
using namespace Common::XML;

Common::ComponentBuilder < CMesh, Component, LibMesh > CMesh_Builder;

////////////////////////////////////////////////////////////////////////////////

CMesh::CMesh ( const std::string& name  ) :
  Component ( name ),
  m_dimension(0u),
  m_dimensionality(0u)
{
  m_properties.add_property("nb_cells",Uint(0));
  m_properties.add_property("nb_nodes",Uint(0));
  m_properties.add_property("dimensionality",Uint(0));
  m_options.add_option< OptionT<Uint> >("dimension", "Dimension", "Dimension of the mesh (set automatically)", Uint(0))->link_to(&m_dimension); //TODO: Hide this, or make properties with triggers?

  mark_basic(); // by default meshes are visible

  m_nodes_link = create_static_component_ptr<CLink>(Mesh::Tags::nodes());
  m_elements   = create_static_component_ptr<CMeshElements>("elements");
  m_topology   = create_static_component_ptr<CRegion>("topology");
  m_metadata   = create_static_component_ptr<MeshMetadata>("metadata");

  regist_signal ( "write_mesh" , "Write mesh, guessing automatically the format", "Write Mesh" )->signal->connect ( boost::bind ( &CMesh::signal_write_mesh, this, _1 ) );
  signal("write_mesh")->signature->connect(boost::bind(&CMesh::signature_write_mesh, this, _1));
}

////////////////////////////////////////////////////////////////////////////////

CMesh::~CMesh()
{
}

////////////////////////////////////////////////////////////////////////////////

void CMesh::update_statistics()
{
  option("dimension").change_value(nodes().coordinates().row_size());
  boost_foreach ( CEntities& elements, find_components_recursively<CEntities>(topology()) )
    m_dimensionality = std::max(m_dimensionality,elements.element_type().dimensionality());
}

////////////////////////////////////////////////////////////////////////////////

CField& CMesh::create_field( const std::string& name ,
                             const CField::Basis::Type base,
                             const std::string& space,
                             const std::string& variables)
{
  std::vector<std::string> tokenized_variables(0);

  if (variables == "scalar_same_name")
  {
    tokenized_variables.push_back(name+"[scalar]");
  }
  else
  {
    typedef boost::tokenizer<boost::char_separator<char> > Tokenizer;
    boost::char_separator<char> sep(",");
    Tokenizer tokens(variables, sep);

    for (Tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
      tokenized_variables.push_back(*tok_iter);
  }


  std::vector<std::string> names;
  std::vector<std::string> types;
  BOOST_FOREACH(std::string var, tokenized_variables)
  {
    boost::regex e_variable("([[:word:]]+)?[[:space:]]*\\[[[:space:]]*([[:word:]]+)[[:space:]]*\\]");

    boost::match_results<std::string::const_iterator> what;
    if (regex_search(var,what,e_variable))
    {
      names.push_back(what[1]);
      types.push_back(what[2]);
    }
    else
      throw ShouldNotBeHere(FromHere(), "No match found for VarType " + var);
  }

  CField& field = *create_component_ptr<CField>(name);
  field.set_topology(topology());
  field.configure_option("Space",space);
  field.configure_option("VarNames",names);
  field.configure_option("VarTypes",types);
  field.configure_option("FieldType",CField::Basis::Convert::instance().to_str(base));
  field.create_data_storage();

  return field;
}

////////////////////////////////////////////////////////////////////////////////

CField& CMesh::create_scalar_field( const std::string& name , CField& based_on_field)
{
  CField& field = *create_component_ptr<CField>(name);
  field.set_topology(based_on_field.topology());

  std::vector<std::string> names(1,name);
  field.configure_option("VarNames",names);

  std::vector<std::string> types(1,"scalar");
  field.configure_option("VarTypes",types);

  std::string base;   based_on_field.option("FieldType").put_value(base);
  field.configure_option("FieldType",base);

  std::string space; based_on_field.option("Space").put_value(space);
  field.configure_option("Space",space);

  field.create_data_storage();

  return field;

}

////////////////////////////////////////////////////////////////////////////////

CField& CMesh::create_field( const std::string& name , CField& based_on_field)
{
  CField& field = *create_component_ptr<CField>(name);
  field.set_topology(based_on_field.topology());

  std::vector<std::string> names; based_on_field.option("VarNames").put_value(names);

  for (Uint i=0; i<names.size(); ++i)
    names[i] = name+"["+to_str(i)+"]";
  field.configure_option("VarNames",names);

  std::vector<std::string> types; based_on_field.option("VarTypes").put_value(types);
  field.configure_option("VarTypes",types);

  std::string base;   based_on_field.option("FieldType").put_value(base);
  field.configure_option("FieldType",base);

  std::string space; based_on_field.option("Space").put_value(space);
  field.configure_option("Space",space);

  field.create_data_storage();
  return field;

}
////////////////////////////////////////////////////////////////////////////////

CField& CMesh::create_scalar_field(const std::string& field_name, const std::string& variable_name, const CF::Mesh::CField::Basis::Type base)
{
  const std::vector<std::string> names(1, variable_name);
  const std::vector< CField::VarType > types(1, CField::SCALAR);
  return create_field(field_name, base, names, types);
}


////////////////////////////////////////////////////////////////////////////////

CField& CMesh::create_field(const std::string& name, const CField::Basis::Type base, const std::vector< std::string >& variable_names, const std::vector< CField::VarType > variable_types)
{
  cf_assert(variable_names.size() == variable_types.size());

  // TODO: Treat variable_types using EnumT, and store enum values in the option instead of strings
  std::vector<std::string> types_str;
  types_str.reserve( variable_types.size() );
  boost_foreach(const CField::VarType var_type, variable_types)
  {
    types_str.push_back( boost::lexical_cast<std::string>(var_type) );
  }

  CField& field = *create_component_ptr<CField>(name);
  field.set_topology(topology());
  field.configure_option("VarNames",variable_names);
  field.configure_option("VarTypes",types_str);
  field.configure_option("FieldType", CField::Basis::Convert::instance().to_str(base) );
  field.create_data_storage();

  return field;
}

////////////////////////////////////////////////////////////////////////////////

CNodes& CMesh::nodes()
{
  cf_assert( is_not_null(m_nodes_link->follow()) );
  return *m_nodes_link->follow()->as_ptr<CNodes>();
}

////////////////////////////////////////////////////////////////////////////////

const CNodes& CMesh::nodes() const
{
  cf_assert( is_not_null(m_nodes_link->follow()) );
  return *m_nodes_link->follow()->as_ptr<CNodes>();
}

////////////////////////////////////////////////////////////////////////////////

CMeshElements& CMesh::elements()
{
  return *m_elements;
}

////////////////////////////////////////////////////////////////////////////////

const CMeshElements& CMesh::elements() const
{
  return *m_elements;
}

//////////////////////////////////////////////////////////////////////////////

void CMesh::signature_write_mesh ( SignalArgs& node)
{
  SignalOptions options( node );

  options.add<std::string>("file" , name()+".msh" , "File to write" );

  boost_foreach (CField& field, find_components<CField>(*this))
  {
    options.add<bool>(field.name(), false, "Mark if field gets to be written");
  }
}

////////////////////////////////////////////////////////////////////////////////

void CMesh::signal_write_mesh ( SignalArgs& node )
{
  WriteMesh::Ptr mesh_writer = create_component_ptr<WriteMesh>("writer");

  SignalOptions options( node );


  std::string file = name()+".msh";

  if (options.exists("file"))
      file = options.option<std::string>("file");

  // check protocol for file loading
  // if( file.scheme() != URI::Scheme::FILE )
  //   throw ProtocolError( FromHere(), "Wrong protocol to access the file, expecting a \'file\' but got \'" + file.string() + "\'" );

  URI fpath( file );
//  if( fpath.scheme() != URI::Scheme::FILE )
//    throw ProtocolError( FromHere(), "Wrong protocol to access the file, expecting a \'file\' but got \'" + fpath.string() + "\'" );

  std::vector<URI> fields;

  boost_foreach( CField& field, find_components<CField>(*this))
  {
    if (options.exists(field.name()))
    {
      bool add_field = options.option<bool>(field.name());
      if (add_field)
        fields.push_back(field.uri());
    }
  }
  mesh_writer->write_mesh(*this,fpath,fields);
  remove_component(mesh_writer->name());
}

////////////////////////////////////////////////////////////////////////////////

} // Mesh
} // CF
