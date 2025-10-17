Package: glog:x64-windows@0.7.1

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35217.0
- CMake Version: 3.31.6
-    vcpkg-tool version: 2025-09-03-4580816534ed8fd9634ac83d46471440edd82dfe
    vcpkg-scripts version: unknown

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/google/glog/archive/v0.7.1.tar.gz -> google-glog-v0.7.1.tar.gz
Successfully downloaded google-glog-v0.7.1.tar.gz
-- Extracting source C:/Users/lizar/.vcpkg/downloads/google-glog-v0.7.1.tar.gz
-- Applying patch fix_glog_CMAKE_MODULE_PATH.patch
-- Applying patch glog_disable_debug_postfix.patch
-- Applying patch fix_crosscompile_symbolize.patch
-- Applying patch fix_cplusplus_macro.patch
-- Using source at C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean
-- Found external ninja('1.12.1').
-- Configuring x64-windows
CMake Error at scripts/cmake/vcpkg_execute_required_process.cmake:127 (message):
    Command failed: "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v
    Working Directory: C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/vcpkg-parallel-configure
    Error code: 1
    See logs for more information:
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-CMakeCache.txt.log
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-rel-CMakeCache.txt.log
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-CMakeConfigureLog.yaml.log
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-rel-CMakeConfigureLog.yaml.log
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-ninja.log
      C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-out.log

Call Stack (most recent call first):
  C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed/x64-windows/share/vcpkg-cmake/vcpkg_cmake_configure.cmake:269 (vcpkg_execute_required_process)
  C:/Users/lizar/AppData/Local/vcpkg/registries/git-trees/da85ebb84a5ed38130c760c9d5f9b2d598b39e2e/portfile.cmake:24 (vcpkg_cmake_configure)
  scripts/ports.cmake:206 (include)



```

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-out.log</summary>

```
[1/2] "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe" -E chdir ".." "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe" "C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean" "-G" "Ninja" "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_PREFIX=C:/Users/lizar/.vcpkg/packages/glog_x64-windows" "-DFETCHCONTENT_FULLY_DISCONNECTED=ON" "-DBUILD_TESTING=OFF" "-DWITH_UNWIND=OFF" "-DWITH_CUSTOM_PREFIX=OFF" "-DCMAKE_DISABLE_FIND_PACKAGE_Unwind=ON" "-DCMAKE_MAKE_PROGRAM=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" "-DBUILD_SHARED_LIBS=ON" "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows" "-DVCPKG_SET_CHARSET_FLAG=ON" "-DVCPKG_PLATFORM_TOOLSET=v143" "-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON" "-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON" "-DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=ON" "-DCMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP=TRUE" "-DCMAKE_VERBOSE_MAKEFILE=ON" "-DVCPKG_APPLOCAL_DEPS=OFF" "-DCMAKE_TOOLCHAIN_FILE=C:/Users/lizar/.vcpkg/scripts/buildsystems/vcpkg.cmake" "-DCMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION=ON" "-DVCPKG_CXX_FLAGS=" "-DVCPKG_CXX_FLAGS_RELEASE=" "-DVCPKG_CXX_FLAGS_DEBUG=" "-DVCPKG_C_FLAGS=" "-DVCPKG_C_FLAGS_RELEASE=" "-DVCPKG_C_FLAGS_DEBUG=" "-DVCPKG_CRT_LINKAGE=dynamic" "-DVCPKG_LINKER_FLAGS=" "-DVCPKG_LINKER_FLAGS_RELEASE=" "-DVCPKG_LINKER_FLAGS_DEBUG=" "-DVCPKG_TARGET_ARCHITECTURE=x64" "-DCMAKE_INSTALL_LIBDIR:STRING=lib" "-DCMAKE_INSTALL_BINDIR:STRING=bin" "-D_VCPKG_ROOT_DIR=C:/Users/lizar/.vcpkg" "-D_VCPKG_INSTALLED_DIR=C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed" "-DVCPKG_MANIFEST_INSTALL=OFF"
FAILED: ../CMakeCache.txt 
...
Skipped 90 lines
...

    C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-k3n11d/cmTC_0a88f.exe

  could not be removed:

    The process cannot access the file because it is being used by another process.

Call Stack (most recent call first):
  C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CheckCXXSourceRuns.cmake:52 (cmake_check_source_runs)
  CMakeLists.txt:252 (check_cxx_source_runs)


