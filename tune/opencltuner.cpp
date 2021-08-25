#include <iostream>
#include <vector>
#include <random>
#include <map>
#include <sstream>
#include <fstream>

#include "openclhelpers.h"
#include "opencltuner.h"
#include "openclkernels.h"

using namespace std;
using namespace OpenCLHelpers;

using half_t = half_float::half;

static map<string, int> readDescKeyValues(const string& fileName, const string& desc) {
    istringstream kvIn(desc);
    string kvChunk;
    map<string, int> keyValues;
    while (getline(kvIn, kvChunk, ' '))
    {
        if (kvChunk.length() <= 0) continue;
        size_t equalsPos = kvChunk.find_first_of('=');
        if (equalsPos == string::npos) continue;
        string leftChunk = Global::trim(kvChunk.substr(0, equalsPos));
        string rightChunk = Global::trim(kvChunk.substr(equalsPos + 1));
        if (leftChunk.length() == 0)
            throw IOError("OpenCLTuner readDescKeyValues: key value pair without key in: " + desc + " in file " + fileName);
        if (rightChunk.length() == 0)
            throw IOError("OpenCLTuner readDescKeyValues: key value pair without value in: " + desc + " in file " + fileName);
        if (keyValues.find(leftChunk) != keyValues.end())
            throw IOError("OpenCLTuner readDescKeyValues: duplicate key: " + leftChunk);
        int value;
        bool suc = Global::tryStringToInt(rightChunk, value);
        if (!suc)
            throw IOError("OpenCLTuner readDescKeyValues: could not parse value for key " + leftChunk + " in file " + fileName);

        keyValues[leftChunk] = value;
    }
    return keyValues;
}

static bool isMultipleOf(int x, int y) {
    return x % y == 0;
}

static int getInt(const map<string, int> map, const string& key, int defaultValue) {
    if (!contains(map, key))
        return defaultValue;
    return map_get(map, key);
}

string OpenCLParams::XGemmDirectParams::desc() const {
    string s;
    s += "WGD=" + to_string(WGD);
    s += " MDIMCD=" + to_string(MDIMCD);
    s += " NDIMCD=" + to_string(NDIMCD);
    s += " MDIMAD=" + to_string(MDIMAD);
    s += " NDIMBD=" + to_string(NDIMBD);
    s += " KWID=" + to_string(KWID);
    s += " VWMD=" + to_string(VWMD);
    s += " VWND=" + to_string(VWND);
    s += " PADA=" + to_string(PADA);
    s += " PADB=" + to_string(PADB);
    return s;
}
string OpenCLParams::XGemmDirectParams::compileOptions() const {
    string s;
    s += "-DWGD=" + to_string(WGD);
    s += " -DMDIMCD=" + to_string(MDIMCD);
    s += " -DNDIMCD=" + to_string(NDIMCD);
    s += " -DMDIMAD=" + to_string(MDIMAD);
    s += " -DNDIMBD=" + to_string(NDIMBD);
    s += " -DKWID=" + to_string(KWID);
    s += " -DVWMD=" + to_string(VWMD);
    s += " -DVWND=" + to_string(VWND);
    s += " -DPADA=" + to_string(PADA);
    s += " -DPADB=" + to_string(PADB);
    return s;
}
void OpenCLParams::XGemmDirectParams::fillFromDesc(const string& fileName, const string& desc) {
    map<string, int> kvs = readDescKeyValues(fileName, desc);
    WGD = getInt(kvs, "WGD", WGD);
    MDIMCD = getInt(kvs, "MDIMCD", MDIMCD);
    NDIMCD = getInt(kvs, "NDIMCD", NDIMCD);
    MDIMAD = getInt(kvs, "MDIMAD", MDIMAD);
    NDIMBD = getInt(kvs, "NDIMBD", NDIMBD);
    KWID = getInt(kvs, "KWID", KWID);
    VWMD = getInt(kvs, "VWMD", VWMD);
    VWND = getInt(kvs, "VWND", VWND);
    PADA = getInt(kvs, "PADA", PADA);
    PADB = getInt(kvs, "PADB", PADB);
}
bool OpenCLParams::XGemmDirectParams::isValid() const {
    if (WGD <= 0) return false;
    if (MDIMCD <= 0) return false;
    if (NDIMCD <= 0) return false;
    if (MDIMAD <= 0) return false;
    if (NDIMBD <= 0) return false;
    if (KWID <= 0) return false;
    if (VWMD <= 0) return false;
    if (VWND <= 0) return false;
    if (PADA < 0) return false;
    if (PADB < 0) return false;
    if (!isMultipleOf(WGD, KWID)) return false;
    if (!isMultipleOf(WGD, MDIMCD * VWMD)) return false;
    if (!isMultipleOf(WGD, NDIMCD * VWND)) return false;
    if (!isMultipleOf(WGD, MDIMAD * VWMD)) return false;
    if (!isMultipleOf(WGD, NDIMBD * VWND)) return false;
    if (!isMultipleOf(WGD, MDIMCD * NDIMCD / MDIMAD)) return false;
    if (!isMultipleOf(WGD, MDIMCD * NDIMCD / NDIMBD)) return false;
    return true;
}

string OpenCLParams::XGemmParams::desc() const {
    string s;
    s += "MWG=" + to_string(MWG);
    s += " NWG=" + to_string(NWG);
    s += " KWG=" + to_string(KWG);
    s += " MDIMC=" + to_string(MDIMC);
    s += " NDIMC=" + to_string(NDIMC);
    s += " MDIMA=" + to_string(MDIMA);
    s += " NDIMB=" + to_string(NDIMB);
    s += " KWI=" + to_string(KWI);
    s += " VWM=" + to_string(VWM);
    s += " VWN=" + to_string(VWN);
    s += " STRM=" + to_string(STRM);
    s += " STRN=" + to_string(STRN);
    s += " SA=" + to_string(SA);
    s += " SB=" + to_string(SB);
    return s;
}
string OpenCLParams::XGemmParams::compileOptions() const {
    string s;
    s += "-DMWG=" + to_string(MWG);
    s += " -DNWG=" + to_string(NWG);
    s += " -DKWG=" + to_string(KWG);
    s += " -DMDIMC=" + to_string(MDIMC);
    s += " -DNDIMC=" + to_string(NDIMC);
    s += " -DMDIMA=" + to_string(MDIMA);
    s += " -DNDIMB=" + to_string(NDIMB);
    s += " -DKWI=" + to_string(KWI);
    s += " -DVWM=" + to_string(VWM);
    s += " -DVWN=" + to_string(VWN);
    s += " -DSTRM=" + to_string(STRM);
    s += " -DSTRN=" + to_string(STRN);
    s += " -DSA=" + to_string(SA);
    s += " -DSB=" + to_string(SB);
    return s;
}
void OpenCLParams::XGemmParams::fillFromDesc(const string& fileName, const string& desc) {
    map<string, int> kvs = readDescKeyValues(fileName, desc);
    MWG = getInt(kvs, "MWG", MWG);
    NWG = getInt(kvs, "NWG", NWG);
    KWG = getInt(kvs, "KWG", KWG);
    MDIMC = getInt(kvs, "MDIMC", MDIMC);
    NDIMC = getInt(kvs, "NDIMC", NDIMC);
    MDIMA = getInt(kvs, "MDIMA", MDIMA);
    NDIMB = getInt(kvs, "NDIMB", NDIMB);
    KWI = getInt(kvs, "KWI", KWI);
    VWM = getInt(kvs, "VWM", VWM);
    VWN = getInt(kvs, "VWN", VWN);
    STRM = getInt(kvs, "STRM", STRM);
    STRN = getInt(kvs, "STRN", STRN);
    SA = getInt(kvs, "SA", SA);
    SB = getInt(kvs, "SB", SB);
}
bool OpenCLParams::XGemmParams::isValid() const {
    if (MWG <= 0) return false;
    if (NWG <= 0) return false;
    if (KWG <= 0) return false;
    if (MDIMC <= 0) return false;
    if (NDIMC <= 0) return false;
    if (MDIMA <= 0) return false;
    if (NDIMB <= 0) return false;
    if (KWI <= 0) return false;
    if (VWM <= 0) return false;
    if (VWN <= 0) return false;
    if (STRM < 0 || STRM > 1) return false;
    if (STRN < 0 || STRN > 1) return false;
    if (SA < 0 || SA > 1) return false;
    if (SB < 0 || SB > 1) return false;
    if (!isMultipleOf(KWG, KWI)) return false;
    if (!isMultipleOf(MWG, MDIMC * VWM)) return false;
    if (!isMultipleOf(NWG, NDIMC * VWN)) return false;
    if (!isMultipleOf(MWG, MDIMA * VWM)) return false;
    if (!isMultipleOf(NWG, NDIMB * VWN)) return false;
    if (!isMultipleOf(KWG, VWM)) return false;
    if (!isMultipleOf(KWG, MDIMC * NDIMC / MDIMA)) return false;
    if (!isMultipleOf(KWG, MDIMC * NDIMC / NDIMB)) return false;
    return true;
}
bool OpenCLParams::XGemmParams::isSimple() const {
    if (MDIMC != MDIMA) return false;
    if (NDIMC != NDIMB) return false;
    if (SA != SB) return false;
    if (VWM != VWN) return false;
    if (MWG != NWG) return false;
    return true;
}

