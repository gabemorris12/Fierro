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

#ifndef FEA_MODULE_SGH_H
#define FEA_MODULE_SGH_H

#include "mesh.h"
#include "state.h"
#include "matar.h"
#include "elements.h"
#include "node_combination.h"
#include "Solver.h"
#include "FEA_Module.h"

class Explicit_Solver_SGH;
class Simulation_Parameters_SGH;
class Simulation_Parameters_Dynamic_Optimization;
class Simulation_Parameters_Elasticity;

class FEA_Module_SGH: public FEA_Module{

public:
  
  FEA_Module_SGH(Solver *Solver_Pointer, mesh_t& mesh, const int my_fea_module_index = 0);
  ~FEA_Module_SGH();
  
  //initialize data for boundaries of the model and storage for boundary conditions and applied loads
  void sgh_interface_setup(mesh_t &mesh, node_t &node, elem_t &elem, corner_t &corner);

  void setup();

  // to allow the user strngth model interface to do all neccesary cleanup
  void cleanup_user_strength_model();

  void sgh_solve();

  void get_force_sgh(const DCArrayKokkos <material_t> &material,
                     const mesh_t &mesh,
                     const DViewCArrayKokkos <double> &node_coords,
                     const DViewCArrayKokkos <double> &node_vel,
                     const DViewCArrayKokkos <double> &elem_den,
                     const DViewCArrayKokkos <double> &elem_sie,
                     const DViewCArrayKokkos <double> &elem_pres,
                     DViewCArrayKokkos <double> &elem_stress,
                     const DViewCArrayKokkos <double> &elem_sspd,
                     const DViewCArrayKokkos <double> &elem_vol,
                     const DViewCArrayKokkos <double> &elem_div,
                     const DViewCArrayKokkos <size_t> &elem_mat_id,
                     DViewCArrayKokkos <double> &corner_force,
                     const DViewCArrayKokkos <double> &elem_statev,
                     const double rk_alpha,
                     const size_t cycle);
  
  void get_force_elastic(const DCArrayKokkos <material_t> &material,
                     const mesh_t &mesh,
                     const DViewCArrayKokkos <double> &node_coords,
                     const DViewCArrayKokkos <double> &node_vel,
                     const DViewCArrayKokkos <double> &node_mass,
                     const DViewCArrayKokkos <double> &elem_den,
                     const DViewCArrayKokkos <double> &elem_vol,
                     const DViewCArrayKokkos <double> &elem_div,
                     const DViewCArrayKokkos <size_t> &elem_mat_id,
                     DViewCArrayKokkos <double> &corner_force,
                     const DViewCArrayKokkos <double> &elem_statev,
                     const double rk_alpha,
                     const size_t cycle);

  void get_force_vgradient_sgh(const DCArrayKokkos <material_t> &material,
                     const mesh_t &mesh,
                     const DViewCArrayKokkos <double> &node_coords,
                     const DViewCArrayKokkos <double> &node_vel,
                     const DViewCArrayKokkos <double> &elem_den,
                     const DViewCArrayKokkos <double> &elem_sie,
                     const DViewCArrayKokkos <double> &elem_pres,
                     const DViewCArrayKokkos <double> &elem_stress,
                     const DViewCArrayKokkos <double> &elem_sspd,
                     const DViewCArrayKokkos <double> &elem_vol,
                     const DViewCArrayKokkos <double> &elem_div,
                     const DViewCArrayKokkos <size_t> &elem_mat_id,
                     const DViewCArrayKokkos <double> &elem_statev,
                     const double rk_alpha,
                     const size_t cycle);