-- Performing Test HAVE_SYMBOLIZE - Success
-- Looking for gmtime_r
-- Looking for gmtime_r - not found
-- Looking for localtime_r
-- Looking for localtime_r - not found
-- Performing Test COMPILER_HAS_DEPRECATED_ATTR
-- Performing Test COMPILER_HAS_DEPRECATED_ATTR - Failed
-- Performing Test COMPILER_HAS_DEPRECATED
-- Performing Test COMPILER_HAS_DEPRECATED - Success
-- Configuring incomplete, errors occurred!
[2/2] "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe" -E chdir "../../x64-windows-dbg" "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe" "C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean" "-G" "Ninja" "-DCMAKE_BUILD_TYPE=Debug" "-DCMAKE_INSTALL_PREFIX=C:/Users/lizar/.vcpkg/packages/glog_x64-windows/debug" "-DFETCHCONTENT_FULLY_DISCONNECTED=ON" "-DBUILD_TESTING=OFF" "-DWITH_UNWIND=OFF" "-DWITH_CUSTOM_PREFIX=OFF" "-DCMAKE_DISABLE_FIND_PACKAGE_Unwind=ON" "-DCMAKE_MAKE_PROGRAM=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" "-DBUILD_SHARED_LIBS=ON" "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows" "-DVCPKG_SET_CHARSET_FLAG=ON" "-DVCPKG_PLATFORM_TOOLSET=v143" "-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON" "-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON" "-DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=ON" "-DCMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP=TRUE" "-DCMAKE_VERBOSE_MAKEFILE=ON" "-DVCPKG_APPLOCAL_DEPS=OFF" "-DCMAKE_TOOLCHAIN_FILE=C:/Users/lizar/.vcpkg/scripts/buildsystems/vcpkg.cmake" "-DCMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION=ON" "-DVCPKG_CXX_FLAGS=" "-DVCPKG_CXX_FLAGS_RELEASE=" "-DVCPKG_CXX_FLAGS_DEBUG=" "-DVCPKG_C_FLAGS=" "-DVCPKG_C_FLAGS_RELEASE=" "-DVCPKG_C_FLAGS_DEBUG=" "-DVCPKG_CRT_LINKAGE=dynamic" "-DVCPKG_LINKER_FLAGS=" "-DVCPKG_LINKER_FLAGS_RELEASE=" "-DVCPKG_LINKER_FLAGS_DEBUG=" "-DVCPKG_TARGET_ARCHITECTURE=x64" "-DCMAKE_INSTALL_LIBDIR:STRING=lib" "-DCMAKE_INSTALL_BINDIR:STRING=bin" "-D_VCPKG_ROOT_DIR=C:/Users/lizar/.vcpkg" "-D_VCPKG_INSTALLED_DIR=C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed" "-DVCPKG_MANIFEST_INSTALL=OFF"
-- The CXX compiler identification is MSVC 19.44.35217.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Could NOT find GTest (missing: GTest_DIR)
-- Looking for gflags namespace
-- Looking for gflags namespace - gflags
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Failed
-- Looking for pthread_create in pthreads
-- Looking for pthread_create in pthreads - not found
-- Looking for pthread_create in pthread
-- Looking for pthread_create in pthread - not found
-- Found Threads: TRUE
-- Looking for C++ include dlfcn.h
-- Looking for C++ include dlfcn.h - not found
-- Looking for C++ include elf.h
-- Looking for C++ include elf.h - not found
-- Looking for C++ include glob.h
-- Looking for C++ include glob.h - not found
-- Looking for C++ include link.h
-- Looking for C++ include link.h - not found
-- Looking for C++ include pwd.h
-- Looking for C++ include pwd.h - not found
-- Looking for C++ include sys/exec_elf.h
-- Looking for C++ include sys/exec_elf.h - not found
-- Looking for C++ include sys/syscall.h
-- Looking for C++ include sys/syscall.h - not found
-- Looking for C++ include sys/time.h
-- Looking for C++ include sys/time.h - not found
-- Looking for C++ include sys/types.h
-- Looking for C++ include sys/types.h - found
-- Looking for C++ include sys/utsname.h
-- Looking for C++ include sys/utsname.h - not found
-- Looking for C++ include sys/wait.h
-- Looking for C++ include sys/wait.h - not found
-- Looking for C++ include syscall.h
-- Looking for C++ include syscall.h - not found
-- Looking for C++ include syslog.h
-- Looking for C++ include syslog.h - not found
-- Looking for C++ include ucontext.h
-- Looking for C++ include ucontext.h - not found
-- Looking for C++ include unistd.h
-- Looking for C++ include unistd.h - not found
-- Looking for C++ include stdint.h
-- Looking for C++ include stdint.h - found
-- Looking for C++ include stddef.h
-- Looking for C++ include stddef.h - found
-- Check size of mode_t
-- Check size of mode_t - failed
-- Check size of ssize_t
-- Check size of ssize_t - failed
-- Looking for dladdr
-- Looking for dladdr - not found
-- Looking for fcntl
-- Looking for fcntl - not found
-- Looking for posix_fadvise
-- Looking for posix_fadvise - not found
-- Looking for pread
-- Looking for pread - not found
-- Looking for pwrite
-- Looking for pwrite - not found
-- Looking for sigaction
-- Looking for sigaction - not found
-- Looking for sigaltstack
-- Looking for sigaltstack - not found
-- Looking for backtrace
-- Looking for backtrace - not found
-- Looking for backtrace_symbols
-- Looking for backtrace_symbols - not found
-- Looking for _chsize_s
-- Looking for _chsize_s - found
-- Looking for UnDecorateSymbolName
-- Looking for UnDecorateSymbolName - found
-- Looking for abi::__cxa_demangle
-- Looking for abi::__cxa_demangle - not found
-- Looking for __argv
-- Looking for __argv - found
-- Looking for getprogname
-- Looking for getprogname - not found
-- Looking for program_invocation_short_name
-- Looking for program_invocation_short_name - not found
-- Performing Test HAVE___PROGNAME
-- Performing Test HAVE___PROGNAME - Failed
-- Performing Test HAVE_SYMBOLIZE
-- Performing Test HAVE_SYMBOLIZE - Success
-- Looking for gmtime_r
-- Looking for gmtime_r - not found
-- Looking for localtime_r
-- Looking for localtime_r - not found
-- Performing Test COMPILER_HAS_DEPRECATED_ATTR
-- Performing Test COMPILER_HAS_DEPRECATED_ATTR - Failed
-- Performing Test COMPILER_HAS_DEPRECATED
-- Performing Test COMPILER_HAS_DEPRECATED - Success
-- Configuring done (17.0s)
-- Generating done (0.1s)
CMake Warning:
  Manually-specified variables were not used by the project:

    FETCHCONTENT_FULLY_DISCONNECTED
    WITH_CUSTOM_PREFIX
    _VCPKG_ROOT_DIR


-- Build files have been written to: C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg
ninja: build stopped: subcommand failed.
```
</details>

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-rel-CMakeCache.txt.log</summary>

```
# This is the CMakeCache file.
# For build in directory: c:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel
# It was generated by CMake: C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe
# You can edit this file to change values found and used by cmake.
# If you do not want to change any of the values, simply exit the editor.
# If you do want to change a value, simply edit, save, and exit the editor.
# The syntax for the file is as follows:
# KEY:TYPE=VALUE
# KEY is the name of a variable in the cache.
# TYPE is a hint to GUIs for the type of VALUE, DO NOT EDIT TYPE!.
# VALUE is the current value for the KEY.

########################
# EXTERNAL cache entries
########################

//Build shared libraries
BUILD_SHARED_LIBS:BOOL=ON

//Build the testing tree.
BUILD_TESTING:BOOL=OFF

//Path to a program.
CMAKE_AR:FILEPATH=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/lib.exe

//Choose the type of build, options are: None Debug Release RelWithDebInfo
// MinSizeRel ...
CMAKE_BUILD_TYPE:STRING=Release

CMAKE_CROSSCOMPILING:STRING=OFF

//CXX compiler
CMAKE_CXX_COMPILER:FILEPATH=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe

CMAKE_CXX_FLAGS:STRING=' /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP '

CMAKE_CXX_FLAGS_DEBUG:STRING='/MDd /Z7 /Ob0 /Od /RTC1 '

//Flags used by the CXX compiler during MINSIZEREL builds.
CMAKE_CXX_FLAGS_MINSIZEREL:STRING=/O1 /Ob1 /DNDEBUG

CMAKE_CXX_FLAGS_RELEASE:STRING='/MD /O2 /Oi /Gy /DNDEBUG /Z7 '

//Flags used by the CXX compiler during RELWITHDEBINFO builds.
CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=/Zi /O2 /Ob1 /DNDEBUG

//Libraries linked by default with all C++ applications.
CMAKE_CXX_STANDARD_LIBRARIES:STRING=kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib

CMAKE_C_FLAGS:STRING=' /nologo /DWIN32 /D_WINDOWS /utf-8 /MP '

CMAKE_C_FLAGS_DEBUG:STRING='/MDd /Z7 /Ob0 /Od /RTC1 '

CMAKE_C_FLAGS_RELEASE:STRING='/MD /O2 /Oi /Gy /DNDEBUG /Z7 '

//No help, variable specified on the command line.
CMAKE_DISABLE_FIND_PACKAGE_Unwind:UNINITIALIZED=ON

//No help, variable specified on the command line.
CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION:UNINITIALIZED=ON

//Flags used by the linker during all build types.
CMAKE_EXE_LINKER_FLAGS:STRING=/machine:x64

//Flags used by the linker during DEBUG builds.
CMAKE_EXE_LINKER_FLAGS_DEBUG:STRING=/nologo    /debug /INCREMENTAL

//Flags used by the linker during MINSIZEREL builds.
CMAKE_EXE_LINKER_FLAGS_MINSIZEREL:STRING=/INCREMENTAL:NO

CMAKE_EXE_LINKER_FLAGS_RELEASE:STRING='/nologo /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF  '

