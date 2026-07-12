#pragma once

#include "ExportOptions.h"

#include <string>

class AssimpExporterApp
{
public:
    int Run(int argc, char** argv);

private:
    bool ParseCommandLine(int argc, char** argv, ExportOptions& kOptions,
        std::string& kError) const;
    void PrintUsage() const;
};