  void get_force_ugradient_sgh(const DCArrayKokkos <material_t> &material,
                     const mesh_t &mesh,
                     const DViewCArrayKokkos <double> &node_coords,
                     const DViewCArrayKokkos <double> &node_vel,
                     const DViewCArrayKokkos <double> &elem_den,
                     const DViewCArrayKokkos <double> &elem_sie,
                     const DViewCArrayKokkos <double> &elem_pres,
                     const DViewCArrayKokkos <double> &elem_stress,
                     const DViewCArrayKokkos <double> &elem_sspd,
                     const DViewCArrayKokkos <double> &elem_vol,
                     const DViewCArrayKokkos <double> &elem_div,
                     const DViewCArrayKokkos <size_t> &elem_mat_id,
                     const DViewCArrayKokkos <double> &elem_statev,
                     const double rk_alpha,
                     const size_t cycle);

  void get_force_dgradient_sgh(const DCArrayKokkos <material_t> &material,
                     const mesh_t &mesh,
                     const DViewCArrayKokkos <double> &node_coords,
                     const DViewCArrayKokkos <double> &node_vel,
                     const DViewCArrayKokkos <double> &elem_den,
                     const DViewCArrayKokkos <double> &elem_sie,
                     const DViewCArrayKokkos <double> &elem_pres,
                     const DViewCArrayKokkos <double> &elem_stress,
                     const DViewCArrayKokkos <double> &elem_sspd,
                     const DViewCArrayKokkos <double> &elem_vol,
                     const DViewCArrayKokkos <double> &elem_div,
                     const DViewCArrayKokkos <size_t> &elem_mat_id,
                     const DViewCArrayKokkos <double> &elem_statev,
                     const double rk_alpha,
                     const size_t cycle);

  void force_design_gradient_term(const_vec_array design_variables, vec_array design_gradients);


  void get_force_sgh2D(const DCArrayKokkos <material_t> &material,
                       const mesh_t &mesh,
                       const DViewCArrayKokkos <double> &node_coords,
                       const DViewCArrayKokkos <double> &node_vel,
                       const DViewCArrayKokkos <double> &elem_den,
                       const DViewCArrayKokkos <double> &elem_sie,
                       const DViewCArrayKokkos <double> &elem_pres,
                       const DViewCArrayKokkos <double> &elem_stress,
                       const DViewCArrayKokkos <double> &elem_sspd,
                       const DViewCArrayKokkos <double> &elem_vol,
                       const DViewCArrayKokkos <double> &elem_div,
                       const DViewCArrayKokkos <size_t> &elem_mat_id,
                       DViewCArrayKokkos <double> &corner_force,
                       const DViewCArrayKokkos <double> &elem_statev,
                       const double rk_alpha,
                       const size_t cycle);
  
  void update_position_sgh(double rk_alpha,
                           const size_t num_nodes,
                           DViewCArrayKokkos <double> &node_coords,
                           const DViewCArrayKokkos <double> &node_vel);

  void get_vol();

  void init_assembly();

  KOKKOS_INLINE_FUNCTION
  void get_vol_hex(const DViewCArrayKokkos <double> &elem_vol,
                   const size_t elem_gid,
                   const DViewCArrayKokkos <double> &node_coords,
                   const ViewCArrayKokkos <size_t>  &elem_node_gids,
                   const size_t rk_level) const;


  KOKKOS_INLINE_FUNCTION
  void get_vol_quad(const DViewCArrayKokkos <double> &elem_vol,
                    const size_t elem_gid,
                    const DViewCArrayKokkos <double> &node_coords,
                    const ViewCArrayKokkos <size_t>  &elem_node_gids,
                    const size_t rk_level) const;


  KOKKOS_FUNCTION
  double get_area_quad(const size_t elem_gid,
                       const DViewCArrayKokkos <double> &node_coords,
                       const ViewCArrayKokkos <size_t>  &elem_node_gids,
                       const size_t rk_level) const;


  KOKKOS_FUNCTION
  void get_bmatrix(const ViewCArrayKokkos <double> &B_matrix,
                   const size_t elem_gid,
                   const DViewCArrayKokkos <double> &node_coords,
                   const ViewCArrayKokkos <size_t>  &elem_node_gids,
                   const size_t rk_level) const;