//Flags used by the linker during RELWITHDEBINFO builds.
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO:STRING=/debug /INCREMENTAL

//Enable/Disable output of build database during the build.
CMAKE_EXPORT_BUILD_DATABASE:BOOL=

//Enable/Disable output of compile commands during generation.
CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=

//No help, variable specified on the command line.
CMAKE_EXPORT_NO_PACKAGE_REGISTRY:UNINITIALIZED=ON

//No help, variable specified on the command line.
CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY:UNINITIALIZED=ON

//No help, variable specified on the command line.
CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY:UNINITIALIZED=ON

//Value Computed by CMake.
CMAKE_FIND_PACKAGE_REDIRECTS_DIR:STATIC=C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/pkgRedirects

//No help, variable specified on the command line.
CMAKE_INSTALL_BINDIR:STRING=bin

//Read-only architecture-independent data (DATAROOTDIR)
CMAKE_INSTALL_DATADIR:PATH=

//Read-only architecture-independent data root (share)
CMAKE_INSTALL_DATAROOTDIR:PATH=share

//Documentation root (DATAROOTDIR/doc/PROJECT_NAME)
CMAKE_INSTALL_DOCDIR:PATH=
...
Skipped 435 lines
...
CMAKE_INSTALL_SBINDIR-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_INSTALL_SHAREDSTATEDIR
CMAKE_INSTALL_SHAREDSTATEDIR-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_INSTALL_SYSCONFDIR
CMAKE_INSTALL_SYSCONFDIR-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_LINKER
CMAKE_LINKER-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MODULE_LINKER_FLAGS
CMAKE_MODULE_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MODULE_LINKER_FLAGS_DEBUG
CMAKE_MODULE_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL
CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MODULE_LINKER_FLAGS_RELEASE
CMAKE_MODULE_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_MT
CMAKE_MT-ADVANCED:INTERNAL=1
//number of local generators
CMAKE_NUMBER_OF_MAKEFILES:INTERNAL=1
//Platform information initialized
CMAKE_PLATFORM_INFO_INITIALIZED:INTERNAL=1
//noop for ranlib
CMAKE_RANLIB:INTERNAL=:
//ADVANCED property for variable: CMAKE_RC_COMPILER
CMAKE_RC_COMPILER-ADVANCED:INTERNAL=1
CMAKE_RC_COMPILER_WORKS:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS
CMAKE_RC_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_DEBUG
CMAKE_RC_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_MINSIZEREL
CMAKE_RC_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_RELEASE
CMAKE_RC_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_RELWITHDEBINFO
CMAKE_RC_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//Path to CMake installation.
CMAKE_ROOT:INTERNAL=C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS
CMAKE_SHARED_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_DEBUG
CMAKE_SHARED_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_RELEASE
CMAKE_SHARED_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SKIP_INSTALL_RPATH
CMAKE_SKIP_INSTALL_RPATH-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SKIP_RPATH
CMAKE_SKIP_RPATH-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS
CMAKE_STATIC_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_DEBUG
CMAKE_STATIC_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL
CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_RELEASE
CMAKE_STATIC_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_TOOLCHAIN_FILE
CMAKE_TOOLCHAIN_FILE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_VERBOSE_MAKEFILE
CMAKE_VERBOSE_MAKEFILE-ADVANCED:INTERNAL=1
//Test COMPILER_HAS_DEPRECATED
COMPILER_HAS_DEPRECATED:INTERNAL=1
//Test COMPILER_HAS_DEPRECATED_ATTR
COMPILER_HAS_DEPRECATED_ATTR:INTERNAL=
//ADVANCED property for variable: CPACK_BINARY_7Z
CPACK_BINARY_7Z-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_IFW
CPACK_BINARY_IFW-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_INNOSETUP
CPACK_BINARY_INNOSETUP-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_NSIS
CPACK_BINARY_NSIS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_NUGET
CPACK_BINARY_NUGET-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_WIX
CPACK_BINARY_WIX-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_ZIP
CPACK_BINARY_ZIP-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_SOURCE_7Z
CPACK_SOURCE_7Z-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_SOURCE_ZIP
CPACK_SOURCE_ZIP-ADVANCED:INTERNAL=1
//Details about finding Threads
FIND_PACKAGE_MESSAGE_DETAILS_Threads:INTERNAL=[TRUE][v()]
//Have symbol UnDecorateSymbolName
HAVE_DBGHELP:INTERNAL=1
//Have symbol dladdr
HAVE_DLADDR:INTERNAL=
//Have include dlfcn.h
HAVE_DLFCN_H:INTERNAL=
//Have include elf.h
HAVE_ELF_H:INTERNAL=
//Have symbol backtrace
HAVE_EXECINFO_BACKTRACE:INTERNAL=
//Have symbol backtrace_symbols
HAVE_EXECINFO_BACKTRACE_SYMBOLS:INTERNAL=
//Have symbol fcntl
HAVE_FCNTL:INTERNAL=
//Have symbol getprogname
HAVE_GETPROGNAME:INTERNAL=
//Have include glob.h
HAVE_GLOB_H:INTERNAL=
//Have symbol gmtime_r
HAVE_GMTIME_R:INTERNAL=
//Result of TRY_COMPILE
HAVE_HAVE_MODE_T:INTERNAL=FALSE
//Result of TRY_COMPILE
HAVE_HAVE_SSIZE_T:INTERNAL=FALSE
//Have include link.h
HAVE_LINK_H:INTERNAL=
//Have symbol localtime_r
HAVE_LOCALTIME_R:INTERNAL=
//CHECK_TYPE_SIZE: mode_t unknown
HAVE_MODE_T:INTERNAL=
//Have symbol posix_fadvise
HAVE_POSIX_FADVISE:INTERNAL=
//Have symbol pread
HAVE_PREAD:INTERNAL=
//Have symbol program_invocation_short_name
HAVE_PROGRAM_INVOCATION_SHORT_NAME:INTERNAL=
//Have include pwd.h
HAVE_PWD_H:INTERNAL=
//Have symbol pwrite
HAVE_PWRITE:INTERNAL=
//Have symbol sigaction
HAVE_SIGACTION:INTERNAL=
//Have symbol sigaltstack
HAVE_SIGALTSTACK:INTERNAL=
//CHECK_TYPE_SIZE: ssize_t unknown
HAVE_SSIZE_T:INTERNAL=
//Have include stddef.h
HAVE_STDDEF_H:INTERNAL=1
//Have include stdint.h
HAVE_STDINT_H:INTERNAL=1
//Test HAVE_SYMBOLIZE
HAVE_SYMBOLIZE:INTERNAL=1
//Result of TRY_COMPILE
HAVE_SYMBOLIZE_COMPILED:INTERNAL=TRUE
//Result of try_run()
HAVE_SYMBOLIZE_EXITCODE:INTERNAL=0
//Have include syscall.h
HAVE_SYSCALL_H:INTERNAL=
//Have include syslog.h
HAVE_SYSLOG_H:INTERNAL=
//Have include sys/exec_elf.h
HAVE_SYS_EXEC_ELF_H:INTERNAL=
//Have include sys/syscall.h
HAVE_SYS_SYSCALL_H:INTERNAL=
//Have include sys/time.h
HAVE_SYS_TIME_H:INTERNAL=
//Have include sys/types.h
HAVE_SYS_TYPES_H:INTERNAL=1
//Have include sys/utsname.h
HAVE_SYS_UTSNAME_H:INTERNAL=
//Have include sys/wait.h
HAVE_SYS_WAIT_H:INTERNAL=
//Have include ucontext.h
HAVE_UCONTEXT_H:INTERNAL=
//Have include unistd.h
HAVE_UNISTD_H:INTERNAL=
//Have symbol _chsize_s
HAVE__CHSIZE_S:INTERNAL=1
//Have symbol __argv
HAVE___ARGV:INTERNAL=1
//Have symbol abi::__cxa_demangle
HAVE___CXA_DEMANGLE:INTERNAL=
//Test HAVE___PROGNAME
HAVE___PROGNAME:INTERNAL=
//Install the dependencies listed in your manifest:
//\n    If this is off, you will have to manually install your dependencies.
//\n    See https://github.com/microsoft/vcpkg/tree/master/docs/specifications/manifests.md
// for more info.
//\n
VCPKG_MANIFEST_INSTALL:INTERNAL=OFF
//ADVANCED property for variable: VCPKG_VERBOSE
VCPKG_VERBOSE-ADVANCED:INTERNAL=1
//STRINGS property for variable: WITH_FUZZING
WITH_FUZZING-STRINGS:INTERNAL=none;libfuzzer;ossfuzz
//STRINGS property for variable: WITH_UNWIND
WITH_UNWIND-STRINGS:INTERNAL=none;unwind;libunwind
//Making sure VCPKG_MANIFEST_MODE doesn't change
Z_VCPKG_CHECK_MANIFEST_MODE:INTERNAL=OFF
//Vcpkg root directory
Z_VCPKG_ROOT_DIR:INTERNAL=C:/Users/lizar/.vcpkg
//CMAKE_INSTALL_PREFIX during last run
_GNUInstallDirs_LAST_CMAKE_INSTALL_PREFIX:INTERNAL=C:/Users/lizar/.vcpkg/packages/glog_x64-windows
//gflags namespace
gflags_NAMESPACE:INTERNAL=gflags