string OpenCLParams::HGemmWmmaParams::desc() const {
    string s;
    s += "MWG=" + to_string(MWG);
    s += " NWG=" + to_string(NWG);
    s += " KWG=" + to_string(KWG);
    s += " MWAVE=" + to_string(MWAVE);
    s += " NWAVE=" + to_string(NWAVE);
    s += " MWARP=" + to_string(MWARP);
    s += " NWARP=" + to_string(NWARP);
    s += " VWM=" + to_string(VWM);
    s += " VWN=" + to_string(VWN);
    s += " SA=" + to_string(SA);
    s += " SB=" + to_string(SB);
    return s;
}
string OpenCLParams::HGemmWmmaParams::compileOptions() const {
    string s;
    s += "-DMWG=" + to_string(MWG);
    s += " -DNWG=" + to_string(NWG);
    s += " -DKWG=" + to_string(KWG);
    s += " -DMWAVE=" + to_string(MWAVE);
    s += " -DNWAVE=" + to_string(NWAVE);
    s += " -DMWARP=" + to_string(MWARP);
    s += " -DNWARP=" + to_string(NWARP);
    s += " -DVWM=" + to_string(VWM);
    s += " -DVWN=" + to_string(VWN);
    s += " -DSA=" + to_string(SA);
    s += " -DSB=" + to_string(SB);
    return s;
}
void OpenCLParams::HGemmWmmaParams::fillFromDesc(const string& fileName, const string& desc) {
    map<string, int> kvs = readDescKeyValues(fileName, desc);
    MWG = getInt(kvs, "MWG", MWG);
    NWG = getInt(kvs, "NWG", NWG);
    KWG = getInt(kvs, "KWG", KWG);
    MWAVE = getInt(kvs, "MWAVE", MWAVE);
    NWAVE = getInt(kvs, "NWAVE", NWAVE);
    MWARP = getInt(kvs, "MWARP", MWARP);
    NWARP = getInt(kvs, "NWARP", NWARP);
    VWM = getInt(kvs, "VWM", VWM);
    VWN = getInt(kvs, "VWN", VWN);
    SA = getInt(kvs, "SA", SA);
    SB = getInt(kvs, "SB", SB);
}
bool OpenCLParams::HGemmWmmaParams::isValid() const {
    if (MWG <= 0) return false;
    if (NWG <= 0) return false;
    if (KWG <= 0) return false;
    if (MWAVE <= 0) return false;
    if (NWAVE <= 0) return false;
    if (MWARP <= 0) return false;
    if (NWARP <= 0) return false;
    if (VWM <= 0) return false;
    if (VWN <= 0) return false;
    if (SA < 0 || SA > 1) return false;
    if (SB < 0 || SB > 1) return false;
    if (SA == 0 && VWM != 2) return false;
    if (SB == 0 && VWN != 2) return false;

    if (!isMultipleOf(MWG, VWM)) return false;
    if (!isMultipleOf(NWG, VWN)) return false;
    if (!isMultipleOf(MWG, MWAVE)) return false;
    if (!isMultipleOf(NWG, NWAVE)) return false;
    if (!isMultipleOf(MWAVE, MWARP)) return false;
    if (!isMultipleOf(NWAVE, NWARP)) return false;
    if (!((MWARP == 8 && NWARP == 32) || (MWARP == 16 && NWARP == 16) || (MWARP == 32 && NWARP == 8))) return false;

    const int WARP_SIZE = 32;
    if (MWAVE / MWARP * WARP_SIZE * NWAVE / NWARP > 1024) return false;
    return true;
}

bool OpenCLParams::HGemmWmmaParams::isSimple() const {
    if (MWAVE != MWARP && MWAVE == MWG) return false;
    if (NWAVE != NWARP && NWAVE == NWG) return false;
    if (SA != SB) return false;
    if (VWM != VWN) return false;
    if (MWG != NWG) return false;
    return true;
}

string OpenCLParams::Conv3x3Params::desc() const {
    string s;
    s += "INTILE_XSIZE=" + to_string(INTILE_XSIZE);
    s += " INTILE_YSIZE=" + to_string(INTILE_YSIZE);
    s += " OUTTILE_XSIZE=" + to_string(OUTTILE_XSIZE);
    s += " OUTTILE_YSIZE=" + to_string(OUTTILE_YSIZE);
    s += " transLocalSize0=" + to_string(transLocalSize0);
    s += " transLocalSize1=" + to_string(transLocalSize1);
    s += " untransLocalSize0=" + to_string(untransLocalSize0);
    s += " untransLocalSize1=" + to_string(untransLocalSize1);
    s += " untransLocalSize2=" + to_string(untransLocalSize2);
    return s;
}
string OpenCLParams::Conv3x3Params::transDesc() const {
    string s;
    s += " transLocalSize0=" + to_string(transLocalSize0);
    s += " transLocalSize1=" + to_string(transLocalSize1);
    return s;
}
string OpenCLParams::Conv3x3Params::untransDesc() const {
    string s;
    s += " untransLocalSize0=" + to_string(untransLocalSize0);
    s += " untransLocalSize1=" + to_string(untransLocalSize1);
    s += " untransLocalSize2=" + to_string(untransLocalSize2);
    return s;
}
string OpenCLParams::Conv3x3Params::compileOptions() const {
    string s;
    s += "-DINTILE_XSIZE=" + to_string(INTILE_XSIZE);
    s += " -DINTILE_YSIZE=" + to_string(INTILE_YSIZE);
    s += " -DOUTTILE_XSIZE=" + to_string(OUTTILE_XSIZE);
    s += " -DOUTTILE_YSIZE=" + to_string(OUTTILE_YSIZE);
    s += " -DCONV_XSIZE=3 -DCONV_YSIZE=3 -DINTILE_XOFFSET=(-1) -DINTILE_YOFFSET=(-1)";
    return s;
}
void OpenCLParams::Conv3x3Params::fillFromDesc(const string& fileName, const string& desc) {
    map<string, int> kvs = readDescKeyValues(fileName, desc);
    INTILE_XSIZE = getInt(kvs, "INTILE_XSIZE", INTILE_XSIZE);
    INTILE_YSIZE = getInt(kvs, "INTILE_YSIZE", INTILE_YSIZE);
    OUTTILE_XSIZE = getInt(kvs, "OUTTILE_XSIZE", OUTTILE_XSIZE);
    OUTTILE_YSIZE = getInt(kvs, "OUTTILE_YSIZE", OUTTILE_YSIZE);
    transLocalSize0 = getInt(kvs, "transLocalSize0", transLocalSize0);
    transLocalSize1 = getInt(kvs, "transLocalSize1", transLocalSize1);
    untransLocalSize0 = getInt(kvs, "untransLocalSize0", untransLocalSize0);
    untransLocalSize1 = getInt(kvs, "untransLocalSize1", untransLocalSize1);
    untransLocalSize2 = getInt(kvs, "untransLocalSize2", untransLocalSize2);
}
bool OpenCLParams::Conv3x3Params::isValid() const {
    if (transLocalSize0 <= 0) return false;
    if (transLocalSize1 <= 0) return false;
    if (untransLocalSize0 <= 0) return false;
    if (untransLocalSize1 <= 0) return false;
    if (untransLocalSize2 <= 0) return false;

    if (transLocalSize0 * transLocalSize1 > 1024) return false;
    if (untransLocalSize0 * untransLocalSize1 * untransLocalSize2 > 1024) return false;

    //Currently, the only supported winograd tile sizes
    if (INTILE_XSIZE == 4 && OUTTILE_XSIZE == 2 && INTILE_YSIZE == 4 && OUTTILE_YSIZE == 2)
        return true;
    if (INTILE_XSIZE == 6 && OUTTILE_XSIZE == 4 && INTILE_YSIZE == 6 && OUTTILE_YSIZE == 4)
        return true;
    return false;
}

bool OpenCLTuneParams::isValid() const {
    return
        xGemmDirect.isValid() &&
        xGemm.isValid() &&
        xGemm16.isValid() &&
        hGemmWmma.isValid() &&
        conv3x3.isValid();
}

bool OpenCLTuneParams::operator==(const OpenCLTuneParams& other) const {
    if (this == &other)
        return true;
    return std::memcmp(this, &other, sizeof(OpenCLTuneParams)) == 0;
}

int OpenCLTuneParams::getXGemmMPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const {
    if (usingFP16TensorCores) {
        return hGemmWmma.MWG;
    }
    if (usingFP16Compute) {
        return xGemm16.MWG;
    }
    return xGemm.MWG;
}
int OpenCLTuneParams::getXGemmNPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const {
    if (usingFP16TensorCores) {
        return hGemmWmma.NWG;
    }
    if (usingFP16Compute) {
        return xGemm16.NWG;
    }
    return xGemm.NWG;
}
int OpenCLTuneParams::getXGemmKPaddingMult(bool usingFP16Compute, bool usingFP16TensorCores) const {
    if (usingFP16TensorCores) {
        return hGemmWmma.KWG;
    }
    if (usingFP16Compute) {
        return xGemm16.KWG;
    }
    return xGemm.KWG;
}

static const int TUNER_VERSION = 8;
static const char* TUNEPARAMS_VERSION_LINE = "VERSION=8";
void OpenCLTuneParams::save(const string& filename, const OpenCLTuneParams& config) {
    ofstream out(filename);
    if (out.fail())
        throw IOError("Could not create file: " + filename);
    out << TUNEPARAMS_VERSION_LINE << "\n";
    out << "#shouldUseFP16Storage" << "\n";
    out << config.shouldUseFP16Storage << "\n";
    out << "#shouldUseFP16Compute" << "\n";
    out << config.shouldUseFP16Compute << "\n";
    out << "#shouldUseFP16TensorCores" << "\n";
    out << config.shouldUseFP16TensorCores << "\n";
    out << "#xGemmDirect" << "\n";
    out << config.xGemmDirect.desc() << "\n";
    out << "#xGemm" << "\n";
    out << config.xGemm.desc() << "\n";
    out << "#xGemm16" << "\n";
    out << config.xGemm16.desc() << "\n";
    out << "#hGemmWmma" << "\n";
    out << config.hGemmWmma.desc() << "\n";
    out << "#conv3x3" << "\n";
    out << config.conv3x3.desc() << "\n";
    out.flush();
    out.close();
}

