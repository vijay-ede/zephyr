# The coverage linker flag is specific for clang.
if(CONFIG_COVERAGE_NATIVE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
elseif(CONFIG_COVERAGE_NATIVE_SOURCE)
  set_property(TARGET linker PROPERTY coverage -fprofile-instr-generate -fcoverage-mapping)
elseif(CONFIG_COVERAGE_LLVM_SOURCE)
  # For bare-metal LLVM source-based coverage, the libclang_rt.profile.a
  # runtime is linked explicitly by subsys/testsuite/coverage_llvm/CMakeLists.txt.
  # The linker driver flags (-fprofile-instr-generate -fcoverage-mapping) are
  # not needed at link time for bare-metal targets; the compiler flags are
  # sufficient to emit the instrumentation sections.
  set_property(TARGET linker PROPERTY coverage)
endif()

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY ld_extra_warning_options ${LINKERFLAGPREFIX},--fatal-warnings)

# GNU ld and LLVM lld complains when used with llvm/clang:
#   error: section: init_array is not contiguous with other relro sections
#
# So do not create RELRO program header.
set_property(TARGET linker APPEND PROPERTY cpp_base ${LINKERFLAGPREFIX},-z,norelro)