```
</details>

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-CMakeCache.txt.log</summary>

```
# This is the CMakeCache file.
# For build in directory: c:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg
# It was generated by CMake: C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/bin/cmake.exe
# You can edit this file to change values found and used by cmake.
# If you do not want to change any of the values, simply exit the editor.
# If you do want to change a value, simply edit, save, and exit the editor.
# The syntax for the file is as follows:
# KEY:TYPE=VALUE
# KEY is the name of a variable in the cache.
# TYPE is a hint to GUIs for the type of VALUE, DO NOT EDIT TYPE!.
# VALUE is the current value for the KEY.

########################
# EXTERNAL cache entries
########################

//Build shared libraries
BUILD_SHARED_LIBS:BOOL=ON

//Build the testing tree.
BUILD_TESTING:BOOL=OFF

//Path to a program.
CMAKE_AR:FILEPATH=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/lib.exe

//Choose the type of build, options are: None Debug Release RelWithDebInfo
// MinSizeRel ...
CMAKE_BUILD_TYPE:STRING=Debug

CMAKE_CROSSCOMPILING:STRING=OFF

//CXX compiler
CMAKE_CXX_COMPILER:FILEPATH=C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe

CMAKE_CXX_FLAGS:STRING=' /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP '

CMAKE_CXX_FLAGS_DEBUG:STRING='/MDd /Z7 /Ob0 /Od /RTC1 '

//Flags used by the CXX compiler during MINSIZEREL builds.
CMAKE_CXX_FLAGS_MINSIZEREL:STRING=/O1 /Ob1 /DNDEBUG

CMAKE_CXX_FLAGS_RELEASE:STRING='/MD /O2 /Oi /Gy /DNDEBUG /Z7 '

//Flags used by the CXX compiler during RELWITHDEBINFO builds.
CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=/Zi /O2 /Ob1 /DNDEBUG

//Libraries linked by default with all C++ applications.
CMAKE_CXX_STANDARD_LIBRARIES:STRING=kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib

CMAKE_C_FLAGS:STRING=' /nologo /DWIN32 /D_WINDOWS /utf-8 /MP '

CMAKE_C_FLAGS_DEBUG:STRING='/MDd /Z7 /Ob0 /Od /RTC1 '

CMAKE_C_FLAGS_RELEASE:STRING='/MD /O2 /Oi /Gy /DNDEBUG /Z7 '

//No help, variable specified on the command line.
CMAKE_DISABLE_FIND_PACKAGE_Unwind:UNINITIALIZED=ON

//No help, variable specified on the command line.
CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION:UNINITIALIZED=ON

//Flags used by the linker during all build types.
CMAKE_EXE_LINKER_FLAGS:STRING=/machine:x64

//Flags used by the linker during DEBUG builds.
CMAKE_EXE_LINKER_FLAGS_DEBUG:STRING=/nologo    /debug /INCREMENTAL

//Flags used by the linker during MINSIZEREL builds.
CMAKE_EXE_LINKER_FLAGS_MINSIZEREL:STRING=/INCREMENTAL:NO

CMAKE_EXE_LINKER_FLAGS_RELEASE:STRING='/nologo /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF  '

//Flags used by the linker during RELWITHDEBINFO builds.
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO:STRING=/debug /INCREMENTAL

//Enable/Disable output of build database during the build.
CMAKE_EXPORT_BUILD_DATABASE:BOOL=

//Enable/Disable output of compile commands during generation.
CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=

//No help, variable specified on the command line.
CMAKE_EXPORT_NO_PACKAGE_REGISTRY:UNINITIALIZED=ON

//No help, variable specified on the command line.
CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY:UNINITIALIZED=ON

