#include "slimimageloader.h"
#include "slimcli.h"
#include "slimserverinfo.h"

// uncomment the following to turn on debugging
// #define SLIMCLI_DEBUG

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << __VA_ARGS__
#else
#define DEBUGF(...)
#endif



SlimImageLoader::SlimImageLoader(SlimCLI *slimcli, SlimServerInfo *sinfo)
{
    cli = slimcli;
    serverInfo = sinfo;
}

SlimImageLoader::~SlimImageLoader()
{
}

