#include "utilities.h"
#include "Simulation_Parameters_Topology_Optimization.h"
#include "elements.h"
#include "swage.h"

using namespace utils;

Simulation_Parameters_Topology_Optimization::Simulation_Parameters_Topology_Optimization(){

  //initialize data and flags to defaults
  report_runtime_flag = false;
  nodal_density_flag = true;
  penalty_power = 3;
  nTO_modules = 0;
}

Simulation_Parameters_Topology_Optimization::~Simulation_Parameters_Topology_Optimization(){
}

void Simulation_Parameters_Topology_Optimization::input(){
  Simulation_Parameters::input();
  //Simulation_Parameters::input();
  //initial buffer size for TO module list storage
  int buffer_size = 10;
  TO_Module_List = std::vector<std::string>(buffer_size);
  TO_Function_Type = std::vector<function_type>(buffer_size);
  Function_Arguments = std::vector<std::vector<real_t>>(buffer_size);
  //use pushback to add arguments for each TO module
  
  //TO objectives and constraints
  //TO_Module_List[0] = "Strain_Energy_Minimize";
  TO_Module_List[0] = "Heat_Capacity_Potential_Minimize";
  TO_Function_Type[0] = OBJECTIVE;
  nTO_modules++;
  TO_Module_List[1] = "Strain_Energy_Constraint";
  TO_Function_Type[1] = INEQUALITY_CONSTRAINT;
  Function_Arguments[1].push_back(0);
  Function_Arguments[1].push_back(8);
  TO_Module_List[1] = "Mass_Constraint";
  TO_Function_Type[1] = EQUALITY_CONSTRAINT;
  Function_Arguments[1].push_back(0.12);
  nTO_modules++;
  //example for later
  if(nTO_modules==buffer_size){
    buffer_size += 10;
    TO_Module_List.resize(buffer_size);
  }

  //simulation spatial dimension
  num_dim = 3;

  //polynomial interpolation order
  p_order = 0;

  //debug and performance report flags
  report_runtime_flag = true;

  //Topology Optimization flags
  nodal_density_flag = true;

  //Topology Optimization parameters
  penalty_power = 3;

  // ---- boundary conditions ---- //
  NB = 6; // number of boundaries
  NBD = 2; //number of surface sets used to specify a fixed density

}


void Simulation_Parameters_Topology_Optimization::FEA_module_setup(){
  
  //initial buffer size for FEA module list storage
  int buffer_size = 10;
  FEA_Module_List = std::vector<std::string>(buffer_size);
  TO_Module_My_FEA_Module = std::vector<int>(buffer_size);
  
  for(int imodule = 0; imodule < nTO_modules; imodule++){
    bool module_found = false;
    //decides which FEA modules to setup based on user decided TO problem
    //automate selection list later; use std::map maybe?
    if(TO_Module_List[imodule] == "Strain_Energy_Minimize"){
      //check if module type was already allocated
      for(int ifea = 0; ifea < nfea_modules; ifea++){
        if(FEA_Module_List[ifea] == "Elasticity"){
          module_found = true;
          TO_Module_My_FEA_Module[imodule] = ifea;
        }
      }
      if(!module_found){
        TO_Module_My_FEA_Module[imodule] = nfea_modules;
        FEA_Module_List[nfea_modules++] = "Elasticity";
        module_found = true;
      }
    }
    if(TO_Module_List[imodule] == "Heat_Capacity_Potential_Minimize"){
      //check if module type was already allocated
      for(int ifea = 0; ifea < nfea_modules; ifea++){
        if(FEA_Module_List[ifea] == "Heat_Conduction"){
          module_found = true;
          TO_Module_My_FEA_Module[imodule] = ifea;
        }
      }
      if(!module_found){
        TO_Module_My_FEA_Module[imodule] = nfea_modules;
        FEA_Module_List[nfea_modules++] = "Heat_Conduction";
        module_found = true;
      }
    }
    if(TO_Module_List[imodule] == "Mass_Constraint"){
      //check if module type was already allocated
      for(int ifea = 0; ifea < nfea_modules; ifea++){
        if(FEA_Module_List[ifea] == "Elasticity"){
          module_found = true;
          TO_Module_My_FEA_Module[imodule] = ifea;
        }
      }
      if(!module_found){
        TO_Module_My_FEA_Module[imodule] = nfea_modules;
        FEA_Module_List[nfea_modules++] = "Elasticity";
        module_found = true;
      }
    }
    if(TO_Module_List[imodule] == "Moment_of_Inertia_Constraint"){
      //check if module type was already allocated
      for(int ifea = 0; ifea < nfea_modules; ifea++){
        if(FEA_Module_List[ifea] == "Elasticity"){
          module_found = true;
          TO_Module_My_FEA_Module[imodule] = ifea;
        }
      }
      if(!module_found){
        TO_Module_My_FEA_Module[imodule] = nfea_modules;
        FEA_Module_List[nfea_modules++] = "Elasticity";
        module_found = true;
      }
    }
    if(TO_Module_List[imodule] == "Strain_Energy_Constraint"){
      //check if module type was already allocated
      for(int ifea = 0; ifea < nfea_modules; ifea++){
        if(FEA_Module_List[ifea] == "Elasticity"){
          module_found = true;
          TO_Module_My_FEA_Module[imodule] = ifea;
        }
      }
      if(!module_found){
        TO_Module_My_FEA_Module[imodule] = nfea_modules;
        FEA_Module_List[nfea_modules++] = "Elasticity";
        module_found = true;
      }
    }
    
    if(module_found){
      if(nfea_modules==buffer_size){
        buffer_size += 10;
        FEA_Module_List.resize(buffer_size);
        TO_Module_My_FEA_Module.resize(buffer_size);
      }
    }
  }
}