//No help, variable specified on the command line.
...
Skipped 470 lines
...
//number of local generators
CMAKE_NUMBER_OF_MAKEFILES:INTERNAL=1
//Platform information initialized
CMAKE_PLATFORM_INFO_INITIALIZED:INTERNAL=1
//noop for ranlib
CMAKE_RANLIB:INTERNAL=:
//ADVANCED property for variable: CMAKE_RC_COMPILER
CMAKE_RC_COMPILER-ADVANCED:INTERNAL=1
CMAKE_RC_COMPILER_WORKS:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS
CMAKE_RC_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_DEBUG
CMAKE_RC_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_MINSIZEREL
CMAKE_RC_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_RELEASE
CMAKE_RC_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_RC_FLAGS_RELWITHDEBINFO
CMAKE_RC_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//Path to CMake installation.
CMAKE_ROOT:INTERNAL=C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS
CMAKE_SHARED_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_DEBUG
CMAKE_SHARED_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_RELEASE
CMAKE_SHARED_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SKIP_INSTALL_RPATH
CMAKE_SKIP_INSTALL_RPATH-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_SKIP_RPATH
CMAKE_SKIP_RPATH-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS
CMAKE_STATIC_LINKER_FLAGS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_DEBUG
CMAKE_STATIC_LINKER_FLAGS_DEBUG-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL
CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_RELEASE
CMAKE_STATIC_LINKER_FLAGS_RELEASE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO
CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_TOOLCHAIN_FILE
CMAKE_TOOLCHAIN_FILE-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CMAKE_VERBOSE_MAKEFILE
CMAKE_VERBOSE_MAKEFILE-ADVANCED:INTERNAL=1
//Test COMPILER_HAS_DEPRECATED
COMPILER_HAS_DEPRECATED:INTERNAL=1
//Test COMPILER_HAS_DEPRECATED_ATTR
COMPILER_HAS_DEPRECATED_ATTR:INTERNAL=
//ADVANCED property for variable: CPACK_BINARY_7Z
CPACK_BINARY_7Z-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_IFW
CPACK_BINARY_IFW-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_INNOSETUP
CPACK_BINARY_INNOSETUP-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_NSIS
CPACK_BINARY_NSIS-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_NUGET
CPACK_BINARY_NUGET-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_WIX
CPACK_BINARY_WIX-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_BINARY_ZIP
CPACK_BINARY_ZIP-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_SOURCE_7Z
CPACK_SOURCE_7Z-ADVANCED:INTERNAL=1
//ADVANCED property for variable: CPACK_SOURCE_ZIP
CPACK_SOURCE_ZIP-ADVANCED:INTERNAL=1
//Details about finding Threads
FIND_PACKAGE_MESSAGE_DETAILS_Threads:INTERNAL=[TRUE][v()]
//Have symbol UnDecorateSymbolName
HAVE_DBGHELP:INTERNAL=1
//Have symbol dladdr
HAVE_DLADDR:INTERNAL=
//Have include dlfcn.h
HAVE_DLFCN_H:INTERNAL=
//Have include elf.h
HAVE_ELF_H:INTERNAL=
//Have symbol backtrace
HAVE_EXECINFO_BACKTRACE:INTERNAL=
//Have symbol backtrace_symbols
HAVE_EXECINFO_BACKTRACE_SYMBOLS:INTERNAL=
//Have symbol fcntl
HAVE_FCNTL:INTERNAL=
//Have symbol getprogname
HAVE_GETPROGNAME:INTERNAL=
//Have include glob.h
HAVE_GLOB_H:INTERNAL=
//Have symbol gmtime_r
HAVE_GMTIME_R:INTERNAL=
//Result of TRY_COMPILE
HAVE_HAVE_MODE_T:INTERNAL=FALSE
//Result of TRY_COMPILE
HAVE_HAVE_SSIZE_T:INTERNAL=FALSE
//Have include link.h
HAVE_LINK_H:INTERNAL=
//Have symbol localtime_r
HAVE_LOCALTIME_R:INTERNAL=
//CHECK_TYPE_SIZE: mode_t unknown
HAVE_MODE_T:INTERNAL=
//Have symbol posix_fadvise
HAVE_POSIX_FADVISE:INTERNAL=
//Have symbol pread
HAVE_PREAD:INTERNAL=
//Have symbol program_invocation_short_name
HAVE_PROGRAM_INVOCATION_SHORT_NAME:INTERNAL=
//Have include pwd.h
HAVE_PWD_H:INTERNAL=
//Have symbol pwrite
HAVE_PWRITE:INTERNAL=
//Have symbol sigaction
HAVE_SIGACTION:INTERNAL=
//Have symbol sigaltstack
HAVE_SIGALTSTACK:INTERNAL=
//CHECK_TYPE_SIZE: ssize_t unknown
HAVE_SSIZE_T:INTERNAL=
//Have include stddef.h
HAVE_STDDEF_H:INTERNAL=1
//Have include stdint.h
HAVE_STDINT_H:INTERNAL=1
//Test HAVE_SYMBOLIZE
HAVE_SYMBOLIZE:INTERNAL=1
//Result of TRY_COMPILE
HAVE_SYMBOLIZE_COMPILED:INTERNAL=TRUE
//Result of try_run()
HAVE_SYMBOLIZE_EXITCODE:INTERNAL=0
//Have include syscall.h
HAVE_SYSCALL_H:INTERNAL=
//Have include syslog.h
HAVE_SYSLOG_H:INTERNAL=
//Have include sys/exec_elf.h
HAVE_SYS_EXEC_ELF_H:INTERNAL=
//Have include sys/syscall.h
HAVE_SYS_SYSCALL_H:INTERNAL=
//Have include sys/time.h
HAVE_SYS_TIME_H:INTERNAL=
//Have include sys/types.h
HAVE_SYS_TYPES_H:INTERNAL=1
//Have include sys/utsname.h
HAVE_SYS_UTSNAME_H:INTERNAL=
//Have include sys/wait.h
HAVE_SYS_WAIT_H:INTERNAL=
//Have include ucontext.h
HAVE_UCONTEXT_H:INTERNAL=
//Have include unistd.h
HAVE_UNISTD_H:INTERNAL=
//Have symbol _chsize_s
HAVE__CHSIZE_S:INTERNAL=1
//Have symbol __argv
HAVE___ARGV:INTERNAL=1
//Have symbol abi::__cxa_demangle
HAVE___CXA_DEMANGLE:INTERNAL=
//Test HAVE___PROGNAME
HAVE___PROGNAME:INTERNAL=
//Install the dependencies listed in your manifest:
//\n    If this is off, you will have to manually install your dependencies.
//\n    See https://github.com/microsoft/vcpkg/tree/master/docs/specifications/manifests.md
// for more info.
//\n
VCPKG_MANIFEST_INSTALL:INTERNAL=OFF
//ADVANCED property for variable: VCPKG_VERBOSE
VCPKG_VERBOSE-ADVANCED:INTERNAL=1
//STRINGS property for variable: WITH_FUZZING
WITH_FUZZING-STRINGS:INTERNAL=none;libfuzzer;ossfuzz
//STRINGS property for variable: WITH_UNWIND
WITH_UNWIND-STRINGS:INTERNAL=none;unwind;libunwind
//Making sure VCPKG_MANIFEST_MODE doesn't change
Z_VCPKG_CHECK_MANIFEST_MODE:INTERNAL=OFF
//Vcpkg root directory
Z_VCPKG_ROOT_DIR:INTERNAL=C:/Users/lizar/.vcpkg
//CMAKE_INSTALL_PREFIX during last run
_GNUInstallDirs_LAST_CMAKE_INSTALL_PREFIX:INTERNAL=C:/Users/lizar/.vcpkg/packages/glog_x64-windows/debug
//gflags namespace
gflags_NAMESPACE:INTERNAL=gflags

```
</details>

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-ninja.log</summary>

```
# CMAKE generated file: DO NOT EDIT!
# Generated by "Ninja" Generator, CMake Version 3.31

# This file contains all the build statements describing the
# compilation DAG.

# =============================================================================
# Write statements declared in CMakeLists.txt:
# 
# Which is the root file.
# =============================================================================

# =============================================================================
# Project: glog
# Configurations: Debug
# =============================================================================

#############################################
# Minimal version of Ninja required by this file

ninja_required_version = 1.5


#############################################
# Set configuration variable for custom commands.

CONFIGURATION = Debug
# =============================================================================
# Include auxiliary files.


#############################################
# Include rules file.

include CMakeFiles\rules.ninja

# =============================================================================

#############################################
# Logical path to working directory; prefix for absolute paths.

