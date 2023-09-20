#pragma once
#include "yaml-serializable.h"

SERIALIZABLE_ENUM(EOS_MODEL, none, ideal_gas, user_eos_model)
SERIALIZABLE_ENUM(STRENGTH_MODEL, none, ideal_gas, user_strength_model)
SERIALIZABLE_ENUM(STRENGTH_TYPE, none, hypo, hyper)
SERIALIZABLE_ENUM(RUN_LOCATION, device, host)

struct material_t {
    EOS_MODEL eos_model                = EOS_MODEL::none;
    STRENGTH_MODEL strength_model      = STRENGTH_MODEL::none;
    STRENGTH_TYPE strength_type        = STRENGTH_TYPE::none;
    RUN_LOCATION strength_run_location = RUN_LOCATION::device;
    RUN_LOCATION eos_run_location      = RUN_LOCATION::device; 

    double q1;
    double q2;
    double q1ex;
    double q2ex;
    
    // Non-serialized fields
    size_t num_global_vars = 0;
};

struct Material : Yaml::DerivedFields, material_t {
    std::string id;
    std::vector<double> global_vars;

    void derive() {
        num_global_vars = global_vars.size();
    }
};
YAML_ADD_REQUIRED_FIELDS_FOR(Material, id)
IMPL_YAML_SERIALIZABLE_FOR(Material, 
    id, eos_model, strength_model, strength_type,
    strength_run_location, eos_run_location,
    q1, q2, q1ex, q2ex, 
    global_vars
)