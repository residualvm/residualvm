#ifndef INCLUDED_FROM_BASE_VERSION_CPP
#error This file may only be included by base/version.cpp
#endif

// Reads revision number from file
// (this is used when building with Visual Studio)
#ifdef RESIDUAL_INTERNAL_REVISION
#include "internal_revision.h"
#endif

#ifdef RELEASE_BUILD
#undef RESIDUAL_REVISION
#endif

#ifndef RESIDUAL_REVISION
#define RESIDUAL_REVISION
#endif

#define RESIDUAL_VERSION "0.0.7git" RESIDUAL_REVISION
