#include "AssimpExporterApp.h"

#include <iostream>
#include <NiStaticDataManager.h>
#include <NiImageConverter.h>
#include <NiDevImageConverter.h>

int main(int argc, char** argv)
{
    std::cout << "AssimpExporter starting..." << std::endl;
    NiInit(nullptr, true);
    NiImageConverter::SetImageConverter(NiNew NiDevImageConverter);
    AssimpExporterApp kApp;
    int iResult = kApp.Run(argc, argv);
    std::cout << (iResult == 0 ? "Done." : "Finished with errors.") << std::endl;
    NiShutdown(true);
    return iResult;
}
