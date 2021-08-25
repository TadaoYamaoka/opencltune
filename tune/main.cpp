#include "opencltuner.h"

#include <iostream>

using namespace std;

int main()
{
	int gpuIdxForTuning = 0;
    enabled_t testFP16Mode = enabled_t::Auto;
    enabled_t testFP16StorageMode = enabled_t::Auto;
    enabled_t testFP16ComputeMode = enabled_t::Auto;
    enabled_t testFP16TensorCoresMode = enabled_t::Auto;
    OpenCLTuner::ModelInfoForTuning modelInfo = { FEATURES1_NUM, 224, 224 };
    bool full = false;
    string openCLTunerFile = "tune.txt";

    vector<DeviceInfo> allDeviceInfos = DeviceInfo::getAllDeviceInfosOnSystem();

	bool enableProfiling = true;
	DevicesContext devicesContext(allDeviceInfos, { gpuIdxForTuning }, enableProfiling);

    OpenCLTuneParams initialParams;
    int batchSize = OpenCLTuner::DEFAULT_BATCH_SIZE;
    bool verboseErrors = false;
    bool verboseTuner = false;
    OpenCLTuneParams results;
    OpenCLTuner::tune(
        initialParams,
        devicesContext,
        gpuIdxForTuning,
        batchSize,
        testFP16Mode,
        testFP16StorageMode,
        testFP16ComputeMode,
        testFP16TensorCoresMode,
        modelInfo,
        full,
        OpenCLTuner::DEFAULT_WINOGRAD_3X3_TILE_SIZE,
        cerr,
        verboseErrors,
        verboseTuner,
        results
    );
    
    OpenCLTuneParams::save(openCLTunerFile, results);

    return 0;
}
