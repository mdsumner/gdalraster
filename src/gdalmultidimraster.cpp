/* GDAL Multidimensional Raster Interface for R
   Implementation file for C++ classes wrapping GDAL Multidim Raster API
   Part of gdalraster package: https://github.com/firelab/gdalraster
*/

#include "gdalmultidimraster.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

// ============================================================================
// Helper functions
// ============================================================================

std::string gdalTypeToRType(GDALDataType eType) {
    switch (eType) {
        case GDT_Byte:
        case GDT_Int8:
        case GDT_Int16:
        case GDT_UInt16:
        case GDT_Int32:
        case GDT_UInt32:
            return "integer";
        case GDT_Float32:
        case GDT_Float64:
        case GDT_Int64:
        case GDT_UInt64:
            return "double";
        case GDT_CInt16:
        case GDT_CInt32:
        case GDT_CFloat32:
        case GDT_CFloat64:
            return "complex";
        default:
            return "raw";
    }
}

GDALRIOResampleAlg stringToResampleAlg(const std::string& alg) {
    std::string algLower = alg;
    std::transform(algLower.begin(), algLower.end(), algLower.begin(), ::tolower);
    
    if (algLower == "nearest" || algLower == "nearestneighbour")
        return GRIORA_NearestNeighbour;
    if (algLower == "bilinear")
        return GRIORA_Bilinear;
    if (algLower == "cubic")
        return GRIORA_Cubic;
    if (algLower == "cubicspline")
        return GRIORA_CubicSpline;
    if (algLower == "lanczos")
        return GRIORA_Lanczos;
    if (algLower == "average")
        return GRIORA_Average;
    if (algLower == "rms")
        return GRIORA_RMS;
    if (algLower == "mode")
        return GRIORA_Mode;
    if (algLower == "gauss")
        return GRIORA_Gauss;
    
    return GRIORA_NearestNeighbour;  // Default
}

char** charVecToCSL(Rcpp::CharacterVector cv) {
    if (cv.size() == 0)
        return nullptr;
    
    char** papszList = nullptr;
    for (R_xlen_t i = 0; i < cv.size(); ++i) {
        papszList = CSLAddString(papszList, 
                                  Rcpp::as<std::string>(cv[i]).c_str());
    }
    return papszList;
}

Rcpp::CharacterVector getMultiDimDrivers() {
    Rcpp::CharacterVector result;
    
    int nDriverCount = GDALGetDriverCount();
    for (int i = 0; i < nDriverCount; ++i) {
        GDALDriverH hDriver = GDALGetDriver(i);
        if (hDriver != nullptr) {
            const char* pszCap = GDALGetMetadataItem(hDriver,
                                                      GDAL_DCAP_MULTIDIM_RASTER,
                                                      nullptr);
            if (pszCap != nullptr && EQUAL(pszCap, "YES")) {
                result.push_back(GDALGetDriverShortName(hDriver));
            }
        }
    }
    return result;
}

// ============================================================================
// GDALExtendedDataTypeR implementation
// ============================================================================

int GDALExtendedDataTypeR::getClass() const {
  return static_cast<int>(m_oType.GetClass());
}

GDALExtendedDataTypeR::GDALExtendedDataTypeR()
    : m_oType(GDALExtendedDataType::Create(GDT_Unknown)) {
}

GDALExtendedDataTypeR::GDALExtendedDataTypeR(GDALDataType eType)
    : m_oType(GDALExtendedDataType::Create(eType)) {
}

GDALExtendedDataTypeR::GDALExtendedDataTypeR(const GDALExtendedDataType& oType)
    : m_oType(oType) {
}

GDALExtendedDataTypeR::GDALExtendedDataTypeR(const GDALExtendedDataTypeR& other)
    : m_oType(other.m_oType) {
}

GDALExtendedDataTypeR::~GDALExtendedDataTypeR() {
}

GDALExtendedDataTypeR GDALExtendedDataTypeR::Create(GDALDataType eType) {
    return GDALExtendedDataTypeR(GDALExtendedDataType::Create(eType));
}

GDALExtendedDataTypeR GDALExtendedDataTypeR::CreateString(size_t nMaxStringLength) {
    return GDALExtendedDataTypeR(
        GDALExtendedDataType::CreateString(nMaxStringLength));
}

GDALExtendedDataTypeR GDALExtendedDataTypeR::CreateCompound(
        const std::string& osName,
        size_t nTotalSize,
        Rcpp::List components) {
    
    std::vector<std::unique_ptr<GDALEDTComponent>> aoComponents;
    
    for (R_xlen_t i = 0; i < components.size(); ++i) {
        Rcpp::List comp = components[i];
        if (comp.size() < 3) {
            Rcpp::stop("Each compound component must have name, offset, and type");
        }
        
        std::string compName = Rcpp::as<std::string>(comp["name"]);
        size_t offset = Rcpp::as<size_t>(comp["offset"]);
        
        // Type can be passed as GDALExtendedDataTypeR or as a GDALDataType integer
        SEXP typeObj = comp["type"];
        GDALExtendedDataType compType = GDALExtendedDataType::Create(GDT_Unknown);
        
        if (Rf_isInteger(typeObj) || Rf_isNumeric(typeObj)) {
            int typeInt = Rcpp::as<int>(typeObj);
            compType = GDALExtendedDataType::Create(static_cast<GDALDataType>(typeInt));
        } else {
            // Assume it's a GDALExtendedDataTypeR
            GDALExtendedDataTypeR* pType = unwrapModulePtr<GDALExtendedDataTypeR>(typeObj);
            compType = pType->getRef();
        }
        
        aoComponents.push_back(
            std::make_unique<GDALEDTComponent>(compName, offset, compType));
    }
    
    return GDALExtendedDataTypeR(
        GDALExtendedDataType::Create(osName, nTotalSize, std::move(aoComponents)));
}

std::string GDALExtendedDataTypeR::getName() const {
    return m_oType.GetName();
}

int GDALExtendedDataTypeR::getNumericDataType() const {
  return static_cast<int>(m_oType.GetNumericDataType());
}

std::string GDALExtendedDataTypeR::getClassAsString() const {
    switch (m_oType.GetClass()) {
        case GEDTC_NUMERIC:
            return "NUMERIC";
        case GEDTC_STRING:
            return "STRING";
        case GEDTC_COMPOUND:
            return "COMPOUND";
        default:
            return "UNKNOWN";
    }
}



std::string GDALExtendedDataTypeR::getNumericDataTypeAsString() const {
    return GDALGetDataTypeName(m_oType.GetNumericDataType());
}

size_t GDALExtendedDataTypeR::getSize() const {
    return m_oType.GetSize();
}

size_t GDALExtendedDataTypeR::getMaxStringLength() const {
    return m_oType.GetMaxStringLength();
}

Rcpp::List GDALExtendedDataTypeR::getComponents() const {
    Rcpp::List result;
    
    const auto& components = m_oType.GetComponents();
    for (const auto& comp : components) {
        Rcpp::List compInfo;
        compInfo["name"] = comp->GetName();
        compInfo["offset"] = comp->GetOffset();
        compInfo["type"] = GDALExtendedDataTypeR(comp->GetType());
        result.push_back(compInfo);
    }
    
    return result;
}

bool GDALExtendedDataTypeR::canConvertTo(const SEXP other) const {
  GDALExtendedDataTypeR* pOther = unwrapModulePtr<GDALExtendedDataTypeR>(other);
    return m_oType.CanConvertTo(pOther->m_oType);
}

bool GDALExtendedDataTypeR::equals(const SEXP other) const {
  GDALExtendedDataTypeR* pOther = unwrapModulePtr<GDALExtendedDataTypeR>(other);
    return m_oType == pOther->m_oType;
}

// ============================================================================
// GDALDimensionR implementation
// ============================================================================

GDALDimensionR::GDALDimensionR()
    : m_poDim(nullptr) {
}

GDALDimensionR::GDALDimensionR(std::shared_ptr<GDALDimension> poDim)
    : m_poDim(poDim) {
}

GDALDimensionR::GDALDimensionR(const GDALDimensionR& other)
    : m_poDim(other.m_poDim) {
}

GDALDimensionR::~GDALDimensionR() {
}

std::string GDALDimensionR::getName() const {
    if (!isValid()) return "";
    return m_poDim->GetName();
}

std::string GDALDimensionR::getFullName() const {
    if (!isValid()) return "";
    return m_poDim->GetFullName();
}