  KOKKOS_FUNCTION
  void get_bmatrix2D(const ViewCArrayKokkos <double> &B_matrix,
                     const size_t elem_gid,
                     const DViewCArrayKokkos <double> &node_coords,
                     const ViewCArrayKokkos <size_t>  &elem_node_gids,
                     const size_t rk_level) const;

  KOKKOS_FUNCTION
  void get_area_weights2D(const ViewCArrayKokkos <double> &corner_areas,
                          const size_t elem_gid,
                          const DViewCArrayKokkos <double> &node_coords,
                          const ViewCArrayKokkos <size_t>  &elem_node_gids,
                          const size_t rk_level) const;


  KOKKOS_INLINE_FUNCTION
  double heron(const double x1,
               const double y1,
               const double x2,
               const double y2,
               const double x3,
               const double y3) const;

  double average_element_density(const int nodes_per_elem, const CArray<double> current_element_densities) const;

  void get_divergence(DViewCArrayKokkos <double> &elem_div,
                      const mesh_t mesh,
                      const DViewCArrayKokkos <double> &node_coords,
                      const DViewCArrayKokkos <double> &node_vel,
                      const DViewCArrayKokkos <double> &elem_vol);


  void get_divergence2D(DViewCArrayKokkos <double> &elem_div,
                        const mesh_t mesh,
                        const DViewCArrayKokkos <double> &node_coords,
                        const DViewCArrayKokkos <double> &node_vel,
                        const DViewCArrayKokkos <double> &elem_vol);


  KOKKOS_FUNCTION
  void get_velgrad(ViewCArrayKokkos <double> &vel_grad,
                   const ViewCArrayKokkos <size_t>  &elem_node_gids,
                   const DViewCArrayKokkos <double> &node_vel,
                   const ViewCArrayKokkos <double> &b_matrix,
                   const double elem_vol,
                   const size_t elem_gid,
                   const size_t rk_level) const;


  KOKKOS_FUNCTION
  void get_velgrad2D(ViewCArrayKokkos <double> &vel_grad,
                     const ViewCArrayKokkos <size_t>  &elem_node_gids,
                     const DViewCArrayKokkos <double> &node_vel,
                     const ViewCArrayKokkos <double> &b_matrix,
                     const double elem_vol,
                     const double elem_area,
                     const size_t elem_gid,
                     const size_t rk_level) const;

  KOKKOS_INLINE_FUNCTION
  void decompose_vel_grad(ViewCArrayKokkos <double> &D_tensor,
                          ViewCArrayKokkos <double> &W_tensor,
                          const ViewCArrayKokkos <double> &vel_grad,
                          const ViewCArrayKokkos <size_t>  &elem_node_gids,
                          const size_t elem_gid,
                          const DViewCArrayKokkos <double> &node_coords,
                          const DViewCArrayKokkos <double> &node_vel,
                          const double vol) const;


  void update_velocity_sgh(double rk_alpha,
                           const mesh_t &mesh,
                           DViewCArrayKokkos <double> &node_vel,
                           const DViewCArrayKokkos <double> &node_mass,
                           const DViewCArrayKokkos <double> &corner_force);
  
  void tag_bdys(const DCArrayKokkos <boundary_t> &boundary,
                mesh_t &mesh,
                const DViewCArrayKokkos <double> &node_coords);

  void boundary_velocity(const mesh_t &mesh,
                         const DCArrayKokkos <boundary_t> &boundary,
                         DViewCArrayKokkos <double> &node_vel);
  
  KOKKOS_INLINE_FUNCTION 
  size_t check_bdy(const size_t patch_gid,
                   const int num_dim,
                   const int num_nodes_in_patch,
                   const int this_bc_tag,
                   const double val,
                   const DViewCArrayKokkos <double> &node_coords,
                   const size_t rk_level) const;

  void rk_init(DViewCArrayKokkos <double> &node_coords,
               DViewCArrayKokkos <double> &node_vel,
               DViewCArrayKokkos <double> &elem_sie,
               DViewCArrayKokkos <double> &elem_stress,
               const size_t num_elems,
               const size_t num_nodes);


