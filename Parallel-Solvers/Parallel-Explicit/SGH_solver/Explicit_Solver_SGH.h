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
 
#ifndef EXPLICIT_SOLVER_SGH_H
#define EXPLICIT_SOLVER_SGH_H

#include "utilities.h"
#include "Explicit_Solver.h"
#include "matar.h"
#include "elements.h"
#include "node_combination.h"
#include <string>
#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_oblackholestream.hpp>

#include <Tpetra_Core.hpp>
#include <Tpetra_Map.hpp>
#include <Tpetra_MultiVector.hpp>
#include <Tpetra_CrsMatrix.hpp>
#include <Kokkos_View.hpp>
#include "Tpetra_Details_DefaultTypes.hpp"
#include <map>

//#include <Xpetra_Operator.hpp>
//#include <MueLu.hpp>

//forward declarations
/*
namespace swage{
  class mesh_t;
}
*/
class mesh_t;

namespace elements{
  class element_selector;
  class Element3D;
  class Element2D;
  class ref_element;
}

class FEA_Module;

class Explicit_Solver_SGH: public Explicit_Solver{

public:
  Explicit_Solver_SGH();
  ~Explicit_Solver_SGH();

  void run(int argc, char *argv[]);

  //void read_mesh_ensight(char *MESH);

  void read_mesh_ansys_dat(const char *MESH);

  void init_state_vectors();

  //void repartition_nodes();

  void comm_velocities();

  void comm_densities();

  void init_design();

  void collect_information();

  void sort_information();

  //process input to decide TO problem and FEA modules
  void FEA_module_setup();

  void setup_optimization_problem();
  
  //initialize data for boundaries of the model and storage for boundary conditions and applied loads
  void init_boundaries();
  
  //interfaces between user input and creating data structures for bcs
  void topology_conditions();

  //void vtk_writer();

  //void ensight_writer();

  //interfaces between user input and creating data structures for topology conditions
  void generate_tcs();

  void init_topology_conditions (int num_sets);

  void tecplot_writer();

  void parallel_tecplot_writer();

  void parallel_vtk_writer();

  //void init_boundary_sets(int num_boundary_sets);

  void tag_boundaries(int this_bc_tag, real_t val, int bdy_set, real_t *patch_limits = NULL);

  int check_boundary(Node_Combination &Patch_Nodes, int this_bc_tag, real_t val, real_t *patch_limits);
  
  mesh_t *init_mesh;
  mesh_t *mesh;
  
  //class Simulation_Parameters *simparam;
  class Simulation_Parameters_SGH *simparam;

  //FEA simulations
  class FEA_Module_SGH *sgh_module;

  //set of enabled FEA modules
  std::vector<std::string> fea_module_types;
  std::vector<FEA_Module*> fea_modules;
  std::vector<bool> fea_module_must_read;
  int nfea_modules;
  int displacement_module;

  //Global FEA data
  Teuchos::RCP<MV> node_velocities_distributed;
  Teuchos::RCP<MV> initial_node_velocities_distributed;
  Teuchos::RCP<MV> all_node_velocities_distributed;
  Teuchos::RCP<MV> ghost_node_velocities_distributed;
  Teuchos::RCP<MV> all_cached_node_velocities_distributed;

  //Distributions of data used to print
  Teuchos::RCP<MV> collected_node_velocities_distributed;
  Teuchos::RCP<MV> sorted_node_velocities_distributed;
  
  //Boundary Conditions Data
  DCArrayKokkos<size_t> Local_Index_Boundary_Patches;
  CArrayKokkos<size_t, array_layout, HostSpace, memory_traits> Topology_Condition_Patches; //set of patches corresponding to each boundary condition
  CArrayKokkos<size_t, array_layout, HostSpace, memory_traits> NTopology_Condition_Patches;

  //types of boundary conditions
  enum tc_type {NONE, TO_SURFACE_CONSTRAINT, TO_BODY_CONSTRAINT};

  //lists what kind of boundary condition each boundary set is assigned to
  CArrayKokkos<int, array_layout, HostSpace, memory_traits> Boundary_Condition_Type_List;
  
  //number of displacement boundary conditions acting on nodes; used to size the reduced global stiffness map
  size_t Number_DOF_BCS;

  //! mapping used to get local ghost index from the global ID.
  //typedef ::Tpetra::Details::FixedHashTable<GO, LO, Kokkos::HostSpace::device_type>
    //global_to_local_table_host_type;

  //global_to_local_table_host_type global2local_map;
  //CArrayKokkos<int, Kokkos::LayoutLeft, Kokkos::HostSpace::device_type> active_ranks;

  //allocation flags to avoid repeat MV and global matrix construction
  int Matrix_alloc;

  //debug flags
  int gradient_print_sync;

};

#endif // end HEADER_H
