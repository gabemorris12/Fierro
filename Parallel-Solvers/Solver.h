/**********************************************************************************************
 © 2020. Triad National Security, LLC. All rights reserved.
 This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos
 National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S.
 Department of Energy/National Nuclear Security Administration. All rights in the program are
 reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear
 Security Administration. The Government is granted for itself and others acting on its behalf a
 nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare
 derivative works, distribute copies to the public, perform publicly and display publicly, and
 to permit others to do so.
 This program is open source under the BSD-3 License.
 Redistribution and use in source and binary forms, with or without modification, are permitted
 provided that the following conditions are met:
 
 1.  Redistributions of source code must retain the above copyright notice, this list of
 conditions and the following disclaimer.
 
 2.  Redistributions in binary form must reproduce the above copyright notice, this list of
 conditions and the following disclaimer in the documentation and/or other materials
 provided with the distribution.
 
 3.  Neither the name of the copyright holder nor the names of its contributors may be used
 to endorse or promote products derived from this software without specific prior
 written permission.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********************************************************************************************/
 
#ifndef FIERRO_SOLVER_H
#define FIERRO_SOLVER_H  

#include "utilities.h"
#include "matar.h"
#include "elements.h"
#include "node_combination.h"
#include <string>
#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_oblackholestream.hpp>
#include <Teuchos_Tuple.hpp>
#include <Teuchos_VerboseObject.hpp>

#include <Tpetra_Core.hpp>
#include <Tpetra_Map.hpp>
#include <Tpetra_MultiVector.hpp>
#include <Tpetra_CrsMatrix.hpp>
#include <Kokkos_View.hpp>
#include "Tpetra_Details_DefaultTypes.hpp"
#include <map>

using namespace mtr;

//forward declarations
namespace swage{
  class mesh_t;
}

namespace elements{
  class element_selector;
  class Element3D;
  class Element2D;
  class ref_element;
}

class Solver{

public:

  //Trilinos type definitions
  typedef Tpetra::Map<>::local_ordinal_type LO;
  typedef Tpetra::Map<>::global_ordinal_type GO;

  typedef Tpetra::CrsMatrix<real_t,LO,GO> MAT;
  typedef const Tpetra::CrsMatrix<real_t,LO,GO> const_MAT;
  typedef Tpetra::MultiVector<real_t,LO,GO> MV;
  typedef Tpetra::MultiVector<GO,LO,GO> MCONN;

  typedef Kokkos::ViewTraits<LO*, Kokkos::LayoutLeft, void, void>::size_type SizeType;
  typedef Tpetra::Details::DefaultTypes::node_type node_type;
  using traits = Kokkos::ViewTraits<LO*, Kokkos::LayoutLeft, void, void>;
  
  using array_layout    = typename traits::array_layout;
  using execution_space = typename traits::execution_space;
  using device_type     = typename traits::device_type;
  using memory_traits   = typename traits::memory_traits;
  using global_size_t = Tpetra::global_size_t;
  
  typedef Kokkos::View<real_t*, Kokkos::LayoutRight, device_type, memory_traits> values_array;
  typedef Kokkos::View<GO*, array_layout, device_type, memory_traits> global_indices_array;
  typedef Kokkos::View<LO*, array_layout, device_type, memory_traits> indices_array;
  //typedef Kokkos::View<SizeType*, array_layout, device_type, memory_traits> row_pointers;
  typedef MAT::local_graph_device_type::row_map_type::non_const_type row_pointers;
  //typedef Kokkos::DualView<real_t**, Kokkos::LayoutLeft, device_type>::t_dev vec_array;
  typedef MV::dual_view_type::t_dev vec_array;
  typedef MV::dual_view_type::t_host host_vec_array;
  typedef Kokkos::View<const real_t**, array_layout, HostSpace, memory_traits> const_host_vec_array;
  typedef Kokkos::View<const real_t**, array_layout, device_type, memory_traits> const_vec_array;
  typedef MV::dual_view_type dual_vec_array;
  typedef MCONN::dual_view_type dual_elem_conn_array;
  typedef MCONN::dual_view_type::t_host host_elem_conn_array;
  typedef MCONN::dual_view_type::t_dev elem_conn_array;
  typedef Kokkos::View<const GO**, array_layout, HostSpace, memory_traits> const_host_elem_conn_array;
  typedef Kokkos::View<const GO**, array_layout, device_type, memory_traits> const_elem_conn_array;

  Solver();
  ~Solver();
  
  virtual void setup() {}

  virtual void run(int argc, char *argv[]) = 0;

  virtual void solver_setup() {}

  virtual void solver_finalize() {}

  virtual void exit_solver(int status);

  virtual void read_mesh_ensight(char *MESH);

  virtual void read_mesh_tecplot(char *MESH);
  
  virtual void read_mesh_vtk(char *MESH);

  virtual void repartition_nodes();

  virtual void tecplot_writer() {}

  virtual void parallel_tecplot_writer() {}

  virtual void parallel_vtk_writer() {}

  //setup ghosts and element maps
  virtual void init_maps();