static cl_mem constantReadOnlyBufferFloat(cl_context context, int numElts, float constant) {
    vector<float> buf(numElts);
    for (int i = 0; i < numElts; i++)
        buf[i] = constant;
    return createReadOnlyBuffer(context, buf);
}
static cl_mem randomReadOnlyBufferFloat(const int64_t seed, cl_context context, int numElts, double scale) {
    vector<float> buf(numElts);
    mt19937_64 mt(seed);
    uniform_real_distribution<float> rand(0.0, scale);
    for (int i = 0; i < numElts; i++)
        buf[i] = rand(mt);
    return createReadOnlyBuffer(context, buf);
}
static cl_mem randomReadOnlyBufferHalf(const int64_t seed, cl_context context, int numElts, double scale) {
    vector<half_t> buf(numElts);
    mt19937_64 mt(seed);
    uniform_real_distribution<double> rand(0.0, scale);
    for (int i = 0; i < numElts; i++)
        buf[i] = half_float::half_cast<half_t>(rand(mt));
    return createReadOnlyBuffer(context, buf);
}
static cl_mem randomReadOnly3dPaddedBufferFloat(
    const int64_t seed, cl_context context,
    int batchSize, int ySize, int ySizePadded, int xSize, int xSizePadded,
    double scale
) {
    vector<float> buf((size_t)batchSize * ySizePadded * xSizePadded);
    mt19937_64 mt(seed);
    uniform_real_distribution<float> rand(0.0, scale);
    size_t i = 0;
    for (int n = 0; n < batchSize; n++) {
        for (int y = 0; y < ySizePadded; y++) {
            for (int x = 0; x < xSizePadded; x++) {
                if (y < ySize && x < xSize)
                    buf[i++] = rand(mt);
                else
                    buf[i++] = 0.0f;
            }
        }
    }
    return createReadOnlyBuffer(context, buf);
}
static cl_mem randomReadOnly3dPaddedBufferHalf(
    const int64_t seed, cl_context context,
    int batchSize, int ySize, int ySizePadded, int xSize, int xSizePadded,
    double scale
) {
    vector<half_t> buf((size_t)batchSize * ySizePadded * xSizePadded);
    mt19937_64 mt(seed);
    uniform_real_distribution<double> rand(0.0, scale);
    size_t i = 0;
    for (int n = 0; n < batchSize; n++) {
        for (int y = 0; y < ySizePadded; y++) {
            for (int x = 0; x < xSizePadded; x++) {
                if (y < ySize && x < xSize)
                    buf[i++] = half_float::half_cast<half_t>(rand(mt));
                else
                    buf[i++] = half_float::half_cast<half_t>(0.0f);
            }
        }
    }
    return createReadOnlyBuffer(context, buf);
}



template<typename T>
static void addConfigs(
    vector<OpenCLTuneParams>& configs,
    std::function<void(OpenCLTuneParams&, T value)> apply,
    const vector<T>& values
) {
    vector<OpenCLTuneParams> newCfgs;
    for (int i = 0; i < values.size(); i++) {
        for (int j = 0; j < configs.size(); j++) {
            OpenCLTuneParams cfg = configs[j];
            apply(cfg, values[i]);
            newCfgs.push_back(cfg);
        }
    }
    configs = newCfgs;
}

static void filterConfigs(
    vector<OpenCLTuneParams>& configs,
    std::function<bool(const OpenCLTuneParams&)> isValid
) {
    vector<OpenCLTuneParams> newCfgs;
    for (int j = 0; j < configs.size(); j++) {
        if (isValid(configs[j]))
            newCfgs.push_back(configs[j]);
    }
    configs = newCfgs;
}

static void shuffleConfigs(
    vector<OpenCLTuneParams>& configs
) {
    mt19937_64 mt(0);
    if (configs.size() == 0)
        return;
    for (int i = configs.size() - 1; i > 0; i--) {
        uniform_int_distribution<int> rand(0, i + 1);
        int j = rand(mt);
        std::swap(configs[i], configs[j]);
    }
}

struct OpenCLTuneAccums {
    bool bad = false;
    cl_int badErr = 0;
    string detailedErrorMessage;
    double weightCounted = 0;
    double weightedTimeTaken = 0;

    void countResultAndFreeEvent(cl_int err, cl_event event, double weight) {
        if (err != 0) {
            if (!bad) {
                bad = true;
                badErr = err;
            }
            return;
        }

        err = clWaitForEvents(1, &event);
        //If the kernel does bad things the error might also pop up here
        if (err != 0) {
            if (!bad) {
                bad = true;
                badErr = err;
            }
            return;
        }

        cl_ulong time_start, time_end;
        err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL); CHECK_ERR(err);
        err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL); CHECK_ERR(err);

        weightedTimeTaken += (time_end - time_start) * 1e-9 * weight;
        weightCounted += weight;

        clReleaseEvent(event);
    }

};

static bool testAllConfigs(
    bool stopOnReferenceImplFail,
    const vector<OpenCLTuneParams>& configsToTest,
    OpenCLTuneParams& currentConfig,
    OpenCLTuneParams referenceConfig,
    ostream& out,
    bool verboseErrors,
    bool verboseTuner,
    double errorToleranceScale,
    std::function<string(const OpenCLTuneParams&)> getDesc,
    std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)> testConfig,
    double& bestKernelsPerSecondBuf
) {
    vector<OpenCLTuneParams> configs = configsToTest;

    //Insert the reference configuration first
    configs.insert(configs.begin(), referenceConfig);

    double bestScore = 0.0;
    double bestKernelsPerSecond = 0.0;
    int lastBestIdx = 0;
    bool anythingGoodYet = false;
    int numTested = 0;
    int numTestedRunnable = 0;

    vector<float> referenceRet;
    vector<float> ret;

    out << "Testing " << configs.size() << " different configs" << endl;
    for (int i = 0; i < configs.size(); i++) {
        OpenCLTuneAccums accums = testConfig(configs[i], ret);

        numTested++;
        if (accums.bad) {
            if (verboseErrors) {
                out << "Tuning " << i << "/" << configs.size() << " failed: " << getErrorMessage(accums.badErr) << endl;
                if (accums.detailedErrorMessage.size() > 0)
                    out << accums.detailedErrorMessage << endl;
            }
            if (i == 0) {
                if (stopOnReferenceImplFail)
                    return false;
                out << "WARNING: Reference implementation failed: " << getErrorMessage(accums.badErr) << endl;
            }
        }
        else {
            if (!anythingGoodYet) {
                //Just use the first thing that worked as the reference
                //Unless something has gone really weird, this should be the reference implementation
                referenceRet = ret;
                anythingGoodYet = true;
            }

            numTestedRunnable++;

            double squerr = 0.0;
            double sqmag = 0.0;
            if (referenceRet.size() != ret.size())
                squerr = std::numeric_limits<double>::infinity();
            else {
                for (int j = 0; j < referenceRet.size(); j++) {
                    if (!isfinite(referenceRet[j]) || !isfinite(ret[j]))
                        squerr = std::numeric_limits<double>::infinity();
                    else {
                        double diff = (double)referenceRet[j] - (double)ret[j];
                        squerr += diff * diff;
                        sqmag += (double)referenceRet[j] * (double)referenceRet[j];
                    }
                }
            }

            double kernelsPerSecond = accums.weightCounted / accums.weightedTimeTaken;
            double errorProp = sqrt(squerr / (sqmag + 1e-30));
            if (!isfinite(errorProp) || errorProp > 1.0)
                errorProp = 1.0;

            double score = kernelsPerSecond * (1.0 - sqrt(errorProp / (errorProp + errorToleranceScale)));
            if (verboseTuner || score > bestScore) {
                out << "Tuning " << i << "/" << configs.size()
                    << (i == 0 ? " (reference)" : "")
                    << " Calls/sec " << kernelsPerSecond
                    << " L2Error " << squerr
                    << " " << getDesc(configs[i]) << endl;
            }
            if (score > bestScore) {
                bestKernelsPerSecond = kernelsPerSecond;
                bestScore = score;
                currentConfig = configs[i];
                lastBestIdx = i;
            }
        }
        if (i % 20 == 0 && i >= lastBestIdx + 10)
            out << "Tuning " << i << "/" << configs.size() << " ..." << endl;
    }
    if (!anythingGoodYet) {
        out << "ERROR: Could not find any configuration that worked" << endl;
        return false;
    }

    bestKernelsPerSecondBuf = bestKernelsPerSecond;
    return true;
}

#define SETTER(field) std::function<void(OpenCLTuneParams&, int value)>([](OpenCLTuneParams& p, int value){ p.field = value; })
#define ISVALID(field) std::function<bool(const OpenCLTuneParams&)>([](const OpenCLTuneParams& p){ return p.field.isValid(); })
#define ISSIMPLE(field) std::function<bool(const OpenCLTuneParams&)>([](const OpenCLTuneParams& p){ return p.field.isSimple(); })

