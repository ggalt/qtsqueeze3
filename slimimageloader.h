#ifndef SLIMIMAGELOADER_H
#define SLIMIMAGELOADER_H

#include "squeezedefines.h"
#include "ImageLoader.h"

class SlimImageLoader : public ImageLoader
{
public:
    SlimImageLoader(SlimCLI *slimcli, SlimServerInfo *slimServerInfo);
    ~SlimImageLoader();

private:
    SlimCLI *cli;
    SlimServerInfo *serverInfo;
};


#endif // SLIMIMAGELOADER_H