  void get_timestep(mesh_t &mesh,
                    DViewCArrayKokkos <double> &node_coords,
                    DViewCArrayKokkos <double> &node_vel,
                    DViewCArrayKokkos <double> &elem_sspd,
                    DViewCArrayKokkos <double> &elem_vol);


  void get_timestep2D(mesh_t &mesh,
                      DViewCArrayKokkos <double> &node_coords,
                      DViewCArrayKokkos <double> &node_vel,
                      DViewCArrayKokkos <double> &elem_sspd,
                      DViewCArrayKokkos <double> &elem_vol);
  
  void update_energy_sgh(double rk_alpha,
                         const mesh_t &mesh,
                         const DViewCArrayKokkos <double> &node_vel,
                         const DViewCArrayKokkos <double> &node_coords,
                         DViewCArrayKokkos <double> &elem_sie,
                         const DViewCArrayKokkos <double> &elem_mass,
                         const DViewCArrayKokkos <double> &corner_force);
                   
  void update_state(const DCArrayKokkos <material_t> &material,
                    const mesh_t &mesh,
                    const DViewCArrayKokkos <double> &node_coords,
                    const DViewCArrayKokkos <double> &node_vel,
                    DViewCArrayKokkos <double> &elem_den,
                    DViewCArrayKokkos <double> &elem_pres,
                    DViewCArrayKokkos <double> &elem_stress,
                    DViewCArrayKokkos <double> &elem_sspd,
                    const DViewCArrayKokkos <double> &elem_sie,
                    const DViewCArrayKokkos <double> &elem_vol,
                    const DViewCArrayKokkos <double> &elem_mass,
                    const DViewCArrayKokkos <size_t> &elem_mat_id,
                    const DViewCArrayKokkos <double> &elem_statev,
                    const double rk_alpha,
                    const size_t cycle);


  void update_state2D(const DCArrayKokkos <material_t> &material,
                      const mesh_t &mesh,
                      const DViewCArrayKokkos <double> &node_coords,
                      const DViewCArrayKokkos <double> &node_vel,
                      DViewCArrayKokkos <double> &elem_den,
                      DViewCArrayKokkos <double> &elem_pres,
                      DViewCArrayKokkos <double> &elem_stress,
                      DViewCArrayKokkos <double> &elem_sspd,
                      const DViewCArrayKokkos <double> &elem_sie,
                      const DViewCArrayKokkos <double> &elem_vol,
                      const DViewCArrayKokkos <double> &elem_mass,
                      const DViewCArrayKokkos <size_t> &elem_mat_id,
                      const DViewCArrayKokkos <double> &elem_statev,
                      const double rk_alpha,
                      const size_t cycle);

  KOKKOS_INLINE_FUNCTION
  void user_eos_model(const DViewCArrayKokkos <double> &elem_pres,
                      const DViewCArrayKokkos <double> &elem_stress,
                      const size_t elem_gid,
                      const size_t mat_id,
                      const DViewCArrayKokkos <double> &elem_state_vars,
                      const DViewCArrayKokkos <double> &elem_sspd,
                      const double den,
                      const double sie) const;


  KOKKOS_INLINE_FUNCTION
  void user_strength_model(const DViewCArrayKokkos <double> &elem_pres,
                           const DViewCArrayKokkos <double> &elem_stress,
                           const size_t elem_gid,
                           const size_t mat_id,
                           const DViewCArrayKokkos <double> &elem_state_vars,
                           const DViewCArrayKokkos <double> &elem_sspd,
                           const double den,
                           const double sie,
                           const ViewCArrayKokkos <double> &vel_grad,
                           const ViewCArrayKokkos <size_t> &elem_node_gids,
                           const DViewCArrayKokkos <double> &node_coords,
                           const DViewCArrayKokkos <double> &node_vel,
                           const double vol,
                           const double rk_alpha) const;