static void tuneXGemmDirect(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig
) {
    out << "------------------------------------------------------" << endl;
    out << "Tuning xGemmDirect for 1x1 convolutions and matrix mult" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(xGemmDirect.WGD), { 8,16,32,64 });
        addConfigs(configs, SETTER(xGemmDirect.MDIMCD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.NDIMCD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.MDIMAD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.NDIMBD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.KWID), { 2,8,16 });
        addConfigs(configs, SETTER(xGemmDirect.VWMD), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemmDirect.VWND), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemmDirect.PADA), { 1 });
        addConfigs(configs, SETTER(xGemmDirect.PADB), { 1 });
    }
    else {
        addConfigs(configs, SETTER(xGemmDirect.WGD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.MDIMCD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.NDIMCD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.MDIMAD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.NDIMBD), { 8,16,32 });
        addConfigs(configs, SETTER(xGemmDirect.KWID), { 2,8 });
        addConfigs(configs, SETTER(xGemmDirect.VWMD), { 2,4 });
        addConfigs(configs, SETTER(xGemmDirect.VWND), { 2,4 });
        addConfigs(configs, SETTER(xGemmDirect.PADA), { 1 });
        addConfigs(configs, SETTER(xGemmDirect.PADB), { 1 });
    }

    filterConfigs(configs, ISVALID(xGemmDirect));
    shuffleConfigs(configs);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.xGemmDirect.WGD = untunedConfig.xGemmDirect.WGD;
    referenceConfig.xGemmDirect.MDIMCD = untunedConfig.xGemmDirect.MDIMCD;
    referenceConfig.xGemmDirect.NDIMCD = untunedConfig.xGemmDirect.NDIMCD;
    referenceConfig.xGemmDirect.MDIMAD = untunedConfig.xGemmDirect.MDIMAD;
    referenceConfig.xGemmDirect.NDIMBD = untunedConfig.xGemmDirect.NDIMBD;
    referenceConfig.xGemmDirect.KWID = untunedConfig.xGemmDirect.KWID;
    referenceConfig.xGemmDirect.VWMD = untunedConfig.xGemmDirect.VWMD;
    referenceConfig.xGemmDirect.VWND = untunedConfig.xGemmDirect.VWND;
    referenceConfig.xGemmDirect.PADA = untunedConfig.xGemmDirect.PADA;
    referenceConfig.xGemmDirect.PADB = untunedConfig.xGemmDirect.PADB;
    OpenCLTuneParams slightlyTunedConfig = referenceConfig;
    slightlyTunedConfig.xGemmDirect.MDIMCD = 8;
    slightlyTunedConfig.xGemmDirect.NDIMCD = 8;
    slightlyTunedConfig.xGemmDirect.MDIMAD = 8;
    slightlyTunedConfig.xGemmDirect.NDIMBD = 8;
    OpenCLTuneParams slightlyTunedConfig2 = slightlyTunedConfig;
    slightlyTunedConfig2.xGemmDirect.WGD = 16;

    configs.insert(configs.begin(), slightlyTunedConfig2);
    configs.insert(configs.begin(), slightlyTunedConfig);
    configs.insert(configs.begin(), currentConfig);

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.xGemmDirect.desc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "xgemmDirectProgram", context, deviceIdsToUse, OpenCLKernels::xgemmDirect,
            cfg.xGemmDirect.compileOptions() + " -DROUTINE_GEMMSTRIDEDBATCHED",
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "XgemmDirectStridedBatchedNN", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int maxChannels = FEATURES1_NUM;
        maxChannels = std::max(FEATURES2_NUM, maxChannels);
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);
        maxChannels = std::max(MAX_MOVE_LABEL_NUM, maxChannels);

        int ioNumFloats = batchSize * nnXLen * nnYLen * maxChannels;
        int filterNumFloats = maxChannels * maxChannels;
        cl_mem input = randomReadOnlyBufferFloat(6381147743675501234ULL/*tuneXGemmDirectInput*/, context, ioNumFloats, 1.0);
        cl_mem filter = randomReadOnlyBufferFloat(1247869217574235315ULL/*tuneXGemmDirectFilter*/, context, filterNumFloats, 1.0 / sqrt(maxChannels));
        cl_mem output = createReadWriteBufferFloat(context, ioNumFloats);

        const int reps = 4;
        for (int i = 0; i < reps; i++) {
            int inChannels;
            int outChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: inChannels = FEATURES1_NUM; outChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: inChannels = FEATURES1_NUM; outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: inChannels = FEATURES2_NUM; outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 3: inChannels = modelInfo.trunkNumChannels; outChannels = MAX_MOVE_LABEL_NUM; weight = 1; break;
            }

            int filterStride = 0; //Reuse same filter for all matrices in batch
            int inputStride = nnXLen * nnYLen * inChannels;
            int outputStride = nnXLen * nnYLen * outChannels;

            cl_event event;
            err = doStridedBatchedXGemmDirect_KM_KN_NM(
                kernel,
                commandQueue,
                cfg,
                nnXLen * nnYLen, outChannels, inChannels,
                inputStride, filterStride, outputStride,
                input, filter, output,
                batchSize,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(ioNumFloats, 0.0);
        else
            blockingReadBuffer(commandQueue, output, ioNumFloats, ret);

        clReleaseMemObject(input);
        clReleaseMemObject(filter);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = false;
    double bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.05;
    testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );
    tunedConfig = currentConfig;
}