cmake_ninja_workdir = C$:\Users\lizar\.vcpkg\buildtrees\glog\x64-windows-dbg\
# =============================================================================
# Object build statements for OBJECT_LIBRARY target glog_internal


#############################################
# Order-only phony target for glog_internal

build cmake_object_order_depends_target_glog_internal: phony || .

build CMakeFiles\glog_internal.dir\src\demangle.cc.obj: CXX_COMPILER__glog_internal_unscanned_Debug C$:\Users\lizar\.vcpkg\buildtrees\glog\src\v0.7.1-795557b621.clean\src\demangle.cc || cmake_object_order_depends_target_glog_internal
  DEFINES = -DGFLAGS_IS_A_DLL=1 -DGLOG_NO_ABBREVIATED_SEVERITIES -DGLOG_NO_SYMBOLIZE_DETECTION -DGLOG_USE_GFLAGS -DGLOG_USE_GLOG_EXPORT -DGLOG_USE_WINDOWS_PORT -DGOOGLE_GLOG_IS_A_DLL -DNOMINMAX -DWIN32_LEAN_AND_MEAN
  FLAGS = /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP  /MDd /Z7 /Ob0 /Od /RTC1  -MDd
  INCLUDES = -IC:\Users\lizar\.vcpkg\buildtrees\glog\src\v0.7.1-795557b621.clean\src\windows -IC:\Users\lizar\.vcpkg\buildtrees\glog\src\v0.7.1-795557b621.clean\src -IC:\Users\lizar\.vcpkg\buildtrees\glog\x64-windows-dbg -IC:\Users\lizar\Documents\ValveWorkbench\vcpkg_installed\x64-windows\include
  OBJECT_DIR = CMakeFiles\glog_internal.dir
  OBJECT_FILE_DIR = CMakeFiles\glog_internal.dir\src
  TARGET_COMPILE_PDB = CMakeFiles\glog_internal.dir\
  TARGET_PDB = ""

build CMakeFiles\glog_internal.dir\src\flags.cc.obj: CXX_COMPILER__glog_internal_unscanned_Debug C$:\Users\lizar\.vcpkg\buildtrees\glog\src\v0.7.1-795557b621.clean\src\flags.cc || cmake_object_order_depends_target_glog_internal
  DEFINES = -DGFLAGS_IS_A_DLL=1 -DGLOG_NO_ABBREVIATED_SEVERITIES -DGLOG_NO_SYMBOLIZE_DETECTION -DGLOG_USE_GFLAGS -DGLOG_USE_GLOG_EXPORT -DGLOG_USE_WINDOWS_PORT -DGOOGLE_GLOG_IS_A_DLL -DNOMINMAX -DWIN32_LEAN_AND_MEAN
  FLAGS = /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP  /MDd /Z7 /Ob0 /Od /RTC1  -MDd
...
Skipped 241 lines
...


#############################################
# Clean all the built files.

build clean: CLEAN


#############################################
# Print all primary targets available.

build help: HELP


#############################################
# Make the all target the default.

default all
```
</details>

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-dbg-CMakeConfigureLog.yaml.log</summary>

```

