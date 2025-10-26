# Install script for directory: C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Ceres")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/llvm-objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ceres" TYPE FILE FILES
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/autodiff_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/autodiff_first_order_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/autodiff_manifold.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/c_api.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/ceres.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/conditioned_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/context.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/cost_function_to_functor.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/covariance.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/crs_matrix.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/cubic_interpolation.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/dynamic_autodiff_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/dynamic_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/dynamic_cost_function_to_functor.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/dynamic_numeric_diff_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/evaluation_callback.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/first_order_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/gradient_checker.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/gradient_problem.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/gradient_problem_solver.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/iteration_callback.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/jet.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/jet_fwd.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/line_manifold.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/loss_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/manifold.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/manifold_test_utils.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/normal_prior.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/numeric_diff_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/numeric_diff_first_order_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/numeric_diff_options.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/ordered_groups.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/problem.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/product_manifold.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/rotation.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/sized_cost_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/solver.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/sphere_manifold.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/tiny_solver.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/tiny_solver_autodiff_function.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/tiny_solver_cost_function_adapter.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/types.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/version.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ceres/internal" TYPE FILE FILES
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/array_selector.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/autodiff.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/disable_warnings.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/eigen.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/fixed_array.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/householder_vector.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/integer_sequence_algorithm.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/jet_traits.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/line_parameterization.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/memory.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/numeric_diff.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/parameter_dims.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/port.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/reenable_warnings.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/sphere_manifold_functions.h"
    "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/include/ceres/internal/variadic_evaluate.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "C:/Users/lizar/Documents/ValveWorkbench/build/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ceres/internal/miniglog/glog" TYPE FILE FILES "C:/Users/lizar/Documents/ValveWorkbench/external/ceres/ceres-solver/internal/ceres/miniglog/glog/logging.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake"
         "C:/Users/lizar/Documents/ValveWorkbench/build/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres/CeresTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "C:/Users/lizar/Documents/ValveWorkbench/build/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "C:/Users/lizar/Documents/ValveWorkbench/build/CMakeFiles/Export/9a3bb6344a10c987f9c537d2a0e39364/CeresTargets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE RENAME "CeresConfig.cmake" FILES "C:/Users/lizar/Documents/ValveWorkbench/build/CeresConfig-install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Ceres" TYPE FILE FILES "C:/Users/lizar/Documents/ValveWorkbench/build/CeresConfigVersion.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/lizar/Documents/ValveWorkbench/build/internal/ceres/cmake_install.cmake")
  include("C:/Users/lizar/Documents/ValveWorkbench/build/examples/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/lizar/Documents/ValveWorkbench/build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/lizar/Documents/ValveWorkbench/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