static bool tuneXGemm(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    bool useFP16Storage,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig,
    double& bestKernelsPerSecond
) {
    out << "------------------------------------------------------" << endl;
    if (useFP16Storage)
        out << "Tuning xGemm for convolutions - trying with FP16 storage" << endl;
    else
        out << "Tuning xGemm for convolutions" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(xGemm.MWG), { 8,16,32,64,128 });
        addConfigs(configs, SETTER(xGemm.NWG), { 8,16,32,64,128 });
        addConfigs(configs, SETTER(xGemm.KWG), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.MDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.NDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.MDIMA), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.NDIMB), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.KWI), { 2,8 });
        addConfigs(configs, SETTER(xGemm.VWM), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemm.VWN), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemm.STRM), { 0 });
        addConfigs(configs, SETTER(xGemm.STRN), { 0 });
        addConfigs(configs, SETTER(xGemm.SA), { 0,1 });
        addConfigs(configs, SETTER(xGemm.SB), { 0,1 });
        filterConfigs(configs, ISVALID(xGemm));
    }
    else {
        addConfigs(configs, SETTER(xGemm.MWG), { 16,32,64 });
        addConfigs(configs, SETTER(xGemm.NWG), { 16,32,64 });
        addConfigs(configs, SETTER(xGemm.KWG), { 16,32 });
        addConfigs(configs, SETTER(xGemm.MDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.NDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.MDIMA), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.NDIMB), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm.KWI), { 2 });
        addConfigs(configs, SETTER(xGemm.VWM), { 2,4 });
        addConfigs(configs, SETTER(xGemm.VWN), { 2,4 });
        addConfigs(configs, SETTER(xGemm.STRM), { 0 });
        addConfigs(configs, SETTER(xGemm.STRN), { 0 });
        addConfigs(configs, SETTER(xGemm.SA), { 0,1 });
        addConfigs(configs, SETTER(xGemm.SB), { 0,1 });
        filterConfigs(configs, ISVALID(xGemm));
        filterConfigs(configs, ISSIMPLE(xGemm));
    }

    shuffleConfigs(configs);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.xGemm.MWG = untunedConfig.xGemm.MWG;
    referenceConfig.xGemm.NWG = untunedConfig.xGemm.NWG;
    referenceConfig.xGemm.KWG = untunedConfig.xGemm.KWG;
    referenceConfig.xGemm.MDIMC = untunedConfig.xGemm.MDIMC;
    referenceConfig.xGemm.NDIMC = untunedConfig.xGemm.NDIMC;
    referenceConfig.xGemm.MDIMA = untunedConfig.xGemm.MDIMA;
    referenceConfig.xGemm.NDIMB = untunedConfig.xGemm.NDIMB;
    referenceConfig.xGemm.KWI = untunedConfig.xGemm.KWI;
    referenceConfig.xGemm.VWM = untunedConfig.xGemm.VWM;
    referenceConfig.xGemm.VWN = untunedConfig.xGemm.VWN;
    referenceConfig.xGemm.STRM = untunedConfig.xGemm.STRM;
    referenceConfig.xGemm.STRN = untunedConfig.xGemm.STRN;
    referenceConfig.xGemm.SA = untunedConfig.xGemm.SA;
    referenceConfig.xGemm.SB = untunedConfig.xGemm.SB;

    OpenCLTuneParams slightlyTunedConfig = referenceConfig;
    slightlyTunedConfig.xGemm.MDIMC = 8;
    slightlyTunedConfig.xGemm.NDIMC = 8;
    slightlyTunedConfig.xGemm.MDIMA = 8;
    slightlyTunedConfig.xGemm.NDIMB = 8;
    OpenCLTuneParams slightlyTunedConfig2 = slightlyTunedConfig;
    slightlyTunedConfig2.xGemm.MWG = 16;
    slightlyTunedConfig2.xGemm.NWG = 16;
    slightlyTunedConfig2.xGemm.KWG = 16;

    configs.insert(configs.begin(), slightlyTunedConfig2);
    configs.insert(configs.begin(), slightlyTunedConfig);
    configs.insert(configs.begin(), currentConfig);

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.xGemm.desc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "xgemmProgram", context, deviceIdsToUse, OpenCLKernels::xgemm,
            cfg.xGemm.compileOptions() + (useFP16Storage ? OpenCLKernels::fp16StorageDefine : ""),
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "XgemmBatched", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int numTilesX = (nnXLen + cfg.conv3x3.OUTTILE_XSIZE - 1) / cfg.conv3x3.OUTTILE_XSIZE;
        int numTilesY = (nnYLen + cfg.conv3x3.OUTTILE_YSIZE - 1) / cfg.conv3x3.OUTTILE_YSIZE;
        int numTilesTotal = batchSize * numTilesX * numTilesY;

        int inTileXSize = cfg.conv3x3.INTILE_XSIZE;
        int inTileYSize = cfg.conv3x3.INTILE_YSIZE;
        int inTileXYSize = inTileXSize * inTileYSize;

        int maxChannels = FEATURES1_NUM;
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);

        int numTilesTotalPadded = roundUpToMultiple(numTilesTotal, cfg.xGemm.MWG);
        int maxOutChannelsPadded = roundUpToMultiple(maxChannels, cfg.xGemm.NWG);
        int maxInChannelsPadded = roundUpToMultiple(maxChannels, cfg.xGemm.KWG);

        int outNumFloats = numTilesTotalPadded * maxOutChannelsPadded * inTileXYSize;
        cl_mem input;
        cl_mem filter;
        cl_mem output;
        if (useFP16Storage) {
            input = randomReadOnly3dPaddedBufferHalf(
                4642632101795320974ULL/*tuneXGemm3x3Input*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, numTilesTotal, numTilesTotalPadded, 1.0);
            filter = randomReadOnly3dPaddedBufferHalf(
                1602854403103414031ULL/*tuneXGemm3x3Filter*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, maxChannels, maxOutChannelsPadded, 1.0 / sqrt(maxChannels * 3 * 3));
            output = createReadWriteBufferHalf(context, outNumFloats);
        }
        else {
            input = randomReadOnly3dPaddedBufferFloat(
                4642632101795320974ULL/*tuneXGemm3x3Input*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, numTilesTotal, numTilesTotalPadded, 1.0);
            filter = randomReadOnly3dPaddedBufferFloat(
                1602854403103414031ULL/*tuneXGemm3x3Filter*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, maxChannels, maxOutChannelsPadded, 1.0 / sqrt(maxChannels * 3 * 3));
            output = createReadWriteBufferFloat(context, outNumFloats);
        }

        const int reps = 3;
        for (int i = 0; i < reps; i++) {
            int inChannels;
            int outChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: inChannels = FEATURES1_NUM; outChannels = modelInfo.trunkNumChannels; weight = 0.02; break;
            }

            int outChannelsPadded = roundUpToMultiple(outChannels, cfg.xGemm.NWG);
            int inChannelsPadded = roundUpToMultiple(inChannels, cfg.xGemm.KWG);

            cl_event event;
            err = doBatchedXGemm_KM_KN_NM(
                kernel,
                commandQueue,
                cfg.xGemm,
                numTilesTotalPadded, outChannelsPadded, inChannelsPadded,
                input, filter, output,
                inTileXYSize,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(outNumFloats, 0.0);
        else if (useFP16Storage)
            blockingReadBufferHalfToFloat(commandQueue, output, outNumFloats, ret);
        else
            blockingReadBuffer(commandQueue, output, outNumFloats, ret);

        //Compact ret down to only what we were supposed to get, without padding
        {
            int i = 0;
            for (int n = 0; n < inTileXYSize; n++) {
                for (int y = 0; y < maxChannels; y++) {
                    for (int x = 0; x < numTilesTotal; x++) {
                        ret[i++] = ret[x + numTilesTotalPadded * (y + maxOutChannelsPadded * n)];
                    }
                }
            }
            ret.resize(inTileXYSize * maxChannels * numTilesTotal);
        }

        clReleaseMemObject(input);
        clReleaseMemObject(filter);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = useFP16Storage;
    bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.05;
    bool suc = testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );
    tunedConfig = currentConfig;
    return suc;
}

static bool tuneXGemm16(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig,
    double& bestKernelsPerSecond
) {
    out << "------------------------------------------------------" << endl;
    out << "Tuning xGemm16 for convolutions" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(xGemm16.MWG), { 8,16,32,64,128 });
        addConfigs(configs, SETTER(xGemm16.NWG), { 8,16,32,64,128 });
        addConfigs(configs, SETTER(xGemm16.KWG), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.MDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.NDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.MDIMA), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.NDIMB), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.KWI), { 2,8 });
        addConfigs(configs, SETTER(xGemm16.VWM), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemm16.VWN), { 1,2,4,8 });
        addConfigs(configs, SETTER(xGemm16.STRM), { 0 });
        addConfigs(configs, SETTER(xGemm16.STRN), { 0 });
        addConfigs(configs, SETTER(xGemm16.SA), { 0,1 });
        addConfigs(configs, SETTER(xGemm16.SB), { 0,1 });
        filterConfigs(configs, ISVALID(xGemm16));
    }
    else {
        addConfigs(configs, SETTER(xGemm16.MWG), { 16,32,64 });
        addConfigs(configs, SETTER(xGemm16.NWG), { 16,32,64 });
        addConfigs(configs, SETTER(xGemm16.KWG), { 16,32 });
        addConfigs(configs, SETTER(xGemm16.MDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.NDIMC), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.MDIMA), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.NDIMB), { 8,16,32 });
        addConfigs(configs, SETTER(xGemm16.KWI), { 2 });
        addConfigs(configs, SETTER(xGemm16.VWM), { 2,4 });
        addConfigs(configs, SETTER(xGemm16.VWN), { 2,4 });
        addConfigs(configs, SETTER(xGemm16.STRM), { 0 });
        addConfigs(configs, SETTER(xGemm16.STRN), { 0 });
        addConfigs(configs, SETTER(xGemm16.SA), { 0,1 });
        addConfigs(configs, SETTER(xGemm16.SB), { 0,1 });
        filterConfigs(configs, ISVALID(xGemm16));
        filterConfigs(configs, ISSIMPLE(xGemm16));
    }

    shuffleConfigs(configs);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.xGemm16.MWG = untunedConfig.xGemm16.MWG;
    referenceConfig.xGemm16.NWG = untunedConfig.xGemm16.NWG;
    referenceConfig.xGemm16.KWG = untunedConfig.xGemm16.KWG;
    referenceConfig.xGemm16.MDIMC = untunedConfig.xGemm16.MDIMC;
    referenceConfig.xGemm16.NDIMC = untunedConfig.xGemm16.NDIMC;
    referenceConfig.xGemm16.MDIMA = untunedConfig.xGemm16.MDIMA;
    referenceConfig.xGemm16.NDIMB = untunedConfig.xGemm16.NDIMB;
    referenceConfig.xGemm16.KWI = untunedConfig.xGemm16.KWI;
    referenceConfig.xGemm16.VWM = untunedConfig.xGemm16.VWM;
    referenceConfig.xGemm16.VWN = untunedConfig.xGemm16.VWN;
    referenceConfig.xGemm16.STRM = untunedConfig.xGemm16.STRM;
    referenceConfig.xGemm16.STRN = untunedConfig.xGemm16.STRN;
    referenceConfig.xGemm16.SA = untunedConfig.xGemm16.SA;
    referenceConfig.xGemm16.SB = untunedConfig.xGemm16.SB;

    OpenCLTuneParams slightlyTunedConfig = referenceConfig;
    slightlyTunedConfig.xGemm16.MDIMC = 8;
    slightlyTunedConfig.xGemm16.NDIMC = 8;
    slightlyTunedConfig.xGemm16.MDIMA = 8;
    slightlyTunedConfig.xGemm16.NDIMB = 8;
    OpenCLTuneParams slightlyTunedConfig2 = slightlyTunedConfig;
    slightlyTunedConfig2.xGemm16.MWG = 16;
    slightlyTunedConfig2.xGemm16.NWG = 16;
    slightlyTunedConfig2.xGemm16.KWG = 16;

    configs.insert(configs.begin(), slightlyTunedConfig2);
    configs.insert(configs.begin(), slightlyTunedConfig);
    configs.insert(configs.begin(), currentConfig);

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.xGemm16.desc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "xgemmProgram", context, deviceIdsToUse, OpenCLKernels::xgemm,
            cfg.xGemm16.compileOptions() + OpenCLKernels::fp16StorageDefine + OpenCLKernels::fp16ComputeDefine,
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "XgemmBatched", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int numTilesX = (nnXLen + cfg.conv3x3.OUTTILE_XSIZE - 1) / cfg.conv3x3.OUTTILE_XSIZE;
        int numTilesY = (nnYLen + cfg.conv3x3.OUTTILE_YSIZE - 1) / cfg.conv3x3.OUTTILE_YSIZE;
        int numTilesTotal = batchSize * numTilesX * numTilesY;

        int inTileXSize = cfg.conv3x3.INTILE_XSIZE;
        int inTileYSize = cfg.conv3x3.INTILE_YSIZE;
        int inTileXYSize = inTileXSize * inTileYSize;

        int maxChannels = FEATURES1_NUM;;
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);

        int numTilesTotalPadded = roundUpToMultiple(numTilesTotal, cfg.xGemm16.MWG);
        int maxOutChannelsPadded = roundUpToMultiple(maxChannels, cfg.xGemm16.NWG);
        int maxInChannelsPadded = roundUpToMultiple(maxChannels, cfg.xGemm16.KWG);

        int outNumFloats = numTilesTotalPadded * maxOutChannelsPadded * inTileXYSize;
        cl_mem input = randomReadOnly3dPaddedBufferHalf(
            4642632101795320974ULL/*tuneXGemm3x3Input*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, numTilesTotal, numTilesTotalPadded, 1.0);
        cl_mem filter = randomReadOnly3dPaddedBufferHalf(
            1602854403103414031ULL/*tuneXGemm3x3Filter*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, maxChannels, maxOutChannelsPadded, 1.0 / sqrt(maxChannels * 3 * 3));
        cl_mem output = createReadWriteBufferHalf(context, outNumFloats);

        const int reps = 3;
        for (int i = 0; i < reps; i++) {
            int inChannels;
            int outChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: inChannels = FEATURES1_NUM;; outChannels = modelInfo.trunkNumChannels; weight = 0.02; break;
            }

            int outChannelsPadded = roundUpToMultiple(outChannels, cfg.xGemm16.NWG);
            int inChannelsPadded = roundUpToMultiple(inChannels, cfg.xGemm16.KWG);

            cl_event event;
            err = doBatchedXGemm_KM_KN_NM(
                kernel,
                commandQueue,
                cfg.xGemm16,
                numTilesTotalPadded, outChannelsPadded, inChannelsPadded,
                input, filter, output,
                inTileXYSize,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(outNumFloats, 0.0);
        else
            blockingReadBufferHalfToFloat(commandQueue, output, outNumFloats, ret);

        //Compact ret down to only what we were supposed to get, without padding
        {
            int i = 0;
            for (int n = 0; n < inTileXYSize; n++) {
                for (int y = 0; y < maxChannels; y++) {
                    for (int x = 0; x < numTilesTotal; x++) {
                        ret[i++] = ret[x + numTilesTotalPadded * (y + maxOutChannelsPadded * n)];
                    }
                }
            }
            ret.resize(inTileXYSize * maxChannels * numTilesTotal);
        }

        clReleaseMemObject(input);
        clReleaseMemObject(filter);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = true;
    bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.05;
    bool suc = testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );
    if (suc) {
        tunedConfig = currentConfig;
    }
    return suc;
}


static bool tuneHGemmWmma(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig,
    double& bestKernelsPerSecond
) {
    out << "------------------------------------------------------" << endl;
    out << "Tuning hGemmWmma for convolutions" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(hGemmWmma.MWG), { 16,32,64,128 });
        addConfigs(configs, SETTER(hGemmWmma.NWG), { 16,32,64,128 });
        addConfigs(configs, SETTER(hGemmWmma.KWG), { 16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.MWAVE), { 8,16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.NWAVE), { 8,16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.MWARP), { 8,16,32 });
        addConfigs(configs, SETTER(hGemmWmma.NWARP), { 8,16,32 });
        addConfigs(configs, SETTER(hGemmWmma.VWM), { 2,4,8 });
        addConfigs(configs, SETTER(hGemmWmma.VWN), { 2,4,8 });
        addConfigs(configs, SETTER(hGemmWmma.SA), { 0,1 });
        addConfigs(configs, SETTER(hGemmWmma.SB), { 0,1 });
        filterConfigs(configs, ISVALID(hGemmWmma));
    }
    else {
        addConfigs(configs, SETTER(hGemmWmma.MWG), { 16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.NWG), { 16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.KWG), { 16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.MWAVE), { 8,16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.NWAVE), { 8,16,32,64 });
        addConfigs(configs, SETTER(hGemmWmma.MWARP), { 8,16,32 });
        addConfigs(configs, SETTER(hGemmWmma.NWARP), { 8,16,32 });
        addConfigs(configs, SETTER(hGemmWmma.VWM), { 2,4 });
        addConfigs(configs, SETTER(hGemmWmma.VWN), { 2,4 });
        addConfigs(configs, SETTER(hGemmWmma.SA), { 0,1 });
        addConfigs(configs, SETTER(hGemmWmma.SB), { 0,1 });
        filterConfigs(configs, ISVALID(hGemmWmma));
        filterConfigs(configs, ISSIMPLE(hGemmWmma));
    }

    shuffleConfigs(configs);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.hGemmWmma.MWG = untunedConfig.hGemmWmma.MWG;
    referenceConfig.hGemmWmma.NWG = untunedConfig.hGemmWmma.NWG;
    referenceConfig.hGemmWmma.KWG = untunedConfig.hGemmWmma.KWG;
    referenceConfig.hGemmWmma.MWAVE = untunedConfig.hGemmWmma.MWAVE;
    referenceConfig.hGemmWmma.NWAVE = untunedConfig.hGemmWmma.NWAVE;
    referenceConfig.hGemmWmma.MWARP = untunedConfig.hGemmWmma.MWARP;
    referenceConfig.hGemmWmma.NWARP = untunedConfig.hGemmWmma.NWARP;
    referenceConfig.hGemmWmma.VWM = untunedConfig.hGemmWmma.VWM;
    referenceConfig.hGemmWmma.VWN = untunedConfig.hGemmWmma.VWN;
    referenceConfig.hGemmWmma.SA = untunedConfig.hGemmWmma.SA;
    referenceConfig.hGemmWmma.SB = untunedConfig.hGemmWmma.SB;

    configs.insert(configs.begin(), currentConfig);

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.hGemmWmma.desc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "hgemmWmmaProgram", context, deviceIdsToUse, OpenCLKernels::hgemmWmma,
            cfg.hGemmWmma.compileOptions() + OpenCLKernels::fp16StorageDefine,
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "hgemmWmmaBatched", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int numTilesX = (nnXLen + cfg.conv3x3.OUTTILE_XSIZE - 1) / cfg.conv3x3.OUTTILE_XSIZE;
        int numTilesY = (nnYLen + cfg.conv3x3.OUTTILE_YSIZE - 1) / cfg.conv3x3.OUTTILE_YSIZE;
        int numTilesTotal = batchSize * numTilesX * numTilesY;

        int inTileXSize = cfg.conv3x3.INTILE_XSIZE;
        int inTileYSize = cfg.conv3x3.INTILE_YSIZE;
        int inTileXYSize = inTileXSize * inTileYSize;

        int maxChannels = FEATURES1_NUM;;
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);

        int numTilesTotalPadded = roundUpToMultiple(numTilesTotal, cfg.hGemmWmma.MWG);
        int maxOutChannelsPadded = roundUpToMultiple(maxChannels, cfg.hGemmWmma.NWG);
        int maxInChannelsPadded = roundUpToMultiple(maxChannels, cfg.hGemmWmma.KWG);

        int outNumFloats = numTilesTotalPadded * maxOutChannelsPadded * inTileXYSize;
        cl_mem input = randomReadOnly3dPaddedBufferHalf(
            7853736013337238298ULL/*tuneHGemmWmma3x3Input*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, numTilesTotal, numTilesTotalPadded, 1.0);
        cl_mem filter = randomReadOnly3dPaddedBufferHalf(
            16554842652272687981ULL/*tuneHGemmWmma3x3Filter*/, context, inTileXYSize, maxChannels, maxInChannelsPadded, maxChannels, maxOutChannelsPadded, 1.0 / sqrt(maxChannels * 3 * 3));
        cl_mem output = createReadWriteBufferHalf(context, outNumFloats);

        const int reps = 3;
        for (int i = 0; i < reps; i++) {
            int inChannels;
            int outChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: inChannels = modelInfo.trunkNumChannels; outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: inChannels = FEATURES1_NUM;; outChannels = modelInfo.trunkNumChannels; weight = 0.02; break;
            }

            int outChannelsPadded = roundUpToMultiple(outChannels, cfg.hGemmWmma.NWG);
            int inChannelsPadded = roundUpToMultiple(inChannels, cfg.hGemmWmma.KWG);

            cl_event event;
            err = doBatchedHGemmWmma_KM_KN_NM(
                kernel,
                commandQueue,
                cfg,
                numTilesTotalPadded, outChannelsPadded, inChannelsPadded,
                input, filter, output,
                inTileXYSize,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(outNumFloats, 0.0);
        else
            blockingReadBufferHalfToFloat(commandQueue, output, outNumFloats, ret);

        //Compact ret down to only what we were supposed to get, without padding
        {
            int i = 0;
            for (int n = 0; n < inTileXYSize; n++) {
                for (int y = 0; y < maxChannels; y++) {
                    for (int x = 0; x < numTilesTotal; x++) {
                        ret[i++] = ret[x + numTilesTotalPadded * (y + maxOutChannelsPadded * n)];
                    }
                }
            }
            ret.resize(inTileXYSize * maxChannels * numTilesTotal);
        }

        clReleaseMemObject(input);
        clReleaseMemObject(filter);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = true;
    bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.02;
    bool suc = testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );
    if (suc) {
        tunedConfig = currentConfig;
    }
    return suc;
}

