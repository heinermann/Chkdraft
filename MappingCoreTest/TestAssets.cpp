#include <gtest/gtest.h>
#include "TestAssets.h"

bool GetPreSavePath(std::string & outPreSavePath)
{
    return false;
}

bool TestAssets::LoadScData(Sc::Data & data)
{
    char* cScAssetPath = std::getenv("SC_ASSET");
    std::string scAssetPath(cScAssetPath != nullptr ? cScAssetPath : "");
    EXPECT_FALSE(scAssetPath.empty());

    Sc::DataFile::BrowserPtr dataFileBrowser(new Sc::DataFile::Browser());
    
    std::unordered_map<Sc::DataFile::Priority, Sc::DataFile::Descriptor> dataFiles({
        { Sc::DataFile::Priority::StarDat, Sc::DataFile::Descriptor(Sc::DataFile::Priority::StarDat,
            Sc::DataFile::starDatFileName, MakeSystemFilePath(scAssetPath, Sc::DataFile::starDatFileName), nullptr, false) },
        { Sc::DataFile::Priority::BrooDat, Sc::DataFile::Descriptor(Sc::DataFile::Priority::BrooDat,
            Sc::DataFile::brooDatFileName, MakeSystemFilePath(scAssetPath, Sc::DataFile::brooDatFileName), nullptr, false) },
        { Sc::DataFile::Priority::PatchRt, Sc::DataFile::Descriptor(Sc::DataFile::Priority::PatchRt,
            Sc::DataFile::patchRtFileName, MakeSystemFilePath(scAssetPath, Sc::DataFile::patchRtFileName), nullptr, false) },
    });

    bool loadedScData = data.Load(dataFileBrowser, dataFiles, GetDefaultScPath(), nullptr);
    EXPECT_TRUE(loadedScData);
    return loadedScData;
}
