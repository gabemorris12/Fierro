#!/usr/bin/python3

import os
import os.path
import sys
import math


executables = []
tests = []



# Add paths to all tested executables
sgh_executable = "./../../build-fierro-openmp/bin/fierro-parallel-explicit"
sgh_tests = ["Noh", "Sedov", "Sod"]

# Check that each executable exists
if not os.path.exists(sgh_executable):
    print("This test script assumes that the fierro-parallel-explicit code exists ")
    print("and has been build in the build-fierro-openmp directory")
    print("The executable could not be found")
    raise ValueError("fierro-parallel-explicit executable not found")


# Add all executables to list
executables.append(sgh_executable)



inputs = []
standard_results = []

tests.append(sgh_tests)


position_keyword = "POINTS"


for i in range(len(tests[0])):
    inputs.append("Solver-Inputs/SGH_"+tests[0][i]+"_simple.yaml")
    standard_results.append("standard-results/SGH/"+tests[0][i]+"/vtk/data/VTK0.vtk")


# Extract vector valued data from vtk output file
def extract_vector_data(filename, keyword):
    data = []
    found_keyword = False

    with open(filename, 'r') as file:
        for line in file:
            if found_keyword:
                if line.strip():  # If the line is not blank
                    # Split each line into fields.
                    fields = line.split()
                    xx = float(fields[0])
                    yy = float(fields[1])
                    zz = float(fields[2])
                    # Group and append to results.
                    vec = [xx, yy, zz]
                    data.append(vec)
                else:
                    break  # Exit the loop when encountering a blank line
            elif keyword in line:
                found_keyword = True

    return data

# Extract scalar valued data from vtk output file
def extract_scalar_data(filename, keyword):
    data = []
    found_keyword = False

    with open(filename, 'r') as file:
        for line in file:
            if found_keyword:
                if line.strip():  # If the line is not blank
                    fields = line.split()
                    if fields[0] != "LOOKUP_TABLE":
                        data.append(float(fields[0]))
                else:
                    break  # Exit the loop when encountering a blank line
            elif keyword in line:
                found_keyword = True
                # file.next() # skip first line "LOOKUP_TABLE"
 
    return data

# Calculate the percent difference between two arrays of scalars
def percent_difference_scalars(array1, array2):
    if len(array1) != len(array2):
        raise ValueError("Arrays must have the same length")

    percent_diff = []
    for i in range(len(array1)):
        diff = array2[i] - array1[i]
        percent_diff.append((diff / array1[i]) * 100 if array1[i] != 0 else 0)

    return percent_diff

# Calculate the percent difference between two arrays of vectors
def percent_difference_vectors(array1, array2):
    if len(array1) != len(array2):
        raise ValueError("Arrays must have the same length")

    percent_diff = []
    for i in range(len(array1)):
        if len(array1[i]) != 3 or len(array2[i]) != 3:
            raise ValueError("Subarrays must have length 3")
        
        diff = [array2[i][j] - array1[i][j] for j in range(3)]
        percent_diff.append([(diff[j] / array1[i][j]) * 100 if array1[i][j] != 0 else 0 for j in range(3)])

    percent_diff_mag = []
    for i in range(len(array1)):
        percent_diff_mag.append(magnitude(percent_diff[i]))

    return percent_diff_mag

# Calculate the magnitude of a vector
def magnitude(array):
    mag = math.sqrt(sum(x**2 for x in array))
    return mag

# Run each test
for i in range(len(tests[0])):
    
    # Run simulation
    print("Running "+tests[0][i])
    os.system(executables[0] + ' ' + inputs[i])

    GT_positions = extract_vector_data(standard_results[i], position_keyword)

    # Read simulation results
    results_filename = "vtk/data/VTK0.vtk"

    results_positions = extract_vector_data(results_filename, position_keyword)
    position_diff = percent_difference_vectors(GT_positions, results_positions)


    for i in range(len(position_diff)):
        if position_diff[i] >= 1.0e-6:
            raise ValueError(" ****************** ERROR: Position difference out of range for "+tests[i]+" problem ****************** ")

    print("Removing simulation outputs")
    os.system('rm -rf  vtk' )