static void tuneTransform(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    int nnXLen,
    int nnYLen,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    const string& maybeFP16CompileOptions,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig
) {
    out << "------------------------------------------------------" << endl;
    out << "Tuning winograd transform for convolutions" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(conv3x3.transLocalSize0), { 1,2,4,8,16,32,64,128 });
        addConfigs(configs, SETTER(conv3x3.transLocalSize1), { 1,2,4,8,16,32,64 });
    }
    else {
        addConfigs(configs, SETTER(conv3x3.transLocalSize0), { 1,2,4,8,16,32,64,128 });
        addConfigs(configs, SETTER(conv3x3.transLocalSize1), { 1,2,4,8,16,32 });
    }

    filterConfigs(configs, ISVALID(conv3x3));
    shuffleConfigs(configs);
    configs.insert(configs.begin(), currentConfig);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.conv3x3.transLocalSize0 = untunedConfig.conv3x3.transLocalSize0;
    referenceConfig.conv3x3.transLocalSize1 = untunedConfig.conv3x3.transLocalSize1;

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.conv3x3.transDesc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "winogradConv3x3NCHWTransformProgram", context, deviceIdsToUse, OpenCLKernels::winogradTransformNCHW,
            cfg.conv3x3.compileOptions() + maybeFP16CompileOptions,
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "transform", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int convSize = 3;
        int numTilesX = (nnXLen + cfg.conv3x3.OUTTILE_XSIZE - 1) / cfg.conv3x3.OUTTILE_XSIZE;
        int numTilesY = (nnYLen + cfg.conv3x3.OUTTILE_YSIZE - 1) / cfg.conv3x3.OUTTILE_YSIZE;
        int numTilesTotal = batchSize * numTilesX * numTilesY;

        int inTileXSize = cfg.conv3x3.INTILE_XSIZE;
        int inTileYSize = cfg.conv3x3.INTILE_YSIZE;

        int maxChannels = modelInfo.maxConvChannels3x3;
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);

        int mPaddingMult = cfg.getXGemmMPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);
        //int nPaddingMult = cfg.getXGemmNPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);
        int kPaddingMult = cfg.getXGemmKPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);

        int inputNumFloats = batchSize * nnXLen * nnYLen * maxChannels;
        int outputNumFloats = roundUpToMultiple(numTilesTotal, mPaddingMult) * roundUpToMultiple(maxChannels, kPaddingMult) * inTileXSize * inTileYSize;

        cl_mem input;
        cl_mem output;
        if (cfg.shouldUseFP16Storage) {
            input = randomReadOnlyBufferHalf(18303053403275884080ULL/*tune3x3TransInput*/, context, inputNumFloats, 1.0);
            output = createReadWriteBufferHalf(context, outputNumFloats);
        }
        else {
            input = randomReadOnlyBufferFloat(18303053403275884080ULL/*tune3x3TransInput*/, context, inputNumFloats, 1.0);
            output = createReadWriteBufferFloat(context, outputNumFloats);
        }

        const int reps = 7;
        for (int i = 0; i < reps; i++) {
            int inChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: inChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: inChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: inChannels = FEATURES1_NUM; weight = 0.02; break;
            case 3: inChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 4: inChannels = FEATURES1_NUM; weight = 0.02; break;
            case 5: inChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 6: inChannels = FEATURES1_NUM; weight = 0.02; break;
            }

            cl_event event;
            err = doWinogradTransform(
                kernel,
                commandQueue,
                cfg,
                input, output,
                nnXLen, nnYLen,
                batchSize, numTilesX, numTilesY, mPaddingMult,
                inChannels, kPaddingMult,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(outputNumFloats, 0.0);
        else if (cfg.shouldUseFP16Storage)
            blockingReadBufferHalfToFloat(commandQueue, output, outputNumFloats, ret);
        else
            blockingReadBuffer(commandQueue, output, outputNumFloats, ret);

        clReleaseMemObject(input);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = false;
    double bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.05;
    testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );

    tunedConfig = currentConfig;
}