  void user_model_init(const DCArrayKokkos <double> &file_state_vars,
                       const size_t num_state_vars,
                       const size_t mat_id,
                       const size_t num_elems);

  void build_boundry_node_sets(const DCArrayKokkos <boundary_t> &boundary, mesh_t &mesh);
  
  void init_boundaries();

  //initializes memory for arrays used in the global stiffness matrix assembly
  void init_boundary_sets(int num_boundary_sets);

  void grow_boundary_sets(int num_boundary_sets);

  virtual void update_forward_solve(Teuchos::RCP<const MV> zp);

  void comm_node_masses();

  void comm_adjoint_vectors(int cycle);

  void comm_variables(Teuchos::RCP<const MV> zp);

  void read_conditions_ansys_dat(std::ifstream *in, std::streampos before_condition_header);

  //interfaces between user input and creating data structures for bcs
  void generate_bcs();

  void Displacement_Boundary_Conditions();

  void init_output();

  void compute_output();
  
  void sort_output(Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > sorted_map);

  void collect_output(Teuchos::RCP<Tpetra::Map<LO,GO,node_type> > global_reduce_map);

  
  void write_outputs (const mesh_t &mesh,
                      DViewCArrayKokkos <double> &node_coords,
                      DViewCArrayKokkos <double> &node_vel,
                      DViewCArrayKokkos <double> &node_mass,
                      DViewCArrayKokkos <double> &elem_den,
                      DViewCArrayKokkos <double> &elem_pres,
                      DViewCArrayKokkos <double> &elem_stress,
                      DViewCArrayKokkos <double> &elem_sspd,
                      DViewCArrayKokkos <double> &elem_sie,
                      DViewCArrayKokkos <double> &elem_vol,
                      DViewCArrayKokkos <double> &elem_mass,
                      DViewCArrayKokkos <size_t> &elem_mat_id);


  void ensight(const mesh_t &mesh,
               const DViewCArrayKokkos <double> &node_coords,
               const DViewCArrayKokkos <double> &node_vel,
               const DViewCArrayKokkos <double> &node_mass,
               const DViewCArrayKokkos <double> &elem_den,
               const DViewCArrayKokkos <double> &elem_pres,
               const DViewCArrayKokkos <double> &elem_stress,
               const DViewCArrayKokkos <double> &elem_sspd,
               const DViewCArrayKokkos <double> &elem_sie,
               const DViewCArrayKokkos <double> &elem_vol,
               const DViewCArrayKokkos <double> &elem_mass,
               const DViewCArrayKokkos <size_t> &elem_mat_id);


  void state_file(const mesh_t &mesh,
                  const DViewCArrayKokkos <double> &node_coords,
                  const DViewCArrayKokkos <double> &node_vel,
                  const DViewCArrayKokkos <double> &node_mass,
                  const DViewCArrayKokkos <double> &elem_den,
                  const DViewCArrayKokkos <double> &elem_pres,
                  const DViewCArrayKokkos <double> &elem_stress,
                  const DViewCArrayKokkos <double> &elem_sspd,
                  const DViewCArrayKokkos <double> &elem_sie,
                  const DViewCArrayKokkos <double> &elem_vol,
                  const DViewCArrayKokkos <double> &elem_mass,
                  const DViewCArrayKokkos <size_t> &elem_mat_id);

  void node_density_constraints(host_vec_array node_densities_lower_bound);

  void compute_topology_optimization_adjoint(); //Force does not depend on node coords and velocity

  void compute_topology_optimization_adjoint_full(); //Force depends on node coords and velocity

  void compute_topology_optimization_gradient(const_vec_array design_densities, vec_array gradients);

  void compute_topology_optimization_gradient_full(const_vec_array design_variables, vec_array design_gradients, const_host_vec_array host_design_variables, host_vec_array host_design_gradients);

  //elastic TO stuff
  void Element_Material_Properties(size_t ielem, real_t &Element_Modulus, real_t &Poisson_Ratio, real_t density);

