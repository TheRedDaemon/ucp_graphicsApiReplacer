#pragma once

#include <type_traits>

#include "unknwn.h"

// source: 
template<typename T, typename = std::enable_if_t<std::is_base_of_v<IUnknown, T>>>
class IUnknownWrapper
{
private:
  T* iUnknownPtr{ nullptr };

  IUnknownWrapper(const IUnknownWrapper&) = delete;
  IUnknownWrapper& operator=(IUnknownWrapper const&) = delete;

public:

  IUnknownWrapper() {};

  IUnknownWrapper(IUnknownWrapper&& otherWrapper)
  {
    T** otherIUnknownPtrPtr{ otherWrapper.expose() };
    iUnknownPtr = *otherIUnknownPtrPtr;
    *otherIUnknownPtrPtr = nullptr;
  };

  virtual ~IUnknownWrapper()
  {
    releaseIfPresent();
  }

  void releaseIfPresent()
  {
    if (iUnknownPtr)
    {
      iUnknownPtr->Release(); // free
      iUnknownPtr = nullptr;
    }
  }

  T* get()
  {
    return iUnknownPtr;
  }
  
  T** expose()
  {
    return &iUnknownPtr;
  };

  T& operator*()
  {
    return *iUnknownPtr;
  };

  T* operator->()
  {
    return iUnknownPtr;
  };

  operator bool() const
  {
    return iUnknownPtr;
  };
};