std::string GDALDimensionR::getType() const {
    if (!isValid()) return "";
    return m_poDim->GetType();
}

std::string GDALDimensionR::getDirection() const {
    if (!isValid()) return "";
    return m_poDim->GetDirection();
}

GUInt64 GDALDimensionR::getSize() const {
    if (!isValid()) return 0;
    return m_poDim->GetSize();
}

GDALMDArrayR GDALDimensionR::getIndexingVariable() const {
    if (!isValid()) return GDALMDArrayR();
    
    auto poArray = m_poDim->GetIndexingVariable();
    if (!poArray) return GDALMDArrayR();
    
    return GDALMDArrayR(poArray);
}

bool GDALDimensionR::setIndexingVariable(SEXP poArrayR) {
    if (!isValid()) return false;
    
    if (Rf_isNull(poArrayR)) {
        return m_poDim->SetIndexingVariable(nullptr);
    }
    
    GDALMDArrayR* pArray = unwrapModulePtr<GDALMDArrayR>(poArrayR);
    return m_poDim->SetIndexingVariable(pArray->getSharedPtr());
}

bool GDALDimensionR::rename(const std::string& osNewName) {
    if (!isValid()) return false;
    return m_poDim->Rename(osNewName);
}

// ============================================================================
// GDALAttributeR implementation
// ============================================================================

GDALAttributeR::GDALAttributeR()
    : m_poAttr(nullptr) {
}

GDALAttributeR::GDALAttributeR(std::shared_ptr<GDALAttribute> poAttr)
    : m_poAttr(poAttr) {
}

GDALAttributeR::GDALAttributeR(const GDALAttributeR& other)
    : m_poAttr(other.m_poAttr) {
}

GDALAttributeR::~GDALAttributeR() {
}

std::string GDALAttributeR::getName() const {
    if (!isValid()) return "";
    return m_poAttr->GetName();
}

std::string GDALAttributeR::getFullName() const {
    if (!isValid()) return "";
    return m_poAttr->GetFullName();
}

GUInt64 GDALAttributeR::getTotalElementsCount() const {
    if (!isValid()) return 0;
    return m_poAttr->GetTotalElementsCount();
}

Rcpp::NumericVector GDALAttributeR::getDimensionCount() const {
    if (!isValid()) return Rcpp::NumericVector();
    return Rcpp::NumericVector::create(m_poAttr->GetDimensionCount());
}

std::vector<GUInt64> GDALAttributeR::getDimensionsSize() const {
    if (!isValid()) return std::vector<GUInt64>();
    return m_poAttr->GetDimensionsSize();
}

GDALExtendedDataTypeR GDALAttributeR::getDataType() const {
  if (!isValid()) return GDALExtendedDataTypeR();
  return GDALExtendedDataTypeR(m_poAttr->GetDataType());
}

Rcpp::RawVector GDALAttributeR::readAsRaw() const {
    if (!isValid()) return Rcpp::RawVector();
    
    GDALRawResult rawResult = m_poAttr->ReadAsRaw();
    Rcpp::RawVector result(rawResult.size());
    std::memcpy(result.begin(), rawResult.data(), rawResult.size());
    return result;
}

std::string GDALAttributeR::readAsString() const {
    if (!isValid()) return "";
    const char* str = m_poAttr->ReadAsString();
    return str ? str : "";
}

int GDALAttributeR::readAsInt() const {
    if (!isValid()) return NA_INTEGER;
    return m_poAttr->ReadAsInt();
}

double GDALAttributeR::readAsDouble() const {
    if (!isValid()) return NA_REAL;
    return m_poAttr->ReadAsDouble();
}

Rcpp::CharacterVector GDALAttributeR::readAsStringArray() const {
    if (!isValid()) return Rcpp::CharacterVector();
    
    CPLStringList strs = m_poAttr->ReadAsStringArray();
    Rcpp::CharacterVector result;
    for (int i = 0; i < strs.size(); ++i) {
        result.push_back(strs[i]);
    }
    return result;
}

Rcpp::IntegerVector GDALAttributeR::readAsIntArray() const {
    if (!isValid()) return Rcpp::IntegerVector();
    
    std::vector<int> data = m_poAttr->ReadAsIntArray();
    return Rcpp::wrap(data);
}

Rcpp::NumericVector GDALAttributeR::readAsDoubleArray() const {
    if (!isValid()) return Rcpp::NumericVector();
    
    std::vector<double> data = m_poAttr->ReadAsDoubleArray();
    return Rcpp::wrap(data);
}

bool GDALAttributeR::write(Rcpp::RawVector data) {
    if (!isValid()) return false;
    
    std::vector<GByte> vec(data.begin(), data.end());
    return m_poAttr->Write(vec.data(), vec.size());
}

bool GDALAttributeR::writeString(const std::string& val) {
    if (!isValid()) return false;
    return m_poAttr->Write(val.c_str());
}

bool GDALAttributeR::writeInt(int val) {
    if (!isValid()) return false;
    return m_poAttr->Write(val);
}

bool GDALAttributeR::writeDouble(double val) {
    if (!isValid()) return false;
    return m_poAttr->Write(val);
}

bool GDALAttributeR::writeStringArray(Rcpp::CharacterVector val) {
    if (!isValid()) return false;
    
    char** papszVals = charVecToCSL(val);
    bool bRet = m_poAttr->Write(papszVals, val.size());
    CSLDestroy(papszVals);
    return bRet;
}

bool GDALAttributeR::writeIntArray(Rcpp::IntegerVector val) {
    if (!isValid()) return false;
    
    std::vector<int> data(val.begin(), val.end());
    return m_poAttr->Write(data.data(), data.size());
}

bool GDALAttributeR::writeDoubleArray(Rcpp::NumericVector val) {
    if (!isValid()) return false;
    
    std::vector<double> data(val.begin(), val.end());
    return m_poAttr->Write(data.data(), data.size());
}

bool GDALAttributeR::rename(const std::string& osNewName) {
    if (!isValid()) return false;
    return m_poAttr->Rename(osNewName);
}

// ============================================================================
// GDALMDArrayR implementation
// ============================================================================

GDALMDArrayR::GDALMDArrayR()
    : m_poArray(nullptr) {
}

GDALMDArrayR::GDALMDArrayR(std::shared_ptr<GDALMDArray> poArray)
    : m_poArray(poArray) {
}

GDALMDArrayR::GDALMDArrayR(const GDALMDArrayR& other)
    : m_poArray(other.m_poArray) {
}

GDALMDArrayR::~GDALMDArrayR() {
}

// Helper methods for vector conversion
std::vector<GUInt64> GDALMDArrayR::rVecToGUInt64(Rcpp::NumericVector v) const {
    std::vector<GUInt64> result(v.size());
    for (R_xlen_t i = 0; i < v.size(); ++i) {
        result[i] = static_cast<GUInt64>(v[i]);
    }
    return result;
}

std::vector<size_t> GDALMDArrayR::rVecToSizeT(Rcpp::NumericVector v) const {
    std::vector<size_t> result(v.size());
    for (R_xlen_t i = 0; i < v.size(); ++i) {
        result[i] = static_cast<size_t>(v[i]);
    }
    return result;
}

std::vector<GInt64> GDALMDArrayR::rVecToGInt64(Rcpp::NumericVector v) const {
    std::vector<GInt64> result(v.size());
    for (R_xlen_t i = 0; i < v.size(); ++i) {
        result[i] = static_cast<GInt64>(v[i]);
    }
    return result;
}

std::vector<GPtrDiff_t> GDALMDArrayR::rVecToGPtrDiff(Rcpp::NumericVector v) const {
    std::vector<GPtrDiff_t> result(v.size());
    for (R_xlen_t i = 0; i < v.size(); ++i) {
        result[i] = static_cast<GPtrDiff_t>(v[i]);
    }
    return result;
}

std::string GDALMDArrayR::getName() const {
    if (!isValid()) return "";
    return m_poArray->GetName();
}

std::string GDALMDArrayR::getFullName() const {
    if (!isValid()) return "";
    return m_poArray->GetFullName();
}

GUInt64 GDALMDArrayR::getTotalElementsCount() const {
    if (!isValid()) return 0;
    return m_poArray->GetTotalElementsCount();
}

size_t GDALMDArrayR::getDimensionCount() const {
    if (!isValid()) return 0;
    return m_poArray->GetDimensionCount();
}