static void tuneUntransform(
    OpenCLTuneParams currentConfig,
    const OpenCLTuneParams& untunedConfig,
    const cl_context& context,
    cl_command_queue& commandQueue,
    const vector<cl_device_id>& deviceIdsToUse,
    int batchSize,
    int nnXLen,
    int nnYLen,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    ostream& out,
    const string& maybeFP16CompileOptions,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig
) {
    out << "------------------------------------------------------" << endl;
    out << "Tuning winograd untransform for convolutions" << endl;

    vector<OpenCLTuneParams> configs;
    configs.push_back(currentConfig);
    if (full) {
        addConfigs(configs, SETTER(conv3x3.untransLocalSize0), { 1,2,4,8,16,32,64 });
        addConfigs(configs, SETTER(conv3x3.untransLocalSize1), { 1,2,4,8,16,32,64 });
        addConfigs(configs, SETTER(conv3x3.untransLocalSize2), { 1,2,4,8,16,32 });
    }
    else {
        addConfigs(configs, SETTER(conv3x3.untransLocalSize0), { 1,2,8,16,32 });
        addConfigs(configs, SETTER(conv3x3.untransLocalSize1), { 1,2,4,16,32 });
        addConfigs(configs, SETTER(conv3x3.untransLocalSize2), { 1,2,4,8,16 });
    }

    filterConfigs(configs, ISVALID(conv3x3));
    shuffleConfigs(configs);
    configs.insert(configs.begin(), currentConfig);

    OpenCLTuneParams referenceConfig = currentConfig;
    referenceConfig.conv3x3.untransLocalSize0 = untunedConfig.conv3x3.untransLocalSize0;
    referenceConfig.conv3x3.untransLocalSize1 = untunedConfig.conv3x3.untransLocalSize1;
    referenceConfig.conv3x3.untransLocalSize2 = untunedConfig.conv3x3.untransLocalSize2;

    auto getDesc = [](const OpenCLTuneParams& cfg) { return cfg.conv3x3.untransDesc(); };

    auto test = [&](const OpenCLTuneParams& cfg, vector<float>& ret) {
        OpenCLTuneAccums accums;

        cl_int err;
        cl_program program;
        string compileError;
        bool compileSuc = tryCompileProgram(
            "winogradConv3x3NCHWUntransformProgram", context, deviceIdsToUse, OpenCLKernels::winogradUntransformNCHW,
            cfg.conv3x3.compileOptions() + maybeFP16CompileOptions,
            program, compileError
        );
        if (!compileSuc) { accums.bad = true; accums.detailedErrorMessage = compileError; accums.badErr = CL_BUILD_PROGRAM_FAILURE; return accums; }
        cl_kernel kernel = clCreateKernel(program, "untransform", &err);
        if (err != 0) { accums.bad = true; accums.badErr = err; return accums; }

        int convSize = 3;
        int numTilesX = (nnXLen + cfg.conv3x3.OUTTILE_XSIZE - 1) / cfg.conv3x3.OUTTILE_XSIZE;
        int numTilesY = (nnYLen + cfg.conv3x3.OUTTILE_YSIZE - 1) / cfg.conv3x3.OUTTILE_YSIZE;
        int numTilesTotal = batchSize * numTilesX * numTilesY;

        int inTileXSize = cfg.conv3x3.INTILE_XSIZE;
        int inTileYSize = cfg.conv3x3.INTILE_YSIZE;

        int maxChannels = FEATURES1_NUM;
        maxChannels = std::max(modelInfo.trunkNumChannels, maxChannels);

        int mPaddingMult = cfg.getXGemmMPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);
        int nPaddingMult = cfg.getXGemmNPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);
        //int kPaddingMult = cfg.getXGemmKPaddingMult(cfg.shouldUseFP16Compute, cfg.shouldUseFP16TensorCores);

        int inputNumFloats = roundUpToMultiple(numTilesTotal, mPaddingMult) * roundUpToMultiple(maxChannels, nPaddingMult) * inTileXSize * inTileYSize;
        int outputNumFloats = batchSize * nnXLen * nnYLen * maxChannels;

        cl_mem input;
        cl_mem output;
        if (cfg.shouldUseFP16Storage) {
            input = randomReadOnlyBufferHalf(9094268440142369664ULL/*tune3x3UntransInput*/, context, inputNumFloats, 1.0);
            output = createReadWriteBufferHalf(context, outputNumFloats);
        }
        else {
            input = randomReadOnlyBufferFloat(9094268440142369664ULL/*tune3x3UntransInput*/, context, inputNumFloats, 1.0);
            output = createReadWriteBufferFloat(context, outputNumFloats);
        }

        const int reps = 7;
        for (int i = 0; i < reps; i++) {
            int outChannels;
            double weight;
            switch (i) {
                //Weight 0 on first kernel call to warm up
            case 0: outChannels = modelInfo.trunkNumChannels; weight = 0; break;
            case 1: outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 2: outChannels = FEATURES1_NUM; weight = 0.02; break;
            case 3: outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 4: outChannels = FEATURES1_NUM; weight = 0.02; break;
            case 5: outChannels = modelInfo.trunkNumChannels; weight = 1; break;
            case 6: outChannels = FEATURES1_NUM; weight = 0.02; break;
            }

            cl_event event;
            err = doWinogradUntransform(
                kernel,
                commandQueue,
                cfg,
                input, output,
                nnXLen, nnYLen,
                batchSize, numTilesX, numTilesY, mPaddingMult,
                outChannels, nPaddingMult,
                &event
            );

            accums.countResultAndFreeEvent(err, event, weight);
            if (accums.bad)
                break;
        }

        if (accums.bad)
            ret.assign(outputNumFloats, 0.0);
        else if (cfg.shouldUseFP16Storage)
            blockingReadBufferHalfToFloat(commandQueue, output, outputNumFloats, ret);
        else
            blockingReadBuffer(commandQueue, output, outputNumFloats, ret);

        clReleaseMemObject(input);
        clReleaseMemObject(output);

        clReleaseKernel(kernel);
        clReleaseProgram(program);

        return accums;
    };

    bool stopOnReferenceImplFail = false;
    double bestKernelsPerSecond = 0.0;
    double errorToleranceScale = 0.05;
    testAllConfigs(
        stopOnReferenceImplFail,
        configs,
        currentConfig,
        referenceConfig,
        out,
        verboseErrors,
        verboseTuner,
        errorToleranceScale,
        std::function<string(const OpenCLTuneParams& cfg)>(getDesc),
        std::function<OpenCLTuneAccums(const OpenCLTuneParams& cfg, vector<float>& ret)>(test),
        bestKernelsPerSecond
    );

    tunedConfig = currentConfig;
}

