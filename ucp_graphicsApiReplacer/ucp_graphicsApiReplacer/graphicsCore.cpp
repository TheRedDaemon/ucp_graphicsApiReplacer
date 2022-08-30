
#include "pch.h"

#include "graphicsCore.h"
#include "directX11Core.h"
#include "openGLCore.h"

namespace UCPGraphicsApiReplacer
{
  std::unique_ptr<GraphicsCore> GraphicsCore::GetGraphicsCore(GraphicsAPIReplacerConfig& conf)
  {
    std::unique_ptr<GraphicsCore> ret{ nullptr };
    switch (conf.graphic.graphicsApi)
    {
    case GRAPHICS_API_DIRECT_X:
      ret = std::make_unique<DirectX11Core>();
      break;
    case GRAPHICS_API_OPEN_GL:
      ret = std::make_unique<OpenGLCore>();
      break;
    default:
      Log(LOG_FATAL, "[graphicsApiReplacer]: Received not supported graphics API enum.");
      return ret;
    }
    ret->setConf(&conf);
    return ret;
  }
}