  //finds the boundary element surfaces in this model
  virtual void Get_Boundary_Patches();

  int setup_flag, finalize_flag;

  //MPI data
  int myrank; //index of this mpi rank in the world communicator
  int nranks; //number of mpi ranks in the world communicator
  MPI_Comm world; //stores the default communicator object (MPI_COMM_WORLD)

  //class Simulation_Parameters *simparam;
  class Simulation_Parameters *simparam;

  //Local FEA data
  size_t nlocal_nodes;
  size_t nlocal_elem_non_overlapping; //used for reduction calls of per element values
  dual_vec_array dual_node_coords; //coordinates of the nodes
  dual_vec_array dual_node_densities; //topology optimization design variable
  dual_elem_conn_array dual_nodes_in_elem; //dual view of element connectivity to nodes
  //host_elem_conn_array nodes_in_elem; //host view of element connectivity to nodes
  CArrayKokkos<elements::elem_types::elem_type, array_layout, HostSpace, memory_traits> Element_Types;
  CArrayKokkos<size_t, array_layout, HostSpace, memory_traits> Nodes_Per_Element_Type;
  size_t max_nodes_per_element, max_nodes_per_patch;
  elements::element_selector *element_select;
  elements::Element3D *elem;
  elements::Element2D *elem2D;
  elements::ref_element  *ref_elem;

  //Ghost data on this MPI rank
  size_t nghost_nodes;
  Kokkos::DualView <GO*, Kokkos::LayoutLeft, device_type, memory_traits> ghost_nodes;
  Kokkos::DualView <int*, array_layout, device_type, memory_traits> ghost_node_ranks;

  //Local FEA data including ghosts
  size_t nall_nodes;
  size_t rnum_elem;

  //Global FEA data
  long long int num_nodes, num_elem;
  Teuchos::RCP<const Teuchos::Comm<int> > comm;
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > map; //map of node indices
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > sorted_map; //sorted contiguous map of node indices
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > ghost_node_map; //map of node indices with ghosts on each rank
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > all_node_map; //map of node indices with ghosts on each rank
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > element_map; //non overlapping map of elements owned by each rank used in reduction ops
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > all_element_map; //overlapping map of elements connected to the local nodes in each rank
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > sorted_element_map; //sorted contiguous map of element indices owned by each rank used in parallel IO
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > local_dof_map; //map of local dofs (typically num_node_local*num_dim)
  Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > all_dof_map; //map of local and ghost dofs (typically num_node_all*num_dim)
  Teuchos::RCP<MCONN> nodes_in_elem_distributed; //element to node connectivity table
  Teuchos::RCP<MCONN> node_nconn_distributed; //how many elements a node is connected to
  Teuchos::RCP<MV> node_coords_distributed;
  Teuchos::RCP<MV> initial_node_coords_distributed;
  Teuchos::RCP<MV> all_node_coords_distributed;
  Teuchos::RCP<MV> design_node_densities_distributed;
  Teuchos::RCP<const MV> test_node_densities_distributed;
  Teuchos::RCP<MV> all_node_densities_distributed;
  Teuchos::RCP<MV> lower_bound_node_densities_distributed;
  Teuchos::RCP<MV> upper_bound_node_densities_distributed;
  Teuchos::RCP<MV> Global_Element_Densities_Upper_Bound;
  Teuchos::RCP<MV> Global_Element_Densities_Lower_Bound;
  Teuchos::RCP<MV> Global_Element_Densities;

  //Distributions of data used to print
  Teuchos::RCP<MV> collected_node_coords_distributed;
  Teuchos::RCP<MV> collected_node_densities_distributed;
  Teuchos::RCP<MCONN> collected_nodes_in_elem_distributed;
  Teuchos::RCP<MV> sorted_node_coords_distributed;
  Teuchos::RCP<MV> sorted_node_densities_distributed;
  Teuchos::RCP<MCONN> sorted_nodes_in_elem_distributed;
  Teuchos::RCP<MV> sorted_element_densities_distributed;

  //Boundary Conditions Data
  //CArray <Nodal_Combination> Patch_Nodes;
  size_t nboundary_patches;
  size_t num_boundary_conditions;
  int current_bdy_id;
  CArrayKokkos<Node_Combination, array_layout, HostSpace, memory_traits> Boundary_Patches;
  std::map<Node_Combination,LO> boundary_patch_to_index; //maps patches to corresponding patch index (inverse of Boundary Patches array)
  
  //file readin variables
  std::ifstream *in;
  std::streampos before_condition_header;
  int words_per_line, elem_words_per_line;
  enum node_ordering_convention {IJK, ENSIGHT};
  node_ordering_convention active_node_ordering_convention;

  //file output variables
  int file_index, nsteps_print;  //file sequence index and print frequency in # of optimization steps

  //output stream
  Teuchos::RCP<Teuchos::FancyOStream> fos;

  //debug and system functions/variables
  double CPU_Time();
  void init_clock();
  double initial_CPU_time, communication_time, dev2host_time, host2dev_time;

};

#endif // end Header Guard