  void compute_stiffness_gradients(const_host_vec_array design_densities, host_vec_array gradients);

  void Gradient_Element_Material_Properties(size_t ielem, real_t &Element_Modulus, real_t &Poisson_Ratio, real_t density);

  void local_matrix_multiply(int ielem, CArrayKokkos<real_t, array_layout, device_type, memory_traits> &Local_Matrix);
  
  void assemble_matrix();
  
  bool nodal_density_flag;
  real_t penalty_power;
  Teuchos::RCP<MAT> Global_Stiffness_Matrix;
  RaggedRightArrayKokkos<real_t, Kokkos::LayoutRight, device_type, memory_traits, array_layout> Stiffness_Matrix;
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Stiffness_Matrix_Strides;
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Global_Stiffness_Matrix_Assembly_Map;
  //end elastic TO data
  
  Simulation_Parameters_SGH *simparam;
  Simulation_Parameters_Elasticity *simparam_elasticity;
  Simulation_Parameters_Dynamic_Optimization *simparam_dynamic_opt;
  Explicit_Solver_SGH *Explicit_Solver_Pointer_;

  elements::ref_element  *ref_elem;
  
  mesh_t& mesh;
  //shallow copies of mesh class views
  size_t num_nodes_in_elem;
  // corner ids in node
  RaggedRightArrayKokkos <size_t> corners_in_node;
  CArrayKokkos <size_t> num_corners_in_node;
    
  // elem ids in node
  RaggedRightArrayKokkos <size_t> elems_in_node;
    
  // node ids in node
  RaggedRightArrayKokkos <size_t> nodes_in_node;
  CArrayKokkos <size_t> num_nodes_in_node;
    
  // node ids in elem
  DCArrayKokkos <size_t> nodes_in_elem;
    
  // corner ids in elem
  CArrayKokkos <size_t> corners_in_elem;
    
  // elem ids in elem
  RaggedRightArrayKokkos <size_t> elems_in_elem;
  CArrayKokkos <size_t> num_elems_in_elem;
    
  // patch ids in elem
  CArrayKokkos <size_t> patches_in_elem;
    
  // node ids in a patch
  CArrayKokkos <size_t> nodes_in_patch;
    
  // element ids in a patch
  CArrayKokkos <size_t> elems_in_patch;

  // patch ids in bdy set
  size_t num_bdy_sets;
  DynamicRaggedRightArrayKokkos <size_t> bdy_patches_in_set;
  
  // bdy nodes
  CArrayKokkos <size_t> bdy_nodes;

  // node ids in bdy_patch set
  RaggedRightArrayKokkos <size_t> bdy_nodes_in_set;
  DCArrayKokkos <size_t> num_bdy_nodes_in_set;
  
  //Topology optimization filter variable
  DCArrayKokkos<double> relative_element_densities;
  
  //Local FEA data
  host_elem_conn_array interface_nodes_in_elem; //host view of element connectivity to nodes

  //Global FEA data
  Teuchos::RCP<MV> node_velocities_distributed;
  Teuchos::RCP<MV> initial_node_coords_distributed;
  Teuchos::RCP<MV> all_initial_node_coords_distributed;
  Teuchos::RCP<MV> initial_node_velocities_distributed;
  Teuchos::RCP<MV> all_node_velocities_distributed;
  Teuchos::RCP<MV> all_cached_node_velocities_distributed;
  Teuchos::RCP<MV> node_masses_distributed;
  Teuchos::RCP<MV> ghost_node_masses_distributed;
  Teuchos::RCP<MV> adjoint_vector_distributed;
  Teuchos::RCP<MV> phi_adjoint_vector_distributed;
  Teuchos::RCP<std::vector<Teuchos::RCP<MV>>> forward_solve_velocity_data;
  Teuchos::RCP<std::vector<Teuchos::RCP<MV>>> forward_solve_coordinate_data;
  Teuchos::RCP<std::vector<Teuchos::RCP<MV>>> adjoint_vector_data;
  Teuchos::RCP<std::vector<Teuchos::RCP<MV>>> phi_adjoint_vector_data;
  Teuchos::RCP<MV> force_gradient_design;
  Teuchos::RCP<MV> force_gradient_position;
  Teuchos::RCP<MV> force_gradient_velocity;

