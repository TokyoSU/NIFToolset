#include "AssimpExporterApp.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include <NiStaticDataManager.h>
#include <NiImageConverter.h>
#include <NiDevImageConverter.h>
#include <NiParticleSDM.h>
#include <NiPortalSDM.h>

#include <efd/DefaultInitializeMemoryManager.h>

// efd deliberately leaves these application-level memory factory functions
// undefined in static builds. AssimpExporter is an executable, so opt into
// the stock allocator and memory-log handler supplied by efd.
EE_USE_DEFAULT_ALLOCATOR

namespace
{
    namespace fs = std::filesystem;

    class TeeStreamBuffer final : public std::streambuf
    {
    public:
        TeeStreamBuffer(std::streambuf* pkFirst, std::streambuf* pkSecond)
            : m_pkFirst(pkFirst), m_pkSecond(pkSecond)
        {
        }

    protected:
        int overflow(int iCharacter) override
        {
            if (iCharacter == traits_type::eof())
                return traits_type::not_eof(iCharacter);

            const char c = static_cast<char>(iCharacter);
            const bool bFirstSucceeded =
                !m_pkFirst || m_pkFirst->sputc(c) != traits_type::eof();
            const bool bSecondSucceeded =
                !m_pkSecond || m_pkSecond->sputc(c) != traits_type::eof();

            return bFirstSucceeded && bSecondSucceeded
                ? iCharacter
                : traits_type::eof();
        }

        int sync() override
        {
            const int iFirstResult = m_pkFirst ? m_pkFirst->pubsync() : 0;
            const int iSecondResult = m_pkSecond ? m_pkSecond->pubsync() : 0;
            return iFirstResult == 0 && iSecondResult == 0 ? 0 : -1;
        }

    private:
        std::streambuf* m_pkFirst;
        std::streambuf* m_pkSecond;
    };

    std::string FindCommandLineValue(int argc, char** argv, const char* pcOption)
    {
        for (int i = 1; i + 1 < argc; ++i)
        {
            if (argv[i] && std::string(argv[i]) == pcOption && argv[i + 1])
                return argv[i + 1];
        }

        return {};
    }

    std::string MakeTimestamp()
    {
        const std::time_t kNow = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        std::tm kLocalTime{};
#if defined(_WIN32)
        localtime_s(&kLocalTime, &kNow);
#else
        localtime_r(&kNow, &kLocalTime);
#endif

        std::ostringstream kStream;
        kStream << std::put_time(&kLocalTime, "%Y-%m-%d_%H-%M-%S");
        return kStream.str();
    }

    fs::path BuildLogPath()
    {
        fs::path kLogFolder = fs::current_path() / "logs";
        std::error_code kError;
        fs::create_directories(kLogFolder, kError);

        if (kError)
        {
            // Fall back to the working directory if the requested output
            // directory cannot be created or written to.
            kLogFolder = fs::current_path();
        }

        return kLogFolder /
            ("AssimpExporter_" + MakeTimestamp() + ".log");
    }
}

static void NIF_Initialize()
{
    NiInit(nullptr, true);
    NiImageConverter::SetImageConverter(NiNew NiDevImageConverter);
    NiParticleSDM::Init();
    NiPortalSDM::Init();
}

static void NIF_Shutdown()
{
	NiParticleSDM::Shutdown();
	NiPortalSDM::Shutdown();
    NiShutdown(true);
}

int main(int argc, char** argv)
{
    const fs::path kLogPath = BuildLogPath();
    std::ofstream kLogFile(kLogPath, std::ios::out | std::ios::app);

    std::streambuf* pkOriginalCout = std::cout.rdbuf();
    std::streambuf* pkOriginalCerr = std::cerr.rdbuf();

    TeeStreamBuffer kCoutTee(pkOriginalCout,
        kLogFile.is_open() ? kLogFile.rdbuf() : nullptr);
    TeeStreamBuffer kCerrTee(pkOriginalCerr,
        kLogFile.is_open() ? kLogFile.rdbuf() : nullptr);

    if (kLogFile.is_open())
    {
        std::cout.rdbuf(&kCoutTee);
        std::cerr.rdbuf(&kCerrTee);
    }

    std::cout << "============================================================" << std::endl;
    std::cout << "AssimpExporter starting..." << std::endl;
    std::cout << "Log file: " << kLogPath.string() << std::endl;
    std::cout << "Command line:";
    for (int i = 0; i < argc; ++i)
        std::cout << " \"" << (argv[i] ? argv[i] : "") << "\"";
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;

    int iResult = 1;

    try
    {
        NIF_Initialize();

        AssimpExporterApp kApp;
        iResult = kApp.Run(argc, argv);

        std::cout << (iResult == 0 ? "Done." : "Finished with errors.")
            << std::endl;

        NIF_Shutdown();
    }
    catch (const std::exception& kException)
    {
        std::cerr << "Fatal exception: " << kException.what() << std::endl;
        NIF_Shutdown();
        iResult = 1;
    }
    catch (...)
    {
        std::cerr << "Fatal unknown exception." << std::endl;
        NIF_Shutdown();
        iResult = 1;
    }

    std::cout << "Exit code: " << iResult << std::endl;
    std::cout << "============================================================" << std::endl;

    std::cout.flush();
    std::cerr.flush();

    if (kLogFile.is_open())
    {
        std::cout.rdbuf(pkOriginalCout);
        std::cerr.rdbuf(pkOriginalCerr);
        kLogFile.flush();
        kLogFile.close();
    }

    return iResult;
}