---
events:
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineSystem.cmake:205 (message)"
      - "CMakeLists.txt:2 (project)"
    message: |
      The system is: Windows - 10.0.26100 - AMD64
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:17 (message)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:64 (__determine_compiler_id_test)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCXXCompiler.cmake:126 (CMAKE_DETERMINE_COMPILER_ID)"
      - "CMakeLists.txt:2 (project)"
    message: |
      Compiling the CXX compiler identification source file "CMakeCXXCompilerId.cpp" succeeded.
      Compiler: C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe 
      Build flags: /nologo;/DWIN32;/D_WINDOWS;/utf-8;/GR;/EHsc;/MP
      Id flags:  
      
      The output was:
      0
      CMakeCXXCompilerId.cpp
      
      
      Compilation of the CXX compiler identification source "CMakeCXXCompilerId.cpp" produced "CMakeCXXCompilerId.exe"
      
      Compilation of the CXX compiler identification source "CMakeCXXCompilerId.cpp" produced "CMakeCXXCompilerId.obj"
      
      The CXX compiler identification is MSVC, found in:
        C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/3.31.6/CompilerIdCXX/CMakeCXXCompilerId.exe
      
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:1288 (message)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:250 (CMAKE_DETERMINE_MSVC_SHOWINCLUDES_PREFIX)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCXXCompiler.cmake:126 (CMAKE_DETERMINE_COMPILER_ID)"
      - "CMakeLists.txt:2 (project)"
    message: |
      Detecting CXX compiler /showIncludes prefix:
        main.c
        Note: including file: C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\ShowIncludes\\foo.h
        
      Found prefix "Note: including file: "
  -
    kind: "try_compile-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerABI.cmake:74 (try_compile)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeTestCXXCompiler.cmake:26 (CMAKE_DETERMINE_COMPILER_ABI)"
      - "CMakeLists.txt:2 (project)"
    checks:
      - "Detecting CXX compiler ABI info"
    directories:
      source: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-r198k5"
      binary: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-r198k5"
    cmakeVariables:
      CMAKE_CXX_FLAGS: " /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP "
      CMAKE_CXX_FLAGS_DEBUG: "/MDd /Z7 /Ob0 /Od /RTC1 "
      CMAKE_CXX_SCAN_FOR_MODULES: "OFF"
      CMAKE_EXE_LINKER_FLAGS: "/machine:x64"
      CMAKE_MSVC_DEBUG_INFORMATION_FORMAT: ""
      CMAKE_MSVC_RUNTIME_LIBRARY: "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<STREQUAL:dynamic,dynamic>:DLL>"
      VCPKG_CHAINLOAD_TOOLCHAIN_FILE: "C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake"
      VCPKG_CRT_LINKAGE: "dynamic"
      VCPKG_CXX_FLAGS: ""
      VCPKG_CXX_FLAGS_DEBUG: ""
      VCPKG_CXX_FLAGS_RELEASE: ""
      VCPKG_C_FLAGS: ""
      VCPKG_C_FLAGS_DEBUG: ""
      VCPKG_C_FLAGS_RELEASE: ""
      VCPKG_INSTALLED_DIR: "C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed"
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "CMAKE_CXX_ABI_COMPILED"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-r198k5'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_8da3c
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP   /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_8da3c.dir\\CMakeCXXCompilerABI.cpp.obj /FdCMakeFiles\\cmTC_8da3c.dir\\ /FS -c C:\\Users\\lizar\\AppData\\Local\\Programs\\Python\\Python313\\Lib\\site-packages\\cmake\\data\\share\\cmake-3.31\\Modules\\CMakeCXXCompilerABI.cpp
...
Skipped 2131 lines
...
      VCPKG_CXX_FLAGS_RELEASE: ""
      VCPKG_C_FLAGS: ""
      VCPKG_C_FLAGS_DEBUG: ""
      VCPKG_C_FLAGS_RELEASE: ""
      VCPKG_INSTALLED_DIR: "C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed"
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "HAVE_LOCALTIME_R"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-jb10na'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_e73b0
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP   /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_e73b0.dir\\CheckSymbolExists.cxx.obj /FdCMakeFiles\\cmTC_e73b0.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-jb10na\\CheckSymbolExists.cxx
        FAILED: CMakeFiles/cmTC_e73b0.dir/CheckSymbolExists.cxx.obj 
        C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP   /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_e73b0.dir\\CheckSymbolExists.cxx.obj /FdCMakeFiles\\cmTC_e73b0.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-jb10na\\CheckSymbolExists.cxx
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-jb10na\\CheckSymbolExists.cxx(9): error C2065: 'localtime_r': undeclared identifier
        ninja: build stopped: subcommand failed.
        
      exitCode: 1
  -
    kind: "try_compile-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/Internal/CheckSourceCompiles.cmake:108 (try_compile)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CheckSourceCompiles.cmake:89 (cmake_check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:206 (check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:263 (_check_cxx_compiler_attribute)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:420 (_test_compiler_has_deprecated)"
      - "CMakeLists.txt:510 (generate_export_header)"
    checks:
      - "Performing Test COMPILER_HAS_DEPRECATED_ATTR"
    directories:
      source: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-qkmgae"
      binary: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-qkmgae"
    cmakeVariables:
      CMAKE_CXX_FLAGS: " /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP "
      CMAKE_CXX_FLAGS_DEBUG: "/MDd /Z7 /Ob0 /Od /RTC1 "
      CMAKE_EXE_LINKER_FLAGS: "/machine:x64"
      CMAKE_MODULE_PATH: "C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean/cmake"
      CMAKE_MSVC_DEBUG_INFORMATION_FORMAT: ""
      CMAKE_MSVC_RUNTIME_LIBRARY: "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<STREQUAL:dynamic,dynamic>:DLL>"
      CMAKE_POSITION_INDEPENDENT_CODE: "ON"
      VCPKG_CHAINLOAD_TOOLCHAIN_FILE: "C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake"
      VCPKG_CRT_LINKAGE: "dynamic"
      VCPKG_CXX_FLAGS: ""
      VCPKG_CXX_FLAGS_DEBUG: ""
      VCPKG_CXX_FLAGS_RELEASE: ""
      VCPKG_C_FLAGS: ""
      VCPKG_C_FLAGS_DEBUG: ""
      VCPKG_C_FLAGS_RELEASE: ""
      VCPKG_INSTALLED_DIR: "C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed"
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "COMPILER_HAS_DEPRECATED_ATTR"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-qkmgae'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_52dd6
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED_ATTR  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_52dd6.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_52dd6.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx
        FAILED: CMakeFiles/cmTC_52dd6.dir/src.cxx.obj 
        C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED_ATTR  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_52dd6.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_52dd6.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(1): error C2065: '__deprecated__': undeclared identifier
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(1): error C4430: missing type specifier - int assumed. Note: C++ does not support default-int
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(1): error C2062: type 'int' unexpected
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(1): error C2143: syntax error: missing ';' before '{'
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(1): error C2447: '{': missing function header (old-style formal list?)
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-qkmgae\\src.cxx(2): error C3861: 'somefunc': identifier not found
        ninja: build stopped: subcommand failed.
        
      exitCode: 1
  -
    kind: "try_compile-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/Internal/CheckSourceCompiles.cmake:108 (try_compile)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CheckSourceCompiles.cmake:89 (cmake_check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:206 (check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:269 (_check_cxx_compiler_attribute)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:420 (_test_compiler_has_deprecated)"
      - "CMakeLists.txt:510 (generate_export_header)"
    checks:
      - "Performing Test COMPILER_HAS_DEPRECATED"
    directories:
      source: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-3frwwj"
      binary: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-3frwwj"
    cmakeVariables:
      CMAKE_CXX_FLAGS: " /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP "
      CMAKE_CXX_FLAGS_DEBUG: "/MDd /Z7 /Ob0 /Od /RTC1 "
      CMAKE_EXE_LINKER_FLAGS: "/machine:x64"
      CMAKE_MODULE_PATH: "C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean/cmake"
      CMAKE_MSVC_DEBUG_INFORMATION_FORMAT: ""
      CMAKE_MSVC_RUNTIME_LIBRARY: "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<STREQUAL:dynamic,dynamic>:DLL>"
      CMAKE_POSITION_INDEPENDENT_CODE: "ON"
      VCPKG_CHAINLOAD_TOOLCHAIN_FILE: "C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake"
      VCPKG_CRT_LINKAGE: "dynamic"
      VCPKG_CXX_FLAGS: ""
      VCPKG_CXX_FLAGS_DEBUG: ""
      VCPKG_CXX_FLAGS_RELEASE: ""
      VCPKG_C_FLAGS: ""
      VCPKG_C_FLAGS_DEBUG: ""
      VCPKG_C_FLAGS_RELEASE: ""
      VCPKG_INSTALLED_DIR: "C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed"
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "COMPILER_HAS_DEPRECATED"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-dbg/CMakeFiles/CMakeScratch/TryCompile-3frwwj'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_9daa2
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_9daa2.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_9daa2.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-dbg\\CMakeFiles\\CMakeScratch\\TryCompile-3frwwj\\src.cxx
        [2/2] C:\\WINDOWS\\system32\\cmd.exe /C "cd . && C:\\Users\\lizar\\AppData\\Local\\Programs\\Python\\Python313\\Lib\\site-packages\\cmake\\data\\bin\\cmake.exe -E vs_link_exe --msvc-ver=1944 --intdir=CMakeFiles\\cmTC_9daa2.dir --rc=C:\\PROGRA~2\\WI3CF2~1\\10\\bin\\100261~1.0\\x64\\rc.exe --mt=C:\\PROGRA~2\\WI3CF2~1\\10\\bin\\100261~1.0\\x64\\mt.exe --manifests  -- C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\link.exe /nologo CMakeFiles\\cmTC_9daa2.dir\\src.cxx.obj  /out:cmTC_9daa2.exe /implib:cmTC_9daa2.lib /pdb:cmTC_9daa2.pdb /version:0.0 /machine:x64  /nologo    /debug /INCREMENTAL /subsystem:console  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
        
      exitCode: 0
...
```
</details>

<details><summary>C:\Users\lizar\.vcpkg\buildtrees\glog\config-x64-windows-rel-CMakeConfigureLog.yaml.log</summary>

```