void OpenCLTuner::tune(
    const OpenCLTuneParams& initialConfig,
    DevicesContext& devicesContext,
    int gpuIdx,
    int batchSize,
    enabled_t testFP16Mode,
    enabled_t testFP16StorageMode,
    enabled_t testFP16ComputeMode,
    enabled_t testFP16TensorCoresMode,
    OpenCLTuner::ModelInfoForTuning modelInfo,
    bool full,
    int winograd3x3TileSize,
    ostream& out,
    bool verboseErrors,
    bool verboseTuner,
    OpenCLTuneParams& tunedConfig
) {
    const InitializedDevice* device = devicesContext.findGpuExn(gpuIdx);
    const cl_context& context = device->context;
    cl_command_queue commandQueue = device->commandQueue;
    const vector<cl_device_id>& deviceIdsToUse = { device->info.deviceId };

    out << "Beginning GPU tuning for " << device->info.name << " channels " << modelInfo.trunkNumChannels << endl;

    OpenCLTuneParams untunedConfig = OpenCLTuneParams();
    OpenCLTuneParams currentConfig = initialConfig;

    if (winograd3x3TileSize == 2) {
        out << "Setting winograd3x3TileSize = 2" << endl;
        untunedConfig.conv3x3.INTILE_XSIZE = 4;
        untunedConfig.conv3x3.INTILE_YSIZE = 4;
        untunedConfig.conv3x3.OUTTILE_XSIZE = 2;
        untunedConfig.conv3x3.OUTTILE_YSIZE = 2;
        currentConfig.conv3x3.INTILE_XSIZE = 4;
        currentConfig.conv3x3.INTILE_YSIZE = 4;
        currentConfig.conv3x3.OUTTILE_XSIZE = 2;
        currentConfig.conv3x3.OUTTILE_YSIZE = 2;
    }
    else if (winograd3x3TileSize == 4) {
        out << "Setting winograd3x3TileSize = 4" << endl;
        untunedConfig.conv3x3.INTILE_XSIZE = 6;
        untunedConfig.conv3x3.INTILE_YSIZE = 6;
        untunedConfig.conv3x3.OUTTILE_XSIZE = 4;
        untunedConfig.conv3x3.OUTTILE_YSIZE = 4;
        currentConfig.conv3x3.INTILE_XSIZE = 6;
        currentConfig.conv3x3.INTILE_YSIZE = 6;
        currentConfig.conv3x3.OUTTILE_XSIZE = 4;
        currentConfig.conv3x3.OUTTILE_YSIZE = 4;
    }

    if (!currentConfig.isValid()) {
        out << "Loaded a config but the config was invalid, starting tuning from basic config" << endl;
        currentConfig = untunedConfig;
    }

    {
        OpenCLTuneParams result;
        tuneXGemmDirect(
            currentConfig,
            untunedConfig,
            context,
            commandQueue,
            deviceIdsToUse,
            batchSize,
            modelInfo,
            full,
            out,
            verboseErrors,
            verboseTuner,
            result
        );
        currentConfig = result;
    }

    {
        OpenCLTuneParams result;
        bool useFP16Storage = false;
        double bestKernelsPerSecond = 0.0;
        tuneXGemm(
            currentConfig,
            untunedConfig,
            context,
            commandQueue,
            deviceIdsToUse,
            batchSize,
            modelInfo,
            full,
            out,
            useFP16Storage,
            verboseErrors,
            verboseTuner,
            result,
            bestKernelsPerSecond
        );
        currentConfig = result;

        //Start with having nothing enabled by default
        currentConfig.shouldUseFP16Storage = false;
        currentConfig.shouldUseFP16Compute = false;
        currentConfig.shouldUseFP16TensorCores = false;
        //Initialize xGemm16 config to the best non-fp16 config, by default
        currentConfig.xGemm16 = currentConfig.xGemm;

        bool shouldTestFP16 = testFP16Mode != enabled_t::False;
        //Try FP16 if allowed
        if (!shouldTestFP16) {
            out << "Not enabling FP16 for anything since FP16 disabled" << endl;
        }
        else {
            const double bestKernelsPerSecondFP32Only = bestKernelsPerSecond;

            //Since FP16 loses precision, require that it be faster by at least this much to use it
            static constexpr double FP16_REQUIRED_SPEEDUP = 1.2;
            bool foundGoodFP16 = false;

            bool shouldTestFP16TensorCores = testFP16TensorCoresMode == enabled_t::True || (testFP16TensorCoresMode == enabled_t::Auto && !foundGoodFP16);
            if (shouldTestFP16TensorCores) {
                OpenCLTuneParams result16;
                double bestKernelsPerSecond16 = 0.0;
                bool suc = tuneHGemmWmma(
                    currentConfig,
                    untunedConfig,
                    context,
                    commandQueue,
                    deviceIdsToUse,
                    batchSize,
                    modelInfo,
                    full,
                    out,
                    verboseErrors,
                    verboseTuner,
                    result16,
                    bestKernelsPerSecond16
                );
                if (!suc) {
                    out << "FP16 tensor core tuning failed, assuming no FP16 tensor core support" << endl;
                }
                else if (bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP < bestKernelsPerSecond) {
                    currentConfig = result16;
                    out << "FP16 tensor cores not significantly faster, not enabling" << endl;
                }
                else {
                    currentConfig = result16;
                    currentConfig.shouldUseFP16Storage = true;
                    currentConfig.shouldUseFP16TensorCores = true;
                    bestKernelsPerSecond = bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP;
                    foundGoodFP16 = true;
                    out << "Enabling FP16 tensor cores due to better performance" << endl;
                }
            }

            bool shouldTestFP16Compute = testFP16ComputeMode == enabled_t::True || (testFP16ComputeMode == enabled_t::Auto && device->info.supportsFP16Compute);
            if (shouldTestFP16Compute) {
                OpenCLTuneParams result16;
                double bestKernelsPerSecond16 = 0.0;
                bool suc = tuneXGemm16(
                    currentConfig,
                    untunedConfig,
                    context,
                    commandQueue,
                    deviceIdsToUse,
                    batchSize,
                    modelInfo,
                    full,
                    out,
                    verboseErrors,
                    verboseTuner,
                    result16,
                    bestKernelsPerSecond16
                );

                if (!suc) {
                    out << "FP16 compute tuning failed, assuming no FP16 compute support" << endl;
                    currentConfig.xGemm16 = currentConfig.xGemm;
                }
                else if (bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP < bestKernelsPerSecondFP32Only) {
                    currentConfig = result16;
                    out << "FP16 compute not significantly faster, not enabling" << endl;
                }
                else if (bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP < bestKernelsPerSecond) {
                    currentConfig = result16;
                    currentConfig.shouldUseFP16Compute = true;
                    out << "FP16 compute not significantly faster than tensor cores, using it generally but using tensor cores for convs" << endl;
                }
                else {
                    currentConfig = result16;
                    currentConfig.shouldUseFP16Storage = true;
                    currentConfig.shouldUseFP16Compute = true;
                    currentConfig.shouldUseFP16TensorCores = false;
                    bestKernelsPerSecond = bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP;
                    foundGoodFP16 = true;
                    out << "Enabling FP16 compute due to better performance" << endl;
                }
            }

            bool shouldTestFP16Storage = testFP16StorageMode == enabled_t::True || (testFP16StorageMode == enabled_t::Auto && !foundGoodFP16);
            if (shouldTestFP16Storage) {
                OpenCLTuneParams result16;
                bool useFP16Storage16 = true;
                double bestKernelsPerSecond16 = 0.0;
                bool suc = tuneXGemm(
                    currentConfig,
                    untunedConfig,
                    context,
                    commandQueue,
                    deviceIdsToUse,
                    batchSize,
                    modelInfo,
                    full,
                    out,
                    useFP16Storage16,
                    verboseErrors,
                    verboseTuner,
                    result16,
                    bestKernelsPerSecond16
                );

                if (!suc) {
                    out << "FP16 storage tuning failed, assuming no FP16 storage support" << endl;
                }
                else if (bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP < bestKernelsPerSecond) {
                    out << "FP16 storage not significantly faster, not enabling on its own" << endl;
                }
                else {
                    currentConfig = result16;
                    currentConfig.shouldUseFP16Storage = true;
                    currentConfig.shouldUseFP16Compute = false;
                    currentConfig.shouldUseFP16TensorCores = false;
                    bestKernelsPerSecond = bestKernelsPerSecond16 / FP16_REQUIRED_SPEEDUP;
                    foundGoodFP16 = true;
                    out << "Enabling FP16 storage due to better performance" << endl;
                }
            }
        }
    }

    out << "------------------------------------------------------" << endl;
    string maybeFP16CompileOptions;
    if (currentConfig.shouldUseFP16Storage) {
        out << "Using FP16 storage!" << endl;
        maybeFP16CompileOptions += OpenCLKernels::fp16StorageDefine;
    }
    else {
        out << "Using FP32 storage!" << endl;
    }
    if (currentConfig.shouldUseFP16Compute) {
        out << "Using FP16 compute!" << endl;
        maybeFP16CompileOptions += OpenCLKernels::fp16ComputeDefine;
    }
    else {
        out << "Using FP32 compute!" << endl;
    }
    if (currentConfig.shouldUseFP16TensorCores) {
        out << "Using FP16 tensor cores!" << endl;
    }


    {
        OpenCLTuneParams result;
        tuneTransform(
            currentConfig,
            untunedConfig,
            context,
            commandQueue,
            deviceIdsToUse,
            batchSize,
            nnXLen,
            nnYLen,
            modelInfo,
            full,
            out,
            maybeFP16CompileOptions,
            verboseErrors,
            verboseTuner,
            result
        );
        currentConfig = result;
    }

    {
        OpenCLTuneParams result;
        tuneUntransform(
            currentConfig,
            untunedConfig,
            context,
            commandQueue,
            deviceIdsToUse,
            batchSize,
            nnXLen,
            nnYLen,
            modelInfo,
            full,
            out,
            maybeFP16CompileOptions,
            verboseErrors,
            verboseTuner,
            result
        );
        currentConfig = result;
    }

    out << "Done tuning" << endl;
    out << "------------------------------------------------------" << endl;
    tunedConfig = currentConfig;
}
