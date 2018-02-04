#ifndef MODIFIEDASSET_H
#define MODIFIEDASSET_H
#include "Basics.h"
#include "FileIO.h"
#include <memory>

enum class AssetAction {
    Add, Remove
};

// Describes a file modified in an MPQ
class ModifiedAsset
{
public:
    std::string assetMpqPath;
    std::string assetTempMpqPath; // Unique path auto-generated by constructor
    WavQuality wavQualitySelected;
    AssetAction actionTaken;

    ModifiedAsset(const std::string &assetMpqPath, AssetAction actionTaken);
    ModifiedAsset(const std::string &assetMpqPath, WavQuality wavQualitySelected, AssetAction actionTaken);
    virtual ~ModifiedAsset();

private:
    static u64 nextAssetId; // Changes are needed if this is accessed in a multi-threaded environment

    ModifiedAsset(); // Disallow ctor
};

typedef std::shared_ptr<ModifiedAsset> ModifiedAssetPtr;

#endif