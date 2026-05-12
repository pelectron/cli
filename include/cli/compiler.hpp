#ifndef CLI_COMPILER_HPP
#define CLI_COMPILER_HPP

// clang-format off
#if defined (_MSC_VER)
  #if defined(__clang__)
    #define CLI_CLANG_CL
  #else
    #define CLI_MSVC
  #endif
#elif defined(__clang__)
  #define CLI_CLANG
#elif defined(__GNUC__)
  #if defined (__arm__)
    #define CLI_ARM_GCC
  #else
    #define CLI_GCC 
  #endif
#else 
#error "Unsupported Compiler detected!"
#endif
// clang-format on

#endif
