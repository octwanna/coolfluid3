#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>

#include "Common/ObjectProvider.hpp"
#include "Common/OptionT.hpp"

#include "Mesh/CMesh.hpp"
#include "Mesh/CRegion.hpp"
#include "Mesh/CGNS/Reader.hpp"

//////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace Mesh {
namespace CGNS {
  
////////////////////////////////////////////////////////////////////////////////

CF::Common::ObjectProvider < Mesh::CGNS::CReader,
                             Mesh::CMeshReader,
                             Mesh::CGNS::CGNSLib,
                             1 >
aCGNSReader_Provider ( "CGNS" );

//////////////////////////////////////////////////////////////////////////////

CReader::CReader(const CName& name)
: CMeshReader(name),
  m_isCoordinatesCreated(false)
{
  build_component(this);
}

//////////////////////////////////////////////////////////////////////////////

void CReader::defineConfigOptions ( CF::Common::OptionList& options )
{
  options.add< CF::Common::OptionT<bool> >
      ( "SectionsAreBCs",
        ("Treat Sections of lower dimensionality as BC. "
        "This means no BCs from cgns will be read"),
        true );
}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_from_to(boost::filesystem::path& fp, const CMesh::Ptr& mesh)
{

  // Set the internal mesh pointer
  m_mesh = mesh;

  // Create basic region structure
  CRegion::Ptr regions        = m_mesh->create_region("regions");
  CRegion::Ptr volume_regions = regions->create_region("volume-regions");
  CRegion::Ptr bc_regions     = regions->create_region("bc-regions");

  // open file in read mode
  cg_open(fp.string().c_str(),CG_MODE_READ,&m_file.idx);

  // check how many bases we have
  cg_nbases(m_file.idx,&m_file.nbBases);
  CFinfo << "nb bases : " << m_file.nbBases << "\n" << CFendl;

  m_base.unique = m_file.nbBases==1 ? true : false;
  for (m_base.idx = 1; m_base.idx<=m_file.nbBases; ++m_base.idx)
  {
    CFinfo << "m_base.idx = " << m_base.idx << "\n" << CFendl;
    read_base(volume_regions);
  }

  // remove bc_regions component if there are no bc's defined
  if (!bc_regions->has_subregions())
  {
    regions->remove_component(bc_regions->name());
    bc_regions.reset();
    CFinfo << "No boundary conditions were found! \n" << CFendl;
  }

  // close the CGNS file
  cg_close(m_file.idx);

}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_base(CRegion::Ptr& parent_region)
{
  // get the name, dimension and physical dimension from the base
  char base_name_char[CGNS_CHAR_MAX];
  cg_base_read(m_file.idx,m_base.idx,base_name_char,&m_base.cell_dim,&m_base.phys_dim);
  m_base.name=base_name_char;
  boost::algorithm::replace_all(m_base.name," ","_");

  CFinfo << "base name     : " << m_base.name  << "\n" << CFendl;
  CFinfo << "base cell dim : " << m_base.cell_dim  << "\n" << CFendl;
  CFinfo << "base phys dim : " << m_base.phys_dim  << "\n" << CFendl;

  // check how many zones we have
  cg_nzones(m_file.idx,m_base.idx,&m_base.nbZones);
  CFinfo << "number of zones     : " << m_base.nbZones  << "\n" << CFendl;

  // create region for the base in mesh
  CRegion::Ptr base_region = parent_region;
  if (!m_base.unique)
    base_region = parent_region->create_region(m_base.name);

  for (m_zone.idx = 1; m_zone.idx<=m_base.nbZones; ++m_zone.idx)
  {
    CFinfo << "m_zone.idx = " << m_zone.idx << "\n" << CFendl;
    read_zone(base_region);
  }
}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_zone(CRegion::Ptr& parent_region)
{


  // get zone type
  cg_zone_type(m_file.idx,m_base.idx,m_zone.idx,&m_zone.type);
  //if (zone_type == Structured) throw CGNSException (FromHere(),"Cannot handle structured meshes.");

  // get zone size and name
  char zone_name_char[CGNS_CHAR_MAX];
  int size[3];
  cg_zone_read(m_file.idx,m_base.idx,m_zone.idx,zone_name_char,size);
    m_zone.name = zone_name_char;
    boost::algorithm::replace_all(m_zone.name," ","_");
  CFinfo << "\nzone name   : " << m_zone.name << "\n" << CFendl;
    m_zone.nbVertices = size[CGNS_VERT_IDX];
    m_zone.nbElements = size[CGNS_CELL_IDX];
    m_zone.nbBdryVertices = size[CGNS_BVRT_IDX];
  CFinfo << "zone type: " << CFendl;
  if (m_zone.type == Structured)
    CFinfo << "Structured \n" << CFendl;
  else if (m_zone.type == Unstructured)
    CFinfo << "Unstructured \n" << CFendl;
  else
    CFinfo << "Unknown zone_type \n" << CFendl;
  // get the number of grids
  cg_ngrids(m_file.idx,m_base.idx,m_zone.idx,&m_zone.nbGrids);
  // nb coord dims
  cg_ncoords(m_file.idx,m_base.idx,m_zone.idx, &m_zone.coord_dim);
  // find out number of solutions
  cg_nsols(m_file.idx,m_base.idx,m_zone.idx,&m_zone.nbSols);
  // find out how many sections
  cg_nsections(m_file.idx,m_base.idx,m_zone.idx,&m_zone.nbSections);
  // find out number of BCs that exist under this zone
  cg_nbocos(m_file.idx,m_base.idx,m_zone.idx,&m_zone.nbBocos);
  // Add up all the nb elements from all sections
  m_zone.total_nbElements = get_total_nbElements();

  // Print zone info
  CFinfo << "coord dim   : " << m_zone.coord_dim << "\n" << CFendl;
  CFinfo << "nb nodes    : " << m_zone.nbVertices << "\n" << CFendl;
  CFinfo << "nb elems    : " << m_zone.nbElements << "\n" << CFendl;
  CFinfo << "nb bnodes   : " << m_zone.nbBdryVertices << "\n" << CFendl;
  CFinfo << "nb grids    : " << m_zone.nbGrids << "\n" << CFendl;
  CFinfo << "nb sols     : " << m_zone.nbSols << "\n" << CFendl;
  CFinfo << "nb sections : " << m_zone.nbSections << "\n" << CFendl;
  CFinfo << "nb bcs      : " << m_zone.nbBocos << "\n" << CFendl;

  CFinfo << "total nb elems : " << m_zone.total_nbElements << "\n" << CFendl;

  // Create a region for this zone
  parent_region->create_region(m_zone.name);
  CRegion::Ptr this_region = parent_region->get_component<CRegion>(m_zone.name);

  // read coordinates in this zone
  for (int i=1; i<=m_zone.nbGrids; ++i)
    read_coordinates();

  // read sections (or subregions) in this zone
  m_global_to_region.reserve(m_zone.total_nbElements);
  for (m_section.idx=1; m_section.idx<=m_zone.nbSections; ++m_section.idx)
    read_section(this_region);

/// @todo  if (!option("SectionsAreBCs")->value<bool>())
  if (false)
  {
    // read boundaryconditions (or subregions) in this zone
    for (m_boco.idx=1; m_boco.idx<=m_zone.nbBocos; ++m_boco.idx)
      read_boco();

    // Remove regions flagged as bc
    for(CRegion::Iterator region_it=this_region->begin(); region_it!=this_region->end(); ++region_it)
      if(!region_it->has_subregions())
        if (region_it->get_component<CElements>("type")->getDimensionality() < static_cast<Uint>(m_base.cell_dim))
    {
      Component::Ptr region_to_rm = region_it.get_ptr()->get_parent();
      CFinfo << "Removing region flagged as bc : " << region_to_rm->name() << "\n" << CFendl;
      region_to_rm->get_parent()->remove_component(region_to_rm->name());
      region_to_rm.reset();
    }
  }

  // Cleanup:

  // truely deallocate the global_to_region vector
  m_global_to_region.resize(0);
  std::vector<Region_TableIndex_pair>().swap (m_global_to_region);


}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_coordinates()
{


  // create coordinates component  mesh/coordinates
  if (!m_isCoordinatesCreated)
  {
    m_mesh->create_array("coordinates");
    m_mesh->get_component<CArray>("coordinates")->initialize(m_zone.coord_dim);
    m_isCoordinatesCreated = true;
  }

  CArray::Ptr coordinates = m_mesh->get_component<CArray>("coordinates");

  // read coordinates
  int one = 1;
  Real *xCoord;
  Real *yCoord;
  Real *zCoord;
  switch (m_zone.coord_dim)
  {
    case 3:
      zCoord = new Real[m_zone.nbVertices];
      cg_coord_read(m_file.idx,m_base.idx,m_zone.idx, "CoordinateZ", RealDouble, &one, &m_zone.nbVertices, zCoord);
    case 2:
      yCoord = new Real[m_zone.nbVertices];
      cg_coord_read(m_file.idx,m_base.idx,m_zone.idx, "CoordinateY", RealDouble, &one, &m_zone.nbVertices, yCoord);
    case 1:
      xCoord = new Real[m_zone.nbVertices];
      cg_coord_read(m_file.idx,m_base.idx,m_zone.idx, "CoordinateX", RealDouble, &one, &m_zone.nbVertices, xCoord);
  }

  CArray::Buffer buffer = coordinates->create_buffer(m_zone.nbVertices);

  std::vector<Real> row(m_zone.coord_dim);
  for (int i=0; i<m_zone.nbVertices; ++i)
  {
    switch (m_zone.coord_dim)
    {
      case 3:
        row[2] = zCoord[i];
       case 2:
        row[1] = yCoord[i];
       case 1:
        row[0] = xCoord[i];
     }
    buffer.add_row(row);
  }

  delete_ptr(xCoord);
  delete_ptr(yCoord);
  delete_ptr(zCoord);
}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_section(CRegion::Ptr& parent_region)
{


  char section_name_char[CGNS_CHAR_MAX];
  cg_section_read(m_file.idx, m_base.idx, m_zone.idx, m_section.idx, section_name_char, &m_section.type,
                          &m_section.eBegin, &m_section.eEnd, &m_section.nbBdry, &m_section.parentFlag);
  m_section.name=section_name_char;
  boost::algorithm::replace_all(m_section.name," ","_");

  CFinfo << "\nsection: " << m_section.name << "\n" << CFendl;
  CRegion::Ptr this_region = parent_region->create_region(m_section.name);

  //CFinfo << "eRange: " << m_section.eBegin << " - " << m_section.eEnd << "\n" << CFendl;

  if (m_section.type == MIXED)
  {
    CFinfo << "etype: MIXED --> create subregions for each element type \n" << CFendl;
    BufferMap buffer = create_leaf_regions_with_buffermap(this_region,get_supported_element_types());
    for (int elem=m_section.eBegin;elem<=m_section.eEnd;++elem)
    {
      // Read one line of connectivity at a time
      cg_ElementPartialSize(m_file.idx,m_base.idx,m_zone.idx,m_section.idx,elem,elem,&m_section.elemNodeCount);
      int elemNodes[1][m_section.elemNodeCount];
      cg_elements_partial_read(m_file.idx,m_base.idx,m_zone.idx,m_section.idx,elem,elem,*elemNodes,&m_section.parentData);
      m_section.elemNodeCount--; // subtract 1 as there is one index too many storing the cell type
      ElementType_t etype = static_cast<ElementType_t>(elemNodes[0][0]);

      // Take out the nodes and put in the buffer of this element type
      std::vector<Uint> row;
      row.reserve(m_section.elemNodeCount);
      for (int n=1;n<=m_section.elemNodeCount;++n)  // n=0 is the cell type
        row.push_back(elemNodes[0][n]-1); // -1 because cgns has index-base 1 instead of 0
      const std::string& etype_CF = m_elemtype_CGNS_to_CF[etype];
      Uint table_idx = buffer[etype_CF]->get_total_nbRows();
      buffer[etype_CF]->add_row(row);
      m_global_to_region.push_back(Region_TableIndex_pair(this_region->get_component<CRegion>(etype_CF),table_idx));
    } // for elem
  } // if mixed
  else
  {
    CFinfo << "etype: " << cg_ElementTypeName(m_section.type) << "\n" << CFendl;

    cg_npe(m_section.type,&m_section.elemNodeCount);
    //CFinfo << "elemNodeCount = " << m_section.elemNodeCount << "\n" << CFendl;

    cg_ElementDataSize(m_file.idx,m_base.idx,m_zone.idx,m_section.idx,&m_section.elemDataSize	);
    //CFinfo << "elementDataSize = " << m_section.elemDataSize << "\n" << CFendl;

    int nbElems = m_section.elemDataSize/m_section.elemNodeCount;
    CFinfo << "nbElems = " << nbElems << "\n" << CFendl;

    const std::string& etype_CF = m_elemtype_CGNS_to_CF[m_section.type];
    CRegion::Ptr leaf_region = this_region->create_leaf_region(etype_CF);
    CTable::Buffer buffer = leaf_region->get_component<CTable>("table")->create_buffer(std::max(1024,nbElems/10));


    int* elemNodes = new int [m_section.elemDataSize];
    cg_elements_read	(m_file.idx,m_base.idx,m_zone.idx,m_section.idx, elemNodes,&m_section.parentData);


    // fill connectivity table
    //boost::progress_display progress(nbElems);
    for (int elem=0; elem<nbElems; ++elem) //, ++progress)
    {
      std::vector<Uint> row;
      row.reserve(m_section.elemNodeCount);
      for (int node=0;node<m_section.elemNodeCount;++node)
        row.push_back(elemNodes[node+elem*m_section.elemNodeCount]-1); // -1 because cgns has index-base 1 instead of 0

      buffer.add_row(row);
      m_global_to_region.push_back(Region_TableIndex_pair(leaf_region,elem));
    } // for elem
    delete_ptr(elemNodes);
  } // else not mixed

  remove_empty_leaf_regions(this_region);

  /// @todo  if (option("SectionsAreBCs")->value<bool>())
  if (true)
  {
    bool is_bc_region = false;
    for(CRegion::Iterator region_it=this_region->begin(); region_it!=this_region->end(); ++region_it)
      if(!region_it->has_subregions())
      {
        if (region_it->get_component<CElements>("type")->getDimensionality() < static_cast<Uint>(m_base.cell_dim))
        {
          is_bc_region = is_bc_region || true;
        }
      }
    if (is_bc_region)
    {
      this_region->move_component( m_mesh->get_component("regions")->get_component("bc-regions") );
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

void CReader::read_boco()
{
  // Read the info for this boundary condition.
  char boco_name_char[CGNS_CHAR_MAX];
  cg_boco_info(m_file.idx, m_base.idx, m_zone.idx, m_boco.idx, boco_name_char, &m_boco.boco_type, &m_boco.ptset_type,
               &m_boco.nBC_elem, &m_boco.normalIndex, &m_boco.normalListFlag, &m_boco.normalDataType, &m_boco.nDataSet);
  m_boco.name = boco_name_char;
  CFinfo << "BC name:       " << m_boco.name << "\n" << CFendl;
  switch (m_boco.ptset_type)
  {
    case ElementRange : CFinfo << "BC boco_type: ElementRange \n" << CFendl; break;
    case ElementList : CFinfo << "BC boco_type: ElementList \n" << CFendl; break;
    case PointRange : CFinfo << "BC boco_type: PointRange \n" << CFendl; break;
    case PointList : CFinfo << "BC boco_type: PointList \n" << CFendl; break;
    default : CFinfo << "BC boco_type : NOT SUPPORTED \n" << CFendl; break;
  }
  CFinfo << "BC nBC_elem :  " << m_boco.nBC_elem << "\n" << CFendl;

  // Read the element ID's
  int* boco_elems = new int [m_boco.nBC_elem];
  void* NormalList(NULL);
  cg_boco_read(m_file.idx, m_base.idx, m_zone.idx, m_boco.idx, boco_elems, NormalList);


  CRegion::Ptr bc_region = m_mesh->get_component("regions")->get_component<CRegion>("bc-regions")->create_region(m_boco.name);
  BufferMap buffer = create_leaf_regions_with_buffermap(bc_region,get_supported_element_types());

  switch (m_boco.ptset_type)
  {
    case ElementRange :
    {
      for (int global_element=boco_elems[0]-1;global_element<boco_elems[1];++global_element)
      {
        CRegion::Ptr region = m_global_to_region[global_element].first;
        Uint local_element = m_global_to_region[global_element].second;
        buffer[region->name()]->add_row(region->get_component<CTable>("table")->get_table()[local_element]);
      }
      break;
    }
    case ElementList :
    {
      for (int i=0; i<m_boco.nBC_elem; ++i)
      {
        Uint global_element = boco_elems[i]-1;
        CRegion::Ptr region = m_global_to_region[global_element].first;
        Uint local_element = m_global_to_region[global_element].second;
        //CFinfo << "    " << global_element << " :  " << region->get_parent()->name() << "  -> " << local_element << "\n" << CFendl;
        buffer[region->name()]->add_row(region->get_component<CTable>("table")->get_table()[local_element]);
      }
      break;
    }
    case PointRange :
    case PointList :

    default :
        CFinfo << "EXCEPTION: NOT SUPPORTED \n" << CFendl;
  }

  // Flush all buffers and remove empty leaf regions
  for (BufferMap::iterator it=buffer.begin(); it!=buffer.end(); ++it)
    it->second->flush();

  remove_empty_leaf_regions(bc_region);

}

//////////////////////////////////////////////////////////////////////////////

Uint CReader::get_total_nbElements()
{
  // read sections (or subregions) in this zone
  Uint nbElements(0);
  for (m_section.idx=1; m_section.idx<=m_zone.nbSections; ++m_section.idx)
  {
    char section_name_char[CGNS_CHAR_MAX];
    cg_section_read(m_file.idx, m_base.idx, m_zone.idx, m_section.idx, section_name_char, &m_section.type,
                            &m_section.eBegin, &m_section.eEnd, &m_section.nbBdry, &m_section.parentFlag);
    nbElements += m_section.eEnd - m_section.eBegin + 1;
  }
  return nbElements;
}

//////////////////////////////////////////////////////////////////////////////

} // CGNS
} // Mesh
} // CF
