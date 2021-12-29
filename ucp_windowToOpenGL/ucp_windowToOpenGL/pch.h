// pch.h: Precompiled header file

#ifndef PCH_H
#define PCH_H

// Fügen Sie hier Header hinzu, die vorkompiliert werden sollen.
#include "framework.h"

// for ptr
#include <memory>

// config is needed everywhere
#include "toOpenGLconfig.h"

// for structures
#include <array>

// one can always use a string
#include <string>

// logging
#include "logHelper.h"

// adding simple Size template

// w and x, and h and y are the same values (unions)
// there is std::valarray, but this is more similar to vector, not size save and overkill here
// maybe something if more complicated math (without other libs) is needed
template <typename T>
struct Size
{
  union
  {
    T w;  // width
    T x;
  };
  union
  {
    T h;  // height
    T y;
  };
};

#endif //PCH_H