Rcpp::List GDALMDArrayR::getDimensions() const {
    Rcpp::List result;
    if (!isValid()) return result;
    
    const auto& dims = m_poArray->GetDimensions();
    for (const auto& dim : dims) {
        result.push_back(GDALDimensionR(dim));
    }
    return result;
}

GDALExtendedDataTypeR GDALMDArrayR::getDataType() const {
  if (!isValid()) return GDALExtendedDataTypeR();
  return GDALExtendedDataTypeR(m_poArray->GetDataType());
}

std::string GDALMDArrayR::getSpatialRef() const {
    if (!isValid()) return "";
    
    auto poSRS = m_poArray->GetSpatialRef();
    if (!poSRS) return "";
    
    char* pszWKT = nullptr;
    poSRS->exportToWkt(&pszWKT);
    std::string result = pszWKT ? pszWKT : "";
    CPLFree(pszWKT);
    return result;
}

std::string GDALMDArrayR::getUnit() const {
    if (!isValid()) return "";
    return m_poArray->GetUnit();
}

bool GDALMDArrayR::setUnit(const std::string& osUnit) {
    if (!isValid()) return false;
    return m_poArray->SetUnit(osUnit);
}

bool GDALMDArrayR::setSpatialRef(const std::string& osWKT) {
    if (!isValid()) return false;
    
    if (osWKT.empty()) {
        return m_poArray->SetSpatialRef(nullptr);
    }
    
    OGRSpatialReference oSRS;
    if (oSRS.importFromWkt(osWKT.c_str()) != OGRERR_NONE) {
        Rcpp::warning("Failed to parse WKT for spatial reference");
        return false;
    }
    return m_poArray->SetSpatialRef(&oSRS);
}

Rcpp::NumericVector GDALMDArrayR::getNoDataValueAsDouble() const {
    if (!isValid()) return Rcpp::NumericVector();
    
    bool bHasNoData = false;
    double noData = m_poArray->GetNoDataValueAsDouble(&bHasNoData);
    
    if (bHasNoData) {
        return Rcpp::NumericVector::create(noData);
    }
    return Rcpp::NumericVector();
}

Rcpp::RawVector GDALMDArrayR::getNoDataValueAsRaw() const {
    if (!isValid()) return Rcpp::RawVector();
    
    const void* pNoData = m_poArray->GetRawNoDataValue();
    if (!pNoData) return Rcpp::RawVector();
    
    size_t nSize = m_poArray->GetDataType().GetSize();
    Rcpp::RawVector result(nSize);
    std::memcpy(result.begin(), pNoData, nSize);
    return result;
}

bool GDALMDArrayR::setNoDataValue(double dfNoData) {
    if (!isValid()) return false;
    return m_poArray->SetNoDataValue(dfNoData);
}

bool GDALMDArrayR::setNoDataValueRaw(Rcpp::RawVector nodata) {
    if (!isValid()) return false;
    return m_poArray->SetRawNoDataValue(nodata.begin());
}

bool GDALMDArrayR::deleteNoDataValue() {
    if (!isValid()) return false;
    return m_poArray->SetRawNoDataValue(nullptr);
}

Rcpp::NumericVector GDALMDArrayR::getOffset() const {
    if (!isValid()) return Rcpp::NumericVector();
    
    bool bHasOffset = false;
    double offset = m_poArray->GetOffset(&bHasOffset);
    
    if (bHasOffset) {
        return Rcpp::NumericVector::create(offset);
    }
    return Rcpp::NumericVector();
}

Rcpp::NumericVector GDALMDArrayR::getScale() const {
    if (!isValid()) return Rcpp::NumericVector();
    
    bool bHasScale = false;
    double scale = m_poArray->GetScale(&bHasScale);
    
    if (bHasScale) {
        return Rcpp::NumericVector::create(scale);
    }
    return Rcpp::NumericVector();
}

bool GDALMDArrayR::setOffset(double dfOffset) {
    if (!isValid()) return false;
    return m_poArray->SetOffset(dfOffset);
}

bool GDALMDArrayR::setScale(double dfScale) {
    if (!isValid()) return false;
    return m_poArray->SetScale(dfScale);
}

Rcpp::NumericVector GDALMDArrayR::getBlockSize() const {
    if (!isValid()) return Rcpp::NumericVector();
    
    std::vector<GUInt64> blockSize = m_poArray->GetBlockSize();
    Rcpp::NumericVector result(blockSize.size());
    for (size_t i = 0; i < blockSize.size(); ++i) {
        result[i] = static_cast<double>(blockSize[i]);
    }
    return result;
}

Rcpp::List GDALMDArrayR::getProcessingChunkSize(size_t nMaxChunkMemory) const {
    if (!isValid()) return Rcpp::List();
    
    std::vector<size_t> chunkSize = m_poArray->GetProcessingChunkSize(nMaxChunkMemory);
    Rcpp::NumericVector result(chunkSize.size());
    for (size_t i = 0; i < chunkSize.size(); ++i) {
        result[i] = static_cast<double>(chunkSize[i]);
    }
    return Rcpp::List::create(Rcpp::Named("chunk_size") = result);
}

std::string GDALMDArrayR::getStructuralInfo() const {
    if (!isValid()) return "";
    
    CSLConstList papszInfo = m_poArray->GetStructuralInfo();
    if (!papszInfo) return "";
    
    std::string result;
    for (int i = 0; papszInfo[i] != nullptr; ++i) {
        if (i > 0) result += "\n";
        result += papszInfo[i];
    }
    return result;
}

Rcpp::CharacterVector GDALMDArrayR::getAttributeNames() const {
    if (!isValid()) return Rcpp::CharacterVector();
    
    auto attrs = m_poArray->GetAttributes();
    Rcpp::CharacterVector result;
    for (const auto& attr : attrs) {
        result.push_back(attr->GetName());
    }
    return result;
}

GDALAttributeR GDALMDArrayR::getAttribute(const std::string& osName) const {
    if (!isValid()) return GDALAttributeR();
    
    auto poAttr = m_poArray->GetAttribute(osName);
    if (!poAttr) return GDALAttributeR();
    
    return GDALAttributeR(poAttr);
}

Rcpp::List GDALMDArrayR::getAttributes() const {
    Rcpp::List result;
    if (!isValid()) return result;
    
    auto attrs = m_poArray->GetAttributes();
    for (const auto& attr : attrs) {
        result.push_back(GDALAttributeR(attr));
    }
    return result;
}

GDALAttributeR GDALMDArrayR::createAttribute(
        const std::string& osName,
        Rcpp::NumericVector dimensions,
        SEXP oType,
        Rcpp::CharacterVector options) {
    
    GDALExtendedDataTypeR* pOther = unwrapModulePtr<GDALExtendedDataTypeR>(oType);
    if (!isValid()) return GDALAttributeR();
    
    std::vector<GUInt64> anDimensions = rVecToGUInt64(dimensions);
    char** papszOptions = charVecToCSL(options);
    
    auto poAttr = m_poArray->CreateAttribute(
        osName, anDimensions, pOther->getRef(), papszOptions);
    
    CSLDestroy(papszOptions);
    
    if (!poAttr) return GDALAttributeR();
    return GDALAttributeR(poAttr);
}

bool GDALMDArrayR::deleteAttribute(const std::string& osName,
                                    Rcpp::CharacterVector options) {
    if (!isValid()) return false;
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poArray->DeleteAttribute(osName, papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

SEXP GDALMDArrayR::read(
        Rcpp::NumericVector arrayStartIdx,
        Rcpp::NumericVector count,
        Rcpp::NumericVector arrayStep,
        Rcpp::NumericVector bufferStride,
        SEXP bufferDataType) {
    
    GDALExtendedDataType oBufType = m_poArray->GetDataType();
  if (bufferDataType == R_NilValue) {
    //oBufType = m_poArray->GetDataType();  // or whatever the default should be
  } else {
    GDALExtendedDataTypeR* pType = unwrapModulePtr<GDALExtendedDataTypeR>(bufferDataType);
    oBufType = pType->getRef();
  }
  //GDALExtendedDataType oBufType = pBuffer->getRef();
    if (!isValid()) return R_NilValue;
    
    const size_t nDims = getDimensionCount();
    const auto& dims = m_poArray->GetDimensions();
    
    // Prepare arrayStartIdx - default to 0 for each dimension
    std::vector<GUInt64> anStart(nDims, 0);
    if (arrayStartIdx.size() > 0) {
        if (static_cast<size_t>(arrayStartIdx.size()) != nDims) {
            Rcpp::stop("arrayStartIdx must have length equal to dimension count");
        }
        anStart = rVecToGUInt64(arrayStartIdx);
    }
    
    // Prepare count - default to full dimension size
    std::vector<size_t> anCount(nDims);
    for (size_t i = 0; i < nDims; ++i) {
        anCount[i] = static_cast<size_t>(dims[i]->GetSize() - anStart[i]);
    }
    if (count.size() > 0) {
        if (static_cast<size_t>(count.size()) != nDims) {
            Rcpp::stop("count must have length equal to dimension count");
        }
        anCount = rVecToSizeT(count);
    }
    
    // Prepare arrayStep - default to 1
    std::vector<GInt64> anStep(nDims, 1);
    if (arrayStep.size() > 0) {
        if (static_cast<size_t>(arrayStep.size()) != nDims) {
            Rcpp::stop("arrayStep must have length equal to dimension count");
        }
        anStep = rVecToGInt64(arrayStep);
    }
    
    // Prepare bufferStride - nullptr for default (C-style row-major)
    std::vector<GPtrDiff_t> anBufferStride;
    GPtrDiff_t* panBufferStride = nullptr;
    if (bufferStride.size() > 0) {
        if (static_cast<size_t>(bufferStride.size()) != nDims) {
            Rcpp::stop("bufferStride must have length equal to dimension count");
        }
        anBufferStride = rVecToGPtrDiff(bufferStride);
        panBufferStride = anBufferStride.data();
    }
    
    // Determine buffer data type
    //GDALExtendedDataType oBufType = pBuffer->getRef();
    if (oBufType.GetClass() == GEDTC_NUMERIC && 
        oBufType.GetNumericDataType() == GDT_Unknown) {
        oBufType = m_poArray->GetDataType();
    }
    
    // Calculate total elements
    size_t nTotalElements = 1;
    for (size_t i = 0; i < nDims; ++i) {
        nTotalElements *= anCount[i];
    }
    
    // Allocate buffer and read
    if (oBufType.GetClass() == GEDTC_STRING) {
        // String array
        std::vector<char*> buffer(nTotalElements, nullptr);
        
        bool bOK = m_poArray->Read(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oBufType, buffer.data());
        
        if (!bOK) {
            Rcpp::stop("Read failed");
        }
        
        Rcpp::CharacterVector result(nTotalElements);
        for (size_t i = 0; i < nTotalElements; ++i) {
            if (buffer[i]) {
                result[i] = buffer[i];
                CPLFree(buffer[i]);
            } else {
                result[i] = NA_STRING;
            }
        }
        
        // Add dimension attributes for R array
        if (nDims > 1) {
            Rcpp::IntegerVector dimVec(nDims);
            for (size_t i = 0; i < nDims; ++i) {
                dimVec[nDims - 1 - i] = static_cast<int>(anCount[i]);
            }
            result.attr("dim") = dimVec;
        }
        return result;
        
    } else if (oBufType.GetClass() == GEDTC_NUMERIC) {
        // Numeric array
        GDALDataType eType = oBufType.GetNumericDataType();
        size_t nTypeSize = GDALGetDataTypeSizeBytes(eType);
        
        // For R, we convert to integer or double
        std::string rType = gdalTypeToRType(eType);
        
        if (rType == "integer") {
            Rcpp::IntegerVector result(nTotalElements);
            
            // Read into native type buffer first, then convert
            std::vector<GByte> buffer(nTotalElements * nTypeSize);
            
            bool bOK = m_poArray->Read(
                anStart.data(), anCount.data(), anStep.data(),
                panBufferStride, oBufType, buffer.data());
            
            if (!bOK) {
                Rcpp::stop("Read failed");
            }
            
            // Convert to R integer
            GDALExtendedDataType oIntType = GDALExtendedDataType::Create(GDT_Int32);
            GDALExtendedDataType::CopyValues(
                buffer.data(), oBufType, 1,
                result.begin(), oIntType, 1,
                nTotalElements);
            
            if (nDims > 1) {
                Rcpp::IntegerVector dimVec(nDims);
                for (size_t i = 0; i < nDims; ++i) {
                    dimVec[nDims - 1 - i] = static_cast<int>(anCount[i]);
                }
                result.attr("dim") = dimVec;
            }
            return result;
            
        } else {
            // Read as double (R's default numeric)
            Rcpp::NumericVector result(nTotalElements);
            
            GDALExtendedDataType oDblType = GDALExtendedDataType::Create(GDT_Float64);
            
            bool bOK = m_poArray->Read(
                anStart.data(), anCount.data(), anStep.data(),
                panBufferStride, oDblType, result.begin());
            
            if (!bOK) {
                Rcpp::stop("Read failed");
            }
            
            if (nDims > 1) {
                Rcpp::IntegerVector dimVec(nDims);
                for (size_t i = 0; i < nDims; ++i) {
                    dimVec[nDims - 1 - i] = static_cast<int>(anCount[i]);
                }
                result.attr("dim") = dimVec;
            }
            return result;
        }
        
    } else {
        // Compound type - return as raw bytes
        size_t nTypeSize = oBufType.GetSize();
        Rcpp::RawVector result(nTotalElements * nTypeSize);
        
        bool bOK = m_poArray->Read(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oBufType, result.begin());
        
        if (!bOK) {
            Rcpp::stop("Read failed");
        }
        
        result.attr("element_size") = static_cast<int>(nTypeSize);
        return result;
    }
}

bool GDALMDArrayR::write(
        SEXP data,
        Rcpp::NumericVector arrayStartIdx,
        Rcpp::NumericVector count,
        Rcpp::NumericVector arrayStep,
        Rcpp::NumericVector bufferStride,
        SEXP bufferDataType) {
    
    GDALExtendedDataTypeR* pBufferType = unwrapModulePtr<GDALExtendedDataTypeR>(bufferDataType);
  
    if (!isValid()) return false;
    
    const size_t nDims = getDimensionCount();
    const auto& dims = m_poArray->GetDimensions();
    
    // Prepare arrayStartIdx
    std::vector<GUInt64> anStart(nDims, 0);
    if (arrayStartIdx.size() > 0) {
        if (static_cast<size_t>(arrayStartIdx.size()) != nDims) {
            Rcpp::stop("arrayStartIdx must have length equal to dimension count");
        }
        anStart = rVecToGUInt64(arrayStartIdx);
    }
    
    // Prepare count - infer from data if not provided
    std::vector<size_t> anCount(nDims);
    for (size_t i = 0; i < nDims; ++i) {
        anCount[i] = static_cast<size_t>(dims[i]->GetSize() - anStart[i]);
    }
    if (count.size() > 0) {
        if (static_cast<size_t>(count.size()) != nDims) {
            Rcpp::stop("count must have length equal to dimension count");
        }
        anCount = rVecToSizeT(count);
    }
    
    // Prepare arrayStep
    std::vector<GInt64> anStep(nDims, 1);
    if (arrayStep.size() > 0) {
        if (static_cast<size_t>(arrayStep.size()) != nDims) {
            Rcpp::stop("arrayStep must have length equal to dimension count");
        }
        anStep = rVecToGInt64(arrayStep);
    }
    
    // Prepare bufferStride
    std::vector<GPtrDiff_t> anBufferStride;
    GPtrDiff_t* panBufferStride = nullptr;
    if (bufferStride.size() > 0) {
        if (static_cast<size_t>(bufferStride.size()) != nDims) {
            Rcpp::stop("bufferStride must have length equal to dimension count");
        }
        anBufferStride = rVecToGPtrDiff(bufferStride);
        panBufferStride = anBufferStride.data();
    }
    
    // Determine buffer data type
    GDALExtendedDataType oBufType = pBufferType->getRef();
    if (oBufType.GetClass() == GEDTC_NUMERIC && 
        oBufType.GetNumericDataType() == GDT_Unknown) {
        oBufType = m_poArray->GetDataType();
    }
    
    // Handle different R types
    if (Rf_isString(data)) {
        // Character vector
        Rcpp::CharacterVector charData(data);
        std::vector<const char*> buffer(charData.size());
        for (R_xlen_t i = 0; i < charData.size(); ++i) {
          buffer[i] = Rcpp::CharacterVector::is_na(charData[i]) ? nullptr : CHAR(charData[i]);
        }
        
        GDALExtendedDataType oStrType = GDALExtendedDataType::CreateString();
        return m_poArray->Write(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oStrType, buffer.data());
        
    } else if (Rf_isNumeric(data)) {
        // Numeric/double vector
        Rcpp::NumericVector numData(data);
        
        GDALExtendedDataType oDblType = GDALExtendedDataType::Create(GDT_Float64);
        return m_poArray->Write(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oDblType, numData.begin());
        
    } else if (Rf_isInteger(data)) {
        // Integer vector
        Rcpp::IntegerVector intData(data);
        
        GDALExtendedDataType oIntType = GDALExtendedDataType::Create(GDT_Int32);
        return m_poArray->Write(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oIntType, intData.begin());
        
    } else if (TYPEOF(data) == RAWSXP) {
        // Raw vector
        Rcpp::RawVector rawData(data);
        
        return m_poArray->Write(
            anStart.data(), anCount.data(), anStep.data(),
            panBufferStride, oBufType, rawData.begin());
    }
    
    Rcpp::stop("Unsupported data type for write");
    return false;
}

bool GDALMDArrayR::adviseRead(
        Rcpp::NumericVector arrayStartIdx,
        Rcpp::NumericVector count,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return false;
    
    const size_t nDims = getDimensionCount();
    const auto& dims = m_poArray->GetDimensions();
    
    std::vector<GUInt64> anStart(nDims, 0);
    if (arrayStartIdx.size() > 0) {
        anStart = rVecToGUInt64(arrayStartIdx);
    }
    
    std::vector<size_t> anCount(nDims);
    for (size_t i = 0; i < nDims; ++i) {
        anCount[i] = static_cast<size_t>(dims[i]->GetSize() - anStart[i]);
    }
    if (count.size() > 0) {
        anCount = rVecToSizeT(count);
    }
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poArray->AdviseRead(anStart.data(), anCount.data(), papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

GDALMDArrayR GDALMDArrayR::getView(const std::string& osViewExpr) const {
    if (!isValid()) return GDALMDArrayR();
    
    auto poView = m_poArray->GetView(osViewExpr);
    if (!poView) return GDALMDArrayR();
    
    return GDALMDArrayR(poView);
}

GDALMDArrayR GDALMDArrayR::transpose(Rcpp::IntegerVector anMapNewAxisToOldAxis) const {
    if (!isValid()) return GDALMDArrayR();
    
    std::vector<int> map(anMapNewAxisToOldAxis.begin(), anMapNewAxisToOldAxis.end());
    auto poTransposed = m_poArray->Transpose(map);
    if (!poTransposed) return GDALMDArrayR();
    
    return GDALMDArrayR(poTransposed);
}

GDALMDArrayR GDALMDArrayR::getUnscaled() const {
    if (!isValid()) return GDALMDArrayR();
    
    auto poUnscaled = m_poArray->GetUnscaled();
    if (!poUnscaled) return GDALMDArrayR();
    
    return GDALMDArrayR(poUnscaled);
}

GDALMDArrayR GDALMDArrayR::getMask(Rcpp::CharacterVector options) const {
    if (!isValid()) return GDALMDArrayR();
    
    char** papszOptions = charVecToCSL(options);
    auto poMask = m_poArray->GetMask(papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poMask) return GDALMDArrayR();
    return GDALMDArrayR(poMask);
}

GDALMDArrayR GDALMDArrayR::getResampled(
        Rcpp::List apoNewDims,
        const std::string& resampleAlg,
        const std::string& targetSRS,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return GDALMDArrayR();
    
    // Build dimension vector
    std::vector<std::shared_ptr<GDALDimension>> aoDims;
    for (R_xlen_t i = 0; i < apoNewDims.size(); ++i) {
        SEXP dimObj = apoNewDims[i];
        if (Rf_isNull(dimObj)) {
            aoDims.push_back(nullptr);
        } else {
            GDALDimensionR* pDim = unwrapModulePtr<GDALDimensionR>(dimObj);
            aoDims.push_back(pDim->getSharedPtr());
        }
    }
    
    // Parse resample algorithm
    GDALRIOResampleAlg eResample = stringToResampleAlg(resampleAlg);
    
    // Parse target SRS
    OGRSpatialReference* poTargetSRS = nullptr;
    OGRSpatialReference oSRS;
    if (!targetSRS.empty()) {
        if (oSRS.importFromWkt(targetSRS.c_str()) == OGRERR_NONE ||
            oSRS.SetFromUserInput(targetSRS.c_str()) == OGRERR_NONE) {
            poTargetSRS = &oSRS;
        }
    }
    
    char** papszOptions = charVecToCSL(options);
    auto poResampled = m_poArray->GetResampled(aoDims, eResample, poTargetSRS, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poResampled) return GDALMDArrayR();
    return GDALMDArrayR(poResampled);
}

SEXP GDALMDArrayR::asClassicDataset(
        size_t iXDim,
        size_t iYDim,
        SEXP poRootGroup,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return R_NilValue;
    
    std::shared_ptr<GDALGroup> poGroup;
    if (!Rf_isNull(poRootGroup)) {
        GDALGroupR* pGroup = unwrapModulePtr<GDALGroupR>(poRootGroup);
        poGroup = pGroup->getSharedPtr();
    }
    
    char** papszOptions = charVecToCSL(options);
    GDALDataset* poDS = m_poArray->AsClassicDataset(iXDim, iYDim, poGroup, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poDS) return R_NilValue;
    
    // Return the filename/description for use with GDALRaster
    // The dataset handle is managed by the array, so we just return info
    return Rcpp::wrap(poDS->GetDescription());
}

bool GDALMDArrayR::cache(Rcpp::CharacterVector options) const {
    if (!isValid()) return false;
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poArray->Cache(papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

Rcpp::List GDALMDArrayR::computeStatistics(
        bool approxOK,
        bool force,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return Rcpp::List();
    
    double dfMin = 0, dfMax = 0, dfMean = 0, dfStdDev = 0;
    GUInt64 nValidCount = 0;
    
    char** papszOptions = charVecToCSL(options);
    
    bool bRet = m_poArray->ComputeStatistics(
        approxOK,
        &dfMin, &dfMax, &dfMean, &dfStdDev,
        &nValidCount,
        nullptr, nullptr,  // Progress callback
        papszOptions);
    
    CSLDestroy(papszOptions);
    
    if (!bRet) {
        return Rcpp::List();
    }
    
    return Rcpp::List::create(
        Rcpp::Named("min") = dfMin,
        Rcpp::Named("max") = dfMax,
        Rcpp::Named("mean") = dfMean,
        Rcpp::Named("stddev") = dfStdDev,
        Rcpp::Named("valid_count") = static_cast<double>(nValidCount)
    );
}

bool GDALMDArrayR::resize(Rcpp::NumericVector newDimSizes,
                           Rcpp::CharacterVector options) {
    if (!isValid()) return false;
    
    std::vector<GUInt64> anNewSizes = rVecToGUInt64(newDimSizes);
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poArray->Resize(anNewSizes, papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

bool GDALMDArrayR::rename(const std::string& osNewName) {
    if (!isValid()) return false;
    return m_poArray->Rename(osNewName);
}

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)

// ---------------------------------------------------------------------------
// getRawBlockInfo - single chunk raw block info query
// ---------------------------------------------------------------------------
Rcpp::List GDALMDArrayR::getRawBlockInfo(Rcpp::IntegerVector blockIdx) const {
    if (!isValid())
        Rcpp::stop("GDALMDArrayR object is not valid");

    size_t ndims = m_poArray->GetDimensionCount();
    if (static_cast<size_t>(blockIdx.size()) != ndims) {
        Rcpp::stop("blockIdx length (%d) must match dimension count (%d)",
                    blockIdx.size(), static_cast<int>(ndims));
    }

    // Convert IntegerVector to uint64_t, checking for negative values
    std::vector<uint64_t> idx(ndims);
    for (size_t d = 0; d < ndims; d++) {
        if (blockIdx[d] < 0)
            Rcpp::stop("blockIdx[%d] = %d is negative",
                        static_cast<int>(d) + 1, blockIdx[d]);
        idx[d] = static_cast<uint64_t>(blockIdx[d]);
    }

    // Default constructor zero-initializes all members.
    // Do NOT memset — struct has non-trivial destructor/copy/move.
    GDALMDArrayRawBlockInfo info;
    bool ok = m_poArray->GetRawBlockInfo(idx.data(), info);
    if (!ok) {
        Rcpp::stop("GetRawBlockInfo failed for array '%s'",
                    m_poArray->GetName().c_str());
    }

    // Missing chunk: all fields zeroed but returns true
    if (info.nSize == 0 && info.pszFilename == nullptr) {
        return Rcpp::List::create(
            Rcpp::Named("filename") = NA_STRING,
            Rcpp::Named("offset") = NA_REAL,
            Rcpp::Named("size") = NA_REAL,
            Rcpp::Named("info") = Rcpp::CharacterVector::create(),
            Rcpp::Named("inline") = R_NilValue
        );
    }

    // Inline data: copy out before struct destructs
    Rcpp::RObject inline_data = R_NilValue;
    if (info.pabyInlineData != nullptr && info.nSize > 0) {
        Rcpp::RawVector raw(info.nSize);
        std::memcpy(raw.begin(), info.pabyInlineData, info.nSize);
        inline_data = raw;
    }

    // Convert CSL info strings to CharacterVector
    Rcpp::CharacterVector info_vec;
    if (info.papszInfo != nullptr) {
        for (int i = 0; info.papszInfo[i] != nullptr; i++) {
            info_vec.push_back(info.papszInfo[i]);
        }
    }

    return Rcpp::List::create(
        Rcpp::Named("filename") = std::string(
            info.pszFilename ? info.pszFilename : ""),
        Rcpp::Named("offset") = static_cast<double>(info.nOffset),
        Rcpp::Named("size") = static_cast<double>(info.nSize),
        Rcpp::Named("info") = info_vec,
        Rcpp::Named("inline") = inline_data
    );
    // info destructor frees owned memory here — no manual cleanup needed
}

// ---------------------------------------------------------------------------
// getRawBlockRefs - bulk scan all chunks, returns data.frame
// ---------------------------------------------------------------------------
Rcpp::DataFrame GDALMDArrayR::getRawBlockRefs() const {
    if (!isValid())
        Rcpp::stop("GDALMDArrayR object is not valid");

    auto dims = m_poArray->GetDimensions();
    auto block_size = m_poArray->GetBlockSize();
    size_t ndims = dims.size();

    if (ndims == 0) {
        Rcpp::stop("getRawBlockRefs requires a dimensioned array");
    }

    // Compute number of chunks per dimension
    std::vector<size_t> n_chunks(ndims);
    size_t total_chunks = 1;
    for (size_t d = 0; d < ndims; d++) {
        size_t dim_size = static_cast<size_t>(dims[d]->GetSize());
        size_t chunk_size = (block_size[d] > 0)
            ? static_cast<size_t>(block_size[d])
            : dim_size;
        n_chunks[d] = (dim_size + chunk_size - 1) / chunk_size;
        total_chunks *= n_chunks[d];
    }

    // Accumulate into vectors. Size not known ahead due to missing chunk
    // filtering, but reserve for the common case (most chunks present).
    std::vector<std::string> filenames;
    std::vector<double> offsets;
    std::vector<double> sizes;
    std::vector<std::string> infos;
    std::vector<std::vector<int>> chunk_indices(ndims);

    filenames.reserve(total_chunks);
    offsets.reserve(total_chunks);
    sizes.reserve(total_chunks);
    infos.reserve(total_chunks);
    for (size_t d = 0; d < ndims; d++) {
        chunk_indices[d].reserve(total_chunks);
    }

    std::vector<uint64_t> idx(ndims, 0);

    for (size_t i = 0; i < total_chunks; i++) {
        if (i % 1000 == 0) R_CheckUserInterrupt();

        // Construct a fresh struct each iteration. The default constructor
        // zero-initializes, and the destructor frees owned memory at end of
        // scope. This is safe regardless of whether the struct has a clear()
        // method, and the per-iteration overhead is trivial vs I/O.
        GDALMDArrayRawBlockInfo info;
        bool ok = m_poArray->GetRawBlockInfo(idx.data(), info);

        // Skip failed calls and missing chunks (all fields zeroed)
        if (ok && info.nSize > 0) {
            filenames.push_back(
                info.pszFilename ? info.pszFilename : "");
            offsets.push_back(static_cast<double>(info.nOffset));
            sizes.push_back(static_cast<double>(info.nSize));

            // Build pipe-separated info string
            std::string info_str;
            if (info.papszInfo != nullptr) {
                for (int j = 0; info.papszInfo[j] != nullptr; j++) {
                    if (j > 0) info_str += '|';
                    info_str += info.papszInfo[j];
                }
            }
            infos.push_back(std::move(info_str));

            for (size_t d = 0; d < ndims; d++) {
                chunk_indices[d].push_back(static_cast<int>(idx[d]));
            }
        }

        // Increment multi-dimensional index (last dimension fastest)
        for (int d = static_cast<int>(ndims) - 1; d >= 0; d--) {
            idx[d]++;
            if (idx[d] < n_chunks[d]) break;
            idx[d] = 0;
        }
    }

    // Build data.frame
    int n_present = static_cast<int>(filenames.size());
    if (n_present == 0) {
        // Return empty data.frame with correct column structure
        Rcpp::List df = Rcpp::List::create(
            Rcpp::Named("filename") = Rcpp::CharacterVector(0),
            Rcpp::Named("offset") = Rcpp::NumericVector(0),
            Rcpp::Named("size") = Rcpp::NumericVector(0),
            Rcpp::Named("info") = Rcpp::CharacterVector(0)
        );
        for (size_t d = 0; d < ndims; d++) {
            std::string col_name = "chunk_" + std::to_string(d);
            df.push_back(Rcpp::IntegerVector(0), col_name);
        }
        df.attr("class") = "data.frame";
        df.attr("row.names") = Rcpp::IntegerVector(0);
        return Rcpp::as<Rcpp::DataFrame>(df);
    }

    Rcpp::List df = Rcpp::List::create(
        Rcpp::Named("filename") = Rcpp::wrap(filenames),
        Rcpp::Named("offset") = Rcpp::wrap(offsets),
        Rcpp::Named("size") = Rcpp::wrap(sizes),
        Rcpp::Named("info") = Rcpp::wrap(infos)
    );
    for (size_t d = 0; d < ndims; d++) {
        std::string col_name = "chunk_" + std::to_string(d);
        df.push_back(Rcpp::IntegerVector(chunk_indices[d].begin(),
                                          chunk_indices[d].end()),
                     col_name);
    }
    df.attr("class") = "data.frame";
    df.attr("row.names") = Rcpp::seq(1, n_present);

    return Rcpp::as<Rcpp::DataFrame>(df);
}

#endif  // GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)

// ============================================================================
// GDALGroupR implementation
// ============================================================================

GDALGroupR::GDALGroupR()
    : m_poGroup(nullptr) {
}

GDALGroupR::GDALGroupR(std::shared_ptr<GDALGroup> poGroup)
    : m_poGroup(poGroup) {
}

GDALGroupR::GDALGroupR(const GDALGroupR& other)
    : m_poGroup(other.m_poGroup) {
}

GDALGroupR::~GDALGroupR() {
}

std::string GDALGroupR::getName() const {
    if (!isValid()) return "";
    return m_poGroup->GetName();
}

std::string GDALGroupR::getFullName() const {
    if (!isValid()) return "";
    return m_poGroup->GetFullName();
}

Rcpp::CharacterVector GDALGroupR::getGroupNames(
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return Rcpp::CharacterVector();
    
    char** papszOptions = charVecToCSL(options);
    std::vector<std::string> names = m_poGroup->GetGroupNames(papszOptions);
    CSLDestroy(papszOptions);
    
    return Rcpp::wrap(names);
}

GDALGroupR GDALGroupR::openGroup(
        const std::string& osName,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return GDALGroupR();
    
    char** papszOptions = charVecToCSL(options);
    auto poSubGroup = m_poGroup->OpenGroup(osName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poSubGroup) return GDALGroupR();
    return GDALGroupR(poSubGroup);
}

GDALGroupR GDALGroupR::openGroupFromFullname(
        const std::string& osFullName,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return GDALGroupR();
    
    char** papszOptions = charVecToCSL(options);
    auto poSubGroup = m_poGroup->OpenGroupFromFullname(osFullName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poSubGroup) return GDALGroupR();
    return GDALGroupR(poSubGroup);
}

GDALGroupR GDALGroupR::createGroup(
        const std::string& osName,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return GDALGroupR();
    
    char** papszOptions = charVecToCSL(options);
    auto poNewGroup = m_poGroup->CreateGroup(osName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poNewGroup) return GDALGroupR();
    return GDALGroupR(poNewGroup);
}

bool GDALGroupR::deleteGroup(
        const std::string& osName,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return false;
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poGroup->DeleteGroup(osName, papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

Rcpp::CharacterVector GDALGroupR::getMDArrayNames(
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return Rcpp::CharacterVector();
    
    char** papszOptions = charVecToCSL(options);
    std::vector<std::string> names = m_poGroup->GetMDArrayNames(papszOptions);
    CSLDestroy(papszOptions);
    
    return Rcpp::wrap(names);
}

GDALMDArrayR GDALGroupR::openMDArray(
        const std::string& osName,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return GDALMDArrayR();
    
    char** papszOptions = charVecToCSL(options);
    auto poArray = m_poGroup->OpenMDArray(osName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poArray) return GDALMDArrayR();
    return GDALMDArrayR(poArray);
}

GDALMDArrayR GDALGroupR::openMDArrayFromFullname(
        const std::string& osFullName,
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return GDALMDArrayR();
    
    char** papszOptions = charVecToCSL(options);
    auto poArray = m_poGroup->OpenMDArrayFromFullname(osFullName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poArray) return GDALMDArrayR();
    return GDALMDArrayR(poArray);
}

GDALMDArrayR GDALGroupR::createMDArray(
    const std::string& osName,
    Rcpp::List dimensions,
    SEXP oType,
    Rcpp::CharacterVector options) {
  
  if (!isValid()) return GDALMDArrayR();
  
  GDALExtendedDataTypeR* pType = unwrapModulePtr<GDALExtendedDataTypeR>(oType);
  
  // Build dimension vector
  std::vector<std::shared_ptr<GDALDimension>> aoDims;
  for (R_xlen_t i = 0; i < dimensions.size(); ++i) {
    GDALDimensionR* pDim = unwrapModulePtr<GDALDimensionR>(dimensions[i]);
    if (pDim->isValid()) {
      aoDims.push_back(pDim->getSharedPtr());
    }
  }
  
  char** papszOptions = charVecToCSL(options);
  auto poArray = m_poGroup->CreateMDArray(osName, aoDims, pType->getRef(), papszOptions);
  CSLDestroy(papszOptions);
  
  if (!poArray) return GDALMDArrayR();
  return GDALMDArrayR(poArray);
}

bool GDALGroupR::deleteMDArray(
        const std::string& osName,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return false;
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poGroup->DeleteMDArray(osName, papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

Rcpp::List GDALGroupR::getDimensions(
        Rcpp::CharacterVector options) const {
    
    Rcpp::List result;
    if (!isValid()) return result;
    
    char** papszOptions = charVecToCSL(options);
    auto dims = m_poGroup->GetDimensions(papszOptions);
    CSLDestroy(papszOptions);
    
    for (const auto& dim : dims) {
        result.push_back(GDALDimensionR(dim));
    }
    return result;
}

GDALDimensionR GDALGroupR::createDimension(
        const std::string& osName,
        const std::string& osType,
        const std::string& osDirection,
        GUInt64 nSize,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return GDALDimensionR();
    
    char** papszOptions = charVecToCSL(options);
    auto poDim = m_poGroup->CreateDimension(
        osName, osType, osDirection, nSize, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poDim) return GDALDimensionR();
    return GDALDimensionR(poDim);
}

Rcpp::CharacterVector GDALGroupR::getAttributeNames(
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return Rcpp::CharacterVector();
    
    char** papszOptions = charVecToCSL(options);
    auto attrs = m_poGroup->GetAttributes(papszOptions);
    CSLDestroy(papszOptions);
    
    Rcpp::CharacterVector result;
    for (const auto& attr : attrs) {
        result.push_back(attr->GetName());
    }
    return result;
}

GDALAttributeR GDALGroupR::getAttribute(const std::string& osName) const {
    if (!isValid()) return GDALAttributeR();
    
    auto poAttr = m_poGroup->GetAttribute(osName);
    if (!poAttr) return GDALAttributeR();
    
    return GDALAttributeR(poAttr);
}

Rcpp::List GDALGroupR::getAttributes(
        Rcpp::CharacterVector options) const {
    
    Rcpp::List result;
    if (!isValid()) return result;
    
    char** papszOptions = charVecToCSL(options);
    auto attrs = m_poGroup->GetAttributes(papszOptions);
    CSLDestroy(papszOptions);
    
    for (const auto& attr : attrs) {
        result.push_back(GDALAttributeR(attr));
    }
    return result;
}

GDALAttributeR GDALGroupR::createAttribute(
        const std::string& osName,
        Rcpp::NumericVector dimensions,
        const SEXP oType,
        Rcpp::CharacterVector options) {
    
    GDALExtendedDataTypeR* pType = unwrapModulePtr<GDALExtendedDataTypeR>(oType);
    if (!isValid()) return GDALAttributeR();
    
    std::vector<GUInt64> anDimensions(dimensions.size());
    for (R_xlen_t i = 0; i < dimensions.size(); ++i) {
        anDimensions[i] = static_cast<GUInt64>(dimensions[i]);
    }
    
    char** papszOptions = charVecToCSL(options);
    auto poAttr = m_poGroup->CreateAttribute(
        osName, anDimensions, pType->getRef(), papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poAttr) return GDALAttributeR();
    return GDALAttributeR(poAttr);
}

bool GDALGroupR::deleteAttribute(
        const std::string& osName,
        Rcpp::CharacterVector options) {
    
    if (!isValid()) return false;
    
    char** papszOptions = charVecToCSL(options);
    bool bRet = m_poGroup->DeleteAttribute(osName, papszOptions);
    CSLDestroy(papszOptions);
    return bRet;
}

Rcpp::CharacterVector GDALGroupR::getVectorLayerNames(
        Rcpp::CharacterVector options) const {
    
    if (!isValid()) return Rcpp::CharacterVector();
    
    char** papszOptions = charVecToCSL(options);
    std::vector<std::string> names = m_poGroup->GetVectorLayerNames(papszOptions);
    CSLDestroy(papszOptions);
    
    return Rcpp::wrap(names);
}

std::string GDALGroupR::getStructuralInfo() const {
    if (!isValid()) return "";
    
    CSLConstList papszInfo = m_poGroup->GetStructuralInfo();
    if (!papszInfo) return "";
    
    std::string result;
    for (int i = 0; papszInfo[i] != nullptr; ++i) {
        if (i > 0) result += "\n";
        result += papszInfo[i];
    }
    return result;
}

bool GDALGroupR::rename(const std::string& osNewName) {
    if (!isValid()) return false;
    return m_poGroup->Rename(osNewName);
}

// ============================================================================
// GDALMultiDimRaster implementation
// ============================================================================

GDALMultiDimRaster::GDALMultiDimRaster()
    : m_osFilename("")
    , m_hDataset(nullptr)
    , m_bReadOnly(true)
    , m_bShared(true)
    , m_bQuiet(false) {
}

GDALMultiDimRaster::GDALMultiDimRaster(
        Rcpp::CharacterVector filename,
        bool read_only,
        Rcpp::CharacterVector open_options,
        bool shared)
    : m_osFilename("")
    , m_hDataset(nullptr)
    , m_bReadOnly(read_only)
    , m_bShared(shared)
    , m_bQuiet(false)
    , m_openOptions(open_options) {
    
    if (filename.size() > 0) {
        m_osFilename = Rcpp::as<std::string>(filename[0]);
    }
    
    if (!m_osFilename.empty()) {
        if (!openInternal()) {
            Rcpp::stop("Failed to open dataset: " + m_osFilename);
        }
    }
}

GDALMultiDimRaster::GDALMultiDimRaster(const GDALMultiDimRaster& other)
    : m_osFilename(other.m_osFilename)
    , m_hDataset(nullptr)
    , m_bReadOnly(other.m_bReadOnly)
    , m_bShared(other.m_bShared)
    , m_bQuiet(other.m_bQuiet)
    , m_openOptions(other.m_openOptions)
    , m_infoOptions(other.m_infoOptions) {
    
    if (!m_osFilename.empty()) {
        openInternal();
    }
}

GDALMultiDimRaster::~GDALMultiDimRaster() {
    close();
}

bool GDALMultiDimRaster::openInternal() {
    if (m_osFilename.empty()) {
        return false;
    }
    
    // Build open flags
    unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER | GDAL_OF_VERBOSE_ERROR;
    if (m_bReadOnly) {
        nOpenFlags |= GDAL_OF_READONLY;
    } else {
        nOpenFlags |= GDAL_OF_UPDATE;
    }
    if (m_bShared) {
        nOpenFlags |= GDAL_OF_SHARED;
    }
    
    // Convert open options
    char** papszOptions = charVecToCSL(m_openOptions);
    
    m_hDataset = static_cast<GDALDataset*>(
        GDALOpenEx(m_osFilename.c_str(), nOpenFlags, nullptr, papszOptions, nullptr));
    
    CSLDestroy(papszOptions);
    
    return m_hDataset != nullptr;
}

void GDALMultiDimRaster::open(bool read_only) {
    if (isOpen()) {
        close();
    }
    m_bReadOnly = read_only;
    if (!openInternal()) {
        Rcpp::stop("Failed to open dataset: " + m_osFilename);
    }
}

void GDALMultiDimRaster::close() {
    if (m_hDataset != nullptr) {
        GDALClose(m_hDataset);
        m_hDataset = nullptr;
    }
}

void GDALMultiDimRaster::checkOpen() const {
    if (!isOpen()) {
        Rcpp::stop("Dataset is not open");
    }
}

std::string GDALMultiDimRaster::info(Rcpp::CharacterVector options) const {
    checkOpen();
    
    // Merge with stored info options
    Rcpp::CharacterVector allOptions = Rcpp::clone(m_infoOptions);
    for (R_xlen_t i = 0; i < options.size(); ++i) {
        allOptions.push_back(options[i]);
    }
    
    // Ensure -json flag is present for structured output
    bool hasJson = false;
    for (R_xlen_t i = 0; i < allOptions.size(); ++i) {
        if (std::string(allOptions[i]) == "-json") {
            hasJson = true;
            break;
        }
    }
    if (!hasJson) {
        allOptions.push_back("-json");
    }
    
    char** papszOptions = charVecToCSL(allOptions);
    GDALMultiDimInfoOptions* psOptions = 
        GDALMultiDimInfoOptionsNew(papszOptions, nullptr);
    CSLDestroy(papszOptions);
    
    if (!psOptions) {
        return "";
    }
    
    char* pszInfo = GDALMultiDimInfo(m_hDataset, psOptions);
    GDALMultiDimInfoOptionsFree(psOptions);
    
    if (!pszInfo) {
        return "";
    }
    
    std::string result = pszInfo;
    CPLFree(pszInfo);
    return result;
}

GDALGroupR GDALMultiDimRaster::getRootGroup() const {
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return GDALGroupR();
    
    return GDALGroupR(poGroup);
}

std::string GDALMultiDimRaster::getDriverShortName() const {
    checkOpen();
    
    GDALDriver* poDriver = m_hDataset->GetDriver();
    if (!poDriver) return "";
    return poDriver->GetDescription();
}

std::string GDALMultiDimRaster::getDriverLongName() const {
    checkOpen();
    
    GDALDriver* poDriver = m_hDataset->GetDriver();
    if (!poDriver) return "";
    
    const char* pszLongName = poDriver->GetMetadataItem(GDAL_DMD_LONGNAME);
    return pszLongName ? pszLongName : "";
}

Rcpp::CharacterVector GDALMultiDimRaster::getFileList() const {
    checkOpen();
    
    char** papszFiles = m_hDataset->GetFileList();
    Rcpp::CharacterVector result;
    
    if (papszFiles) {
        for (int i = 0; papszFiles[i] != nullptr; ++i) {
            result.push_back(papszFiles[i]);
        }
        CSLDestroy(papszFiles);
    }
    
    return result;
}

Rcpp::CharacterVector GDALMultiDimRaster::getMetadata(
        const std::string& domain) const {
    
    checkOpen();
    
    CSLConstList papszMD = m_hDataset->GetMetadata(domain.empty() ? nullptr : domain.c_str());
    Rcpp::CharacterVector result;
    
    if (papszMD) {
        for (int i = 0; papszMD[i] != nullptr; ++i) {
            result.push_back(papszMD[i]);
        }
    }
    
    return result;
}

std::string GDALMultiDimRaster::getMetadataItem(
        const std::string& name,
        const std::string& domain) const {
    
    checkOpen();
    
    const char* pszValue = m_hDataset->GetMetadataItem(
        name.c_str(), domain.empty() ? nullptr : domain.c_str());
    
    return pszValue ? pszValue : "";
}

bool GDALMultiDimRaster::setMetadata(
        Rcpp::CharacterVector metadata,
        const std::string& domain) {
    
    checkOpen();
    
    char** papszMD = charVecToCSL(metadata);
    CPLErr err = m_hDataset->SetMetadata(
        papszMD, domain.empty() ? nullptr : domain.c_str());
    CSLDestroy(papszMD);
    
    return err == CE_None;
}

bool GDALMultiDimRaster::setMetadataItem(
        const std::string& name,
        const std::string& value,
        const std::string& domain) {
    
    checkOpen();
    
    CPLErr err = m_hDataset->SetMetadataItem(
        name.c_str(), value.c_str(),
        domain.empty() ? nullptr : domain.c_str());
    
    return err == CE_None;
}

Rcpp::CharacterVector GDALMultiDimRaster::getMetadataDomainList() const {
    checkOpen();
    
    char** papszDomains = m_hDataset->GetMetadataDomainList();
    Rcpp::CharacterVector result;
    
    if (papszDomains) {
        for (int i = 0; papszDomains[i] != nullptr; ++i) {
            result.push_back(papszDomains[i]);
        }
        CSLDestroy(papszDomains);
    }
    
    return result;
}

void GDALMultiDimRaster::flushCache() {
    if (isOpen()) {
        m_hDataset->FlushCache();
    }
}

GDALMultiDimRaster GDALMultiDimRaster::createMultiDimensional(
        const std::string& pszFilename,
        const std::string& pszDriverName,
        SEXP poRootGroup,
        Rcpp::CharacterVector options) {
    
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName.c_str());
    if (!poDriver) {
        Rcpp::stop("Driver not found: " + pszDriverName);
    }
    
    // Check if driver supports multidimensional creation
    const char* pszCap = poDriver->GetMetadataItem(GDAL_DCAP_MULTIDIM_RASTER);
    if (!pszCap || !EQUAL(pszCap, "YES")) {
        Rcpp::stop("Driver does not support multidimensional raster: " + pszDriverName);
    }
    
    char** papszOptions = charVecToCSL(options);
    
    // Create the dataset
    GDALDataset* poDS = poDriver->CreateMultiDimensional(
        pszFilename.c_str(), nullptr, papszOptions);
    
    CSLDestroy(papszOptions);
    
    if (!poDS) {
        Rcpp::stop("Failed to create multidimensional dataset");
    }
    
    // Wrap in our class — the copy constructor will close this handle
    // and reopen from pszFilename in update mode
    GDALMultiDimRaster raster;
    raster.m_osFilename = pszFilename;
    raster.m_hDataset = poDS;
    raster.m_bReadOnly = false;
    
    return raster;
}

Rcpp::CharacterVector GDALMultiDimRaster::getArrayNames() const {
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return Rcpp::CharacterVector();
    
    return Rcpp::wrap(poGroup->GetMDArrayNames());
}

GDALMDArrayR GDALMultiDimRaster::openArray(
        const std::string& osName,
        Rcpp::CharacterVector options) const {
    
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return GDALMDArrayR();
    
    char** papszOptions = charVecToCSL(options);
    auto poArray = poGroup->OpenMDArray(osName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poArray) return GDALMDArrayR();
    return GDALMDArrayR(poArray);
}

GDALMDArrayR GDALMultiDimRaster::openArrayFromFullname(
        const std::string& osFullname,
        Rcpp::CharacterVector options) const {
    
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return GDALMDArrayR();
    
    char** papszOptions = charVecToCSL(options);
    auto poArray = poGroup->OpenMDArrayFromFullname(osFullname, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poArray) return GDALMDArrayR();
    return GDALMDArrayR(poArray);
}

Rcpp::CharacterVector GDALMultiDimRaster::getSubGroupNames() const {
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return Rcpp::CharacterVector();
    
    return Rcpp::wrap(poGroup->GetGroupNames());
}

GDALGroupR GDALMultiDimRaster::openSubGroup(
        const std::string& osName,
        Rcpp::CharacterVector options) const {
    
    checkOpen();
    
    auto poGroup = m_hDataset->GetRootGroup();
    if (!poGroup) return GDALGroupR();
    
    char** papszOptions = charVecToCSL(options);
    auto poSubGroup = poGroup->OpenGroup(osName, papszOptions);
    CSLDestroy(papszOptions);
    
    if (!poSubGroup) return GDALGroupR();
    return GDALGroupR(poSubGroup);
}