---
events:
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineSystem.cmake:205 (message)"
      - "CMakeLists.txt:2 (project)"
    message: |
      The system is: Windows - 10.0.26100 - AMD64
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:17 (message)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:64 (__determine_compiler_id_test)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCXXCompiler.cmake:126 (CMAKE_DETERMINE_COMPILER_ID)"
      - "CMakeLists.txt:2 (project)"
    message: |
      Compiling the CXX compiler identification source file "CMakeCXXCompilerId.cpp" succeeded.
      Compiler: C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe 
      Build flags: /nologo;/DWIN32;/D_WINDOWS;/utf-8;/GR;/EHsc;/MP
      Id flags:  
      
      The output was:
      0
      CMakeCXXCompilerId.cpp
      
      
      Compilation of the CXX compiler identification source "CMakeCXXCompilerId.cpp" produced "CMakeCXXCompilerId.exe"
      
      Compilation of the CXX compiler identification source "CMakeCXXCompilerId.cpp" produced "CMakeCXXCompilerId.obj"
      
      The CXX compiler identification is MSVC, found in:
        C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/3.31.6/CompilerIdCXX/CMakeCXXCompilerId.exe
      
  -
    kind: "message-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:1288 (message)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerId.cmake:250 (CMAKE_DETERMINE_MSVC_SHOWINCLUDES_PREFIX)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCXXCompiler.cmake:126 (CMAKE_DETERMINE_COMPILER_ID)"
      - "CMakeLists.txt:2 (project)"
    message: |
      Detecting CXX compiler /showIncludes prefix:
        main.c
        Note: including file: C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\ShowIncludes\\foo.h
        
      Found prefix "Note: including file: "
  -
    kind: "try_compile-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeDetermineCompilerABI.cmake:74 (try_compile)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CMakeTestCXXCompiler.cmake:26 (CMAKE_DETERMINE_COMPILER_ABI)"
      - "CMakeLists.txt:2 (project)"
    checks:
      - "Detecting CXX compiler ABI info"
    directories:
      source: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-g6ltdc"
      binary: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-g6ltdc"
    cmakeVariables:
      CMAKE_CXX_FLAGS: " /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP "
...
Skipped 2221 lines
...
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "COMPILER_HAS_DEPRECATED_ATTR"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-t6lfly'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_c3e3d
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED_ATTR  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_c3e3d.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_c3e3d.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx
        FAILED: CMakeFiles/cmTC_c3e3d.dir/src.cxx.obj 
        C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED_ATTR  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_c3e3d.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_c3e3d.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(1): error C2065: '__deprecated__': undeclared identifier
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(1): error C4430: missing type specifier - int assumed. Note: C++ does not support default-int
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(1): error C2062: type 'int' unexpected
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(1): error C2143: syntax error: missing ';' before '{'
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(1): error C2447: '{': missing function header (old-style formal list?)
        C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-t6lfly\\src.cxx(2): error C3861: 'somefunc': identifier not found
        ninja: build stopped: subcommand failed.
        
      exitCode: 1
  -
    kind: "try_compile-v1"
    backtrace:
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/Internal/CheckSourceCompiles.cmake:108 (try_compile)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/CheckSourceCompiles.cmake:89 (cmake_check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:206 (check_source_compiles)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:269 (_check_cxx_compiler_attribute)"
      - "C:/Users/lizar/AppData/Local/Programs/Python/Python313/Lib/site-packages/cmake/data/share/cmake-3.31/Modules/GenerateExportHeader.cmake:420 (_test_compiler_has_deprecated)"
      - "CMakeLists.txt:510 (generate_export_header)"
    checks:
      - "Performing Test COMPILER_HAS_DEPRECATED"
    directories:
      source: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-bgih32"
      binary: "C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-bgih32"
    cmakeVariables:
      CMAKE_CXX_FLAGS: " /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP "
      CMAKE_CXX_FLAGS_DEBUG: "/MDd /Z7 /Ob0 /Od /RTC1 "
      CMAKE_EXE_LINKER_FLAGS: "/machine:x64"
      CMAKE_MODULE_PATH: "C:/Users/lizar/.vcpkg/buildtrees/glog/src/v0.7.1-795557b621.clean/cmake"
      CMAKE_MSVC_DEBUG_INFORMATION_FORMAT: ""
      CMAKE_MSVC_RUNTIME_LIBRARY: "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<STREQUAL:dynamic,dynamic>:DLL>"
      CMAKE_POSITION_INDEPENDENT_CODE: "ON"
      VCPKG_CHAINLOAD_TOOLCHAIN_FILE: "C:/Users/lizar/.vcpkg/scripts/toolchains/windows.cmake"
      VCPKG_CRT_LINKAGE: "dynamic"
      VCPKG_CXX_FLAGS: ""
      VCPKG_CXX_FLAGS_DEBUG: ""
      VCPKG_CXX_FLAGS_RELEASE: ""
      VCPKG_C_FLAGS: ""
      VCPKG_C_FLAGS_DEBUG: ""
      VCPKG_C_FLAGS_RELEASE: ""
      VCPKG_INSTALLED_DIR: "C:/Users/lizar/Documents/ValveWorkbench/vcpkg_installed"
      VCPKG_LINKER_FLAGS: ""
      VCPKG_LINKER_FLAGS_DEBUG: ""
      VCPKG_LINKER_FLAGS_RELEASE: ""
      VCPKG_PLATFORM_TOOLSET: "v143"
      VCPKG_PREFER_SYSTEM_LIBS: "OFF"
      VCPKG_SET_CHARSET_FLAG: "ON"
      VCPKG_TARGET_ARCHITECTURE: "x64"
      VCPKG_TARGET_TRIPLET: "x64-windows"
      Z_VCPKG_ROOT_DIR: "C:/Users/lizar/.vcpkg"
    buildResult:
      variable: "COMPILER_HAS_DEPRECATED"
      cached: true
      stdout: |
        Change Dir: 'C:/Users/lizar/.vcpkg/buildtrees/glog/x64-windows-rel/CMakeFiles/CMakeScratch/TryCompile-bgih32'
        
        Run Build Command(s): "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -v cmTC_7e8c8
        [1/2] C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\cl.exe  /nologo /TP -DCOMPILER_HAS_DEPRECATED  /nologo /DWIN32 /D_WINDOWS /utf-8 /GR /EHsc /MP   /MDd /Z7 /Ob0 /Od /RTC1  -MDd /showIncludes /FoCMakeFiles\\cmTC_7e8c8.dir\\src.cxx.obj /FdCMakeFiles\\cmTC_7e8c8.dir\\ /FS -c C:\\Users\\lizar\\.vcpkg\\buildtrees\\glog\\x64-windows-rel\\CMakeFiles\\CMakeScratch\\TryCompile-bgih32\\src.cxx
        [2/2] C:\\WINDOWS\\system32\\cmd.exe /C "cd . && C:\\Users\\lizar\\AppData\\Local\\Programs\\Python\\Python313\\Lib\\site-packages\\cmake\\data\\bin\\cmake.exe -E vs_link_exe --msvc-ver=1944 --intdir=CMakeFiles\\cmTC_7e8c8.dir --rc=C:\\PROGRA~2\\WI3CF2~1\\10\\bin\\100261~1.0\\x64\\rc.exe --mt=C:\\PROGRA~2\\WI3CF2~1\\10\\bin\\100261~1.0\\x64\\mt.exe --manifests  -- C:\\PROGRA~1\\MICROS~2\\2022\\ENTERP~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\link.exe /nologo CMakeFiles\\cmTC_7e8c8.dir\\src.cxx.obj  /out:cmTC_7e8c8.exe /implib:cmTC_7e8c8.lib /pdb:cmTC_7e8c8.pdb /version:0.0 /machine:x64  /nologo    /debug /INCREMENTAL /subsystem:console  kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib && cd ."
        
      exitCode: 0
...
```
</details>

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "valveworkbench",
  "version-string": "1.0.0",
  "description": "Valve Workbench application",
  "builtin-baseline": "b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01",
  "dependencies": [
    "ceres",
    "glog",
    "qt5-base",
    "qt5-serialport"
  ]
}

```
</details>
