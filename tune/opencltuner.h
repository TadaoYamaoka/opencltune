#pragma once

#include <string>
#include "core/commontypes.h"
#include "openclhelpers.h"

constexpr int FEATURES1_NUM = 62;
constexpr int FEATURES2_NUM = 57;
constexpr int MAX_MOVE_LABEL_NUM = 27;
constexpr int nnXLen = 9;
constexpr int nnYLen = 9;

namespace OpenCLParams {
    struct XGemmDirectParams {
        int WGD = 8;
        int MDIMCD = 1;
        int NDIMCD = 1;
        int MDIMAD = 1;
        int NDIMBD = 1;
        int KWID = 1;
        int VWMD = 1;
        int VWND = 1;
        int PADA = 1;
        int PADB = 1;

        std::string desc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
    };

    struct XGemmParams {
        int MWG = 8;
        int NWG = 8;
        int KWG = 8;
        int MDIMC = 1;
        int NDIMC = 1;
        int MDIMA = 1;
        int NDIMB = 1;
        int KWI = 1;
        int VWM = 1;
        int VWN = 1;
        int STRM = 0;
        int STRN = 0;
        int SA = 0;
        int SB = 0;

        std::string desc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
        bool isSimple() const;
    };

    struct HGemmWmmaParams {
        int MWG = 16;
        int NWG = 16;
        int KWG = 16;
        int MWAVE = 16;
        int NWAVE = 16;
        int MWARP = 16;
        int NWARP = 16;
        int VWM = 2;
        int VWN = 2;
        int SA = 0;
        int SB = 0;

        std::string desc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
        bool isSimple() const;
    };

    struct Conv3x3Params {
        //Winograd input and output tile sizes
        int INTILE_XSIZE = 4;
        int INTILE_YSIZE = 4;
        int OUTTILE_XSIZE = 2;
        int OUTTILE_YSIZE = 2;

        int transLocalSize0 = 1;
        int transLocalSize1 = 1;

        int untransLocalSize0 = 1;
        int untransLocalSize1 = 1;
        int untransLocalSize2 = 1;

        std::string desc() const;
        std::string transDesc() const;
        std::string untransDesc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
    };

    struct Conv5x5Params {
        //Winograd input and output tile sizes
        int INTILE_XSIZE = 6;
        int INTILE_YSIZE = 6;
        int OUTTILE_XSIZE = 2;
        int OUTTILE_YSIZE = 2;

        int transLocalSize0 = 1;
        int transLocalSize1 = 1;

        int untransLocalSize0 = 1;
        int untransLocalSize1 = 1;
        int untransLocalSize2 = 1;

        std::string desc() const;
        std::string transDesc() const;
        std::string untransDesc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
    };

    struct GPoolParams {
        int XYSTRIDE = 1;
        int CHANNELSTRIDE = 1;
        int BATCHSTRIDE = 1;

        std::string desc() const;
        std::string compileOptions() const;
        void fillFromDesc(const std::string& fileName, const std::string& desc);
        bool isValid() const;
    };
}

struct OpenCLTuneParams {
    OpenCLParams::XGemmDirectParams xGemmDirect = OpenCLParams::XGemmDirectParams();
    OpenCLParams::XGemmParams xGemm = OpenCLParams::XGemmParams();

    bool shouldUseFP16Storage = false;
    bool shouldUseFP16Compute = false;
    OpenCLParams::XGemmParams xGemm16 = OpenCLParams::XGemmParams();
    bool shouldUseFP16TensorCores = false;
    OpenCLParams::HGemmWmmaParams hGemmWmma = OpenCLParams::HGemmWmmaParams();

    OpenCLParams::Conv3x3Params conv3x3 = OpenCLParams::Conv3x3Params();

    bool operator==(const OpenCLTuneParams& other) const;
    bool isValid() const;

    int getXGemmMPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const;
    int getXGemmNPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const;
    int getXGemmKPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const;

    static void save(const std::string& filename, const OpenCLTuneParams& config);
    static OpenCLTuneParams load(const std::string& filename);
};

namespace OpenCLTuner {
    constexpr int DEFAULT_BATCH_SIZE = 4;
    constexpr int DEFAULT_WINOGRAD_3X3_TILE_SIZE = 4;

    struct ModelInfoForTuning {
        int maxConvChannels1x1;
        int maxConvChannels3x3;
        int trunkNumChannels;
    };

    void tune(
        const OpenCLTuneParams& initialConfig,
        DevicesContext& devicesContext,
        int gpuIdx,
        int batchSize,
        enabled_t testFP16Mode,
        enabled_t testFP16StorageMode,
        enabled_t testFP16ComputeMode,
        enabled_t testFP16TensorCoresMode,
        ModelInfoForTuning modelInfo,
        bool full,
        int winograd3x3TileSize,
        std::ostream& out,
        bool verboseErrors,
        bool verboseTuner,
        OpenCLTuneParams& tunedConfig
    );
}