  //Local FEA data
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Global_Gradient_Matrix_Assembly_Map;
  RaggedRightArrayKokkos<GO, array_layout, device_type, memory_traits> Graph_Matrix; //stores global indices
  RaggedRightArrayKokkos<GO, array_layout, device_type, memory_traits> DOF_Graph_Matrix; //stores global indices
  RaggedRightArrayKokkos<real_t, Kokkos::LayoutRight, device_type, memory_traits, array_layout> Force_Gradient_Positions;
  RaggedRightArrayKokkos<real_t, Kokkos::LayoutRight, device_type, memory_traits, array_layout> Force_Gradient_Velocities;
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Gradient_Matrix_Strides;
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Graph_Matrix_Strides;
  RaggedRightArrayKokkos<real_t, array_layout, device_type, memory_traits> Original_Gradient_Entries;
  RaggedRightArrayKokkos<LO, array_layout, device_type, memory_traits> Original_Gradient_Entry_Indices;
  DCArrayKokkos<size_t, array_layout, device_type, memory_traits> Original_Gradient_Entries_Strides;

  //distributed matrices
  Teuchos::RCP<MAT> distributed_force_gradient_positions;
  Teuchos::RCP<MAT> distributed_force_gradient_velocities;

  std::vector<real_t> time_data;
  int max_time_steps, last_time_step;

  //Dual View wrappers
  // Dual Views of the individual node struct variables
  DViewCArrayKokkos <double> node_coords;
  DViewCArrayKokkos <double> node_vel;
  DViewCArrayKokkos <double> node_mass;
             
  // Dual Views of the individual elem struct variables
  DViewCArrayKokkos <double> elem_den;
  DViewCArrayKokkos <double> elem_pres;
  DViewCArrayKokkos <double> elem_stress; // always 3D even in 2D-RZ
  DViewCArrayKokkos <double> elem_sspd;
  DViewCArrayKokkos <double> elem_sie;
  DViewCArrayKokkos <double> elem_vol;
  DViewCArrayKokkos <double> elem_div;    
  DViewCArrayKokkos <double> elem_mass;
  DViewCArrayKokkos <size_t> elem_mat_id;
  DViewCArrayKokkos <double> elem_statev;

  // Element velocity gradient 
  DCArrayKokkos <double> elem_vel_grad;

  // for storing global variables used in user material model
  DCArrayKokkos <double> global_vars;

  // Dual Views of the corner struct variables
  DViewCArrayKokkos <double> corner_force;
  DViewCArrayKokkos <double> corner_mass;
  
  //Boundary Conditions Data
  DCArrayKokkos<size_t> Local_Index_Boundary_Patches;
  //CArray <Nodal_Combination> Patch_Nodes;
  enum bc_type {NONE, POINT_LOADING_CONDITION, LINE_LOADING_CONDITION, SURFACE_LOADING_CONDITION};
  
  //Boundary Conditions Data
  int max_boundary_sets;

  //output dof data
  //Global arrays with collected data used to print
  int output_velocity_index, output_strain_index, output_stress_index;
  
  //file parameters
  DCArrayKokkos <size_t>read_from_file;
  DCArrayKokkos <double>file_state_vars;

  //parameters
  double time_value, time_final, dt, dt_max, dt_min, dt_cfl, graphics_time, graphics_dt_ival;
  size_t graphics_cyc_ival, cycle_stop, rk_num_stages, graphics_id;
  double fuzz, tiny, small;
  CArray <double> graphics_times;

  //optimization flags
  bool kinetic_energy_objective;
};

#endif // end HEADER_H
