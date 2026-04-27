/* GDAL Multidimensional Raster Interface for R
 Header file for C++ classes wrapping GDAL Multidim Raster API
 Part of gdalraster package: https://github.com/firelab/gdalraster
 
 Design notes:
 - GDALMultiDimRaster wraps a GDALDataset opened with GDAL_OF_MULTIDIM_RASTER
 - GDALGroupR wraps GDALGroup (root or sub-group)
 - GDALMDArrayR wraps GDALMDArray 
 - GDALDimensionR wraps GDALDimension
 - GDALAttributeR wraps GDALAttribute
 - GDALExtendedDataTypeR wraps GDALExtendedDataType
 
 Naming: Classes suffixed with 'R' to indicate R bindings and avoid
 confusion with GDAL's own classes.
 */

#ifndef GDALMULTIDIMRASTER_H
#define GDALMULTIDIMRASTER_H

#include <string>
#include <vector>
#include <memory>

#include <Rcpp.h>
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "gdal_utils.h"

// GDAL version check for multidim support
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3,1,0)
#error "GDAL >= 3.1.0 is required for multidimensional raster support"
#endif


// Helper to extract a pointer from either a raw Rcpp::XPtr<T> (EXTPTRSXP)
// or a module-wrapped S4 object (which stores the XPtr in a ".pointer" slot).
// Handles both old-style XPtr returns and new-style module-wrapped returns.
template<typename T>
T* unwrapModulePtr(SEXP x) {
    if (TYPEOF(x) == EXTPTRSXP) {
        return Rcpp::XPtr<T>(x);
    }
    if (Rf_isS4(x)) {
        Rcpp::S4 s4(x);
        if (s4.hasSlot(".pointer")) {
            return Rcpp::XPtr<T>(s4.slot(".pointer"));
        }
        if (s4.hasSlot(".xData")) {
            Rcpp::Environment env(s4.slot(".xData"));
            SEXP ptr = env.get(".pointer");
            if (TYPEOF(ptr) == EXTPTRSXP) {
                return Rcpp::XPtr<T>(ptr);
            }
        }
    }
    Rcpp::stop("Expected a module object or external pointer");
    return nullptr;
}

// Forward declarations
class GDALGroupR;
class GDALMDArrayR;
class GDALDimensionR;
class GDALAttributeR;
class GDALExtendedDataTypeR;

// ============================================================================
// GDALExtendedDataTypeR - Wrapper for GDALExtendedDataType
// ============================================================================

class GDALExtendedDataTypeR {
public:
  // Constructors
  GDALExtendedDataTypeR();
  explicit GDALExtendedDataTypeR(GDALDataType eType);
  explicit GDALExtendedDataTypeR(const GDALExtendedDataType& oType);
  GDALExtendedDataTypeR(const GDALExtendedDataTypeR& other);
  ~GDALExtendedDataTypeR();
  
  // Static factory methods
  static GDALExtendedDataTypeR Create(GDALDataType eType);
  static GDALExtendedDataTypeR CreateString(size_t nMaxStringLength = 0);
  static GDALExtendedDataTypeR CreateCompound(
      const std::string& osName,
      size_t nTotalSize,
      Rcpp::List components);
  
  // Properties
  std::string getName() const;
  int getClass() const;  // GDALExtendedDataTypeClass enum
  std::string getClassAsString() const;
  int getNumericDataType() const;
  std::string getNumericDataTypeAsString() const;
  size_t getSize() const;
  size_t getMaxStringLength() const;
  Rcpp::List getComponents() const;
  bool canConvertTo(const SEXP other) const;
  bool equals(const SEXP other) const;
  
  // Internal accessor
  const GDALExtendedDataType& getRef() const { return m_oType; }
  
private:
  GDALExtendedDataType m_oType;
};

// ============================================================================
// GDALDimensionR - Wrapper for GDALDimension
// ============================================================================

class GDALDimensionR {
public:
  // Constructors
  GDALDimensionR();
  explicit GDALDimensionR(std::shared_ptr<GDALDimension> poDim);
  GDALDimensionR(const GDALDimensionR& other);
  ~GDALDimensionR();
  
  // Validity
  bool isValid() const { return m_poDim != nullptr; }
  
  // Properties (read-only)
  std::string getName() const;
  std::string getFullName() const;
  std::string getType() const;
  std::string getDirection() const;
  GUInt64 getSize() const;
  
  // Indexing variable
  GDALMDArrayR getIndexingVariable() const;
  bool setIndexingVariable(SEXP poArrayR);
  
  // Rename (if supported by driver)
  bool rename(const std::string& osNewName);
  
  // Internal accessor
  std::shared_ptr<GDALDimension> getSharedPtr() const { return m_poDim; }
  
private:
  std::shared_ptr<GDALDimension> m_poDim;
};

// ============================================================================
// GDALAttributeR - Wrapper for GDALAttribute
// ============================================================================

class GDALAttributeR {
public:
  // Constructors
  GDALAttributeR();
  explicit GDALAttributeR(std::shared_ptr<GDALAttribute> poAttr);
  GDALAttributeR(const GDALAttributeR& other);
  ~GDALAttributeR();
  
  // Validity
  bool isValid() const { return m_poAttr != nullptr; }
  
  // Properties
  std::string getName() const;
  std::string getFullName() const;
  GUInt64 getTotalElementsCount() const;
  Rcpp::NumericVector getDimensionCount() const;
  std::vector<GUInt64> getDimensionsSize() const;
  GDALExtendedDataTypeR getDataType() const;
  
  // Read methods - return appropriate R type based on data type
  Rcpp::RawVector readAsRaw() const;
  std::string readAsString() const;
  int readAsInt() const;
  double readAsDouble() const;
  Rcpp::CharacterVector readAsStringArray() const;
  Rcpp::IntegerVector readAsIntArray() const;
  Rcpp::NumericVector readAsDoubleArray() const;
  
  // Write methods
  bool write(Rcpp::RawVector data);
  bool writeString(const std::string& val);
  bool writeInt(int val);
  bool writeDouble(double val);
  bool writeStringArray(Rcpp::CharacterVector val);
  bool writeIntArray(Rcpp::IntegerVector val);
  bool writeDoubleArray(Rcpp::NumericVector val);
  
  // Rename (if supported)
  bool rename(const std::string& osNewName);
  
  // Internal accessor
  std::shared_ptr<GDALAttribute> getSharedPtr() const { return m_poAttr; }
  
private:
  std::shared_ptr<GDALAttribute> m_poAttr;
};

// ============================================================================
// GDALMDArrayR - Wrapper for GDALMDArray
// ============================================================================

class GDALMDArrayR {
public:
  // Constructors
  GDALMDArrayR();
  explicit GDALMDArrayR(std::shared_ptr<GDALMDArray> poArray);
  GDALMDArrayR(const GDALMDArrayR& other);
  ~GDALMDArrayR();
  
  // Validity
  bool isValid() const { return m_poArray != nullptr; }
  
  // Properties
  std::string getName() const;
  std::string getFullName() const;
  GUInt64 getTotalElementsCount() const;
  size_t getDimensionCount() const;
  Rcpp::List getDimensions() const;  // Returns list of GDALDimensionR
  GDALExtendedDataTypeR getDataType() const;
  std::string getSpatialRef() const;  // WKT
  std::string getUnit() const;
  bool setUnit(const std::string& osUnit);
  bool setSpatialRef(const std::string& osWKT);
  Rcpp::NumericVector getNoDataValueAsDouble() const;  // Length 1 or 0 if not set
  Rcpp::RawVector getNoDataValueAsRaw() const;
  bool setNoDataValue(double dfNoData);
  bool setNoDataValueRaw(Rcpp::RawVector nodata);
  bool deleteNoDataValue();
  Rcpp::NumericVector getOffset() const;  // Length 1 or 0 if not set
  Rcpp::NumericVector getScale() const;   // Length 1 or 0 if not set
  bool setOffset(double dfOffset);
  bool setScale(double dfScale);
  Rcpp::NumericVector getBlockSize() const;
  Rcpp::List getProcessingChunkSize(size_t nMaxChunkMemory) const;
  std::string getStructuralInfo() const;
  
  // Attributes
  Rcpp::CharacterVector getAttributeNames() const;
  GDALAttributeR getAttribute(const std::string& osName) const;
  Rcpp::List getAttributes() const;
  GDALAttributeR createAttribute(
      const std::string& osName,
      Rcpp::NumericVector dimensions,
      SEXP oType,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  bool deleteAttribute(const std::string& osName,
                       Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Read methods
  // Read entire array or hyperslab
  // arrayStartIdx, count, arrayStep are vectors of length getDimensionCount()
  // bufferStride can be empty for default (C-style row-major)
  SEXP read(
      Rcpp::NumericVector arrayStartIdx = Rcpp::NumericVector(),
      Rcpp::NumericVector count = Rcpp::NumericVector(),
      Rcpp::NumericVector arrayStep = Rcpp::NumericVector(),
      Rcpp::NumericVector bufferStride = Rcpp::NumericVector(),
      SEXP bufferDataType =  R_NilValue);
  
  // Write methods
  bool write(SEXP data, Rcpp::NumericVector arrayStartIdx,
             Rcpp::NumericVector count, Rcpp::NumericVector arrayStep,
             Rcpp::NumericVector bufferStride,
             SEXP bufferDataType);
  
  // AdviseRead for optimization hints
  bool adviseRead(
      Rcpp::NumericVector arrayStartIdx = Rcpp::NumericVector(),
      Rcpp::NumericVector count = Rcpp::NumericVector(),
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Views and transforms
  GDALMDArrayR getView(const std::string& osViewExpr) const;
  GDALMDArrayR transpose(Rcpp::IntegerVector anMapNewAxisToOldAxis) const;
  GDALMDArrayR getUnscaled() const;
  GDALMDArrayR getMask(Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Resampling and reprojection
  GDALMDArrayR getResampled(
      Rcpp::List apoNewDims,  // List of GDALDimensionR or empty/NULL for same
      const std::string& resampleAlg,
      const std::string& targetSRS = "",  // Empty for same
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Convert to classic dataset (2D view)
  SEXP asClassicDataset(
      size_t iXDim,
      size_t iYDim,
      SEXP poRootGroup = R_NilValue,  // GDALGroupR
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Caching
  bool cache(Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Statistics
  Rcpp::List computeStatistics(
      bool approxOK = false,
      bool force = true,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Resize (if supported)
  bool resize(Rcpp::NumericVector newDimSizes,
              Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Rename (if supported)
  bool rename(const std::string& osNewName);
  
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
  // Raw block info (chunk byte-range references)
  // Single chunk query — returns list with filename, offset, size, info, inline
  Rcpp::List getRawBlockInfo(Rcpp::IntegerVector blockIdx) const;
  
  // Bulk scan all chunks — returns data.frame with filename, offset, size,
  // info, chunk_0, chunk_1, ... (one column per dimension).
  // Missing chunks are excluded.
  Rcpp::DataFrame getRawBlockRefs() const;
#endif
  
  // Internal accessor
  std::shared_ptr<GDALMDArray> getSharedPtr() const { return m_poArray; }
  
private:
  std::shared_ptr<GDALMDArray> m_poArray;
  
  // Helper to convert R vectors to GDAL arrays
  std::vector<GUInt64> rVecToGUInt64(Rcpp::NumericVector v) const;
  std::vector<size_t> rVecToSizeT(Rcpp::NumericVector v) const;
  std::vector<GInt64> rVecToGInt64(Rcpp::NumericVector v) const;
  std::vector<GPtrDiff_t> rVecToGPtrDiff(Rcpp::NumericVector v) const;
};

// ============================================================================
// GDALGroupR - Wrapper for GDALGroup
// ============================================================================

class GDALGroupR {
public:
  // Constructors
  GDALGroupR();
  explicit GDALGroupR(std::shared_ptr<GDALGroup> poGroup);
  GDALGroupR(const GDALGroupR& other);
  ~GDALGroupR();
  
  // Validity
  bool isValid() const { return m_poGroup != nullptr; }
  
  // Properties
  std::string getName() const;
  std::string getFullName() const;
  
  // Subgroups
  Rcpp::CharacterVector getGroupNames(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALGroupR openGroup(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALGroupR openGroupFromFullname(
      const std::string& osFullName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALGroupR createGroup(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  bool deleteGroup(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Arrays
  Rcpp::CharacterVector getMDArrayNames(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALMDArrayR openMDArray(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALMDArrayR openMDArrayFromFullname(
      const std::string& osFullName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALMDArrayR createMDArray(const std::string& name, Rcpp::List dimensions,
                     SEXP oType, Rcpp::CharacterVector options);
  bool deleteMDArray(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Dimensions
  Rcpp::List getDimensions(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALDimensionR createDimension(
      const std::string& osName,
      const std::string& osType,
      const std::string& osDirection,
      GUInt64 nSize,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Attributes
  Rcpp::CharacterVector getAttributeNames(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALAttributeR getAttribute(const std::string& osName) const;
  Rcpp::List getAttributes(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  GDALAttributeR createAttribute(
      const std::string& osName,
      Rcpp::NumericVector dimensions,
      const SEXP oType,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  bool deleteAttribute(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // Vector layers (if supported)
  Rcpp::CharacterVector getVectorLayerNames(
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  // Note: Opening vector layers would return OGRLayer which is handled by GDALVector
  
  // Structural info
  std::string getStructuralInfo() const;
  
  // Rename (if supported)
  bool rename(const std::string& osNewName);
  
  // Internal accessor
  std::shared_ptr<GDALGroup> getSharedPtr() const { return m_poGroup; }
  
private:
  std::shared_ptr<GDALGroup> m_poGroup;
};

// ============================================================================
// GDALMultiDimRaster - Main class wrapping a multidimensional dataset
// ============================================================================

class GDALMultiDimRaster {
public:
  // Constructors
  GDALMultiDimRaster();
  
  // Open existing dataset
  GDALMultiDimRaster(
    Rcpp::CharacterVector filename,
    bool read_only = true,
    Rcpp::CharacterVector open_options = Rcpp::CharacterVector(),
    bool shared = true);
  
  // Copy constructor
  GDALMultiDimRaster(const GDALMultiDimRaster& other);
  
  // Destructor
  ~GDALMultiDimRaster();
  
  // -------------------------------------------------------------------------
  // Dataset operations
  // -------------------------------------------------------------------------
  
  // Open/close
  void open(bool read_only);
  void close();
  bool isOpen() const { return m_hDataset != nullptr; }
  
  // Dataset info (similar to gdalinfo -json with multidim)
  std::string info(Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Get the root group
  GDALGroupR getRootGroup() const;
  
  // Driver info
  std::string getDriverShortName() const;
  std::string getDriverLongName() const;
  
  // Files
  Rcpp::CharacterVector getFileList() const;
  
  // Metadata (dataset level)
  Rcpp::CharacterVector getMetadata(
      const std::string& domain = "") const;
  std::string getMetadataItem(
      const std::string& name,
      const std::string& domain = "") const;
  bool setMetadata(
      Rcpp::CharacterVector metadata,
      const std::string& domain = "");
  bool setMetadataItem(
      const std::string& name,
      const std::string& value,
      const std::string& domain = "");
  Rcpp::CharacterVector getMetadataDomainList() const;
  
  // Flush cache
  void flushCache();
  
  // -------------------------------------------------------------------------
  // Static methods for creating new multidimensional datasets
  // -------------------------------------------------------------------------
  
  // Create a new multidimensional dataset using a driver that supports it
  static GDALMultiDimRaster createMultiDimensional(
      const std::string& pszFilename,
      const std::string& pszDriverName,
      SEXP poRootGroup,  // GDALGroupR with initial structure, or NULL
      Rcpp::CharacterVector options = Rcpp::CharacterVector());
  
  // -------------------------------------------------------------------------
  // Convenience accessors (shortcuts to common operations)
  // -------------------------------------------------------------------------
  
  // Get array names from root group
  Rcpp::CharacterVector getArrayNames() const;
  
  // Open array from root group by name
  GDALMDArrayR openArray(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Open array by full path
  GDALMDArrayR openArrayFromFullname(
      const std::string& osFullname,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // Get subgroup names from root group
  Rcpp::CharacterVector getSubGroupNames() const;
  
  // Open subgroup from root group
  GDALGroupR openSubGroup(
      const std::string& osName,
      Rcpp::CharacterVector options = Rcpp::CharacterVector()) const;
  
  // -------------------------------------------------------------------------
  // R fields (exposed via Rcpp modules)
  // -------------------------------------------------------------------------
  
  // Dataset source
  std::string getFilename() const { return m_osFilename; }
  
  // Read-only status
  bool getReadOnly() const { return m_bReadOnly; }
  
  // Quiet mode
  bool getQuiet() const { return m_bQuiet; }
  void setQuiet(bool quiet) { m_bQuiet = quiet; }
  
  // Info options (for info() method)
  Rcpp::CharacterVector getInfoOptions() const { return m_infoOptions; }
  void setInfoOptions(Rcpp::CharacterVector options) { m_infoOptions = options; }
  
  // -------------------------------------------------------------------------
  // Internal accessors
  // -------------------------------------------------------------------------
  GDALDataset* getDataset() const { return m_hDataset; }
  GDALDatasetH getDatasetH() const { return static_cast<GDALDatasetH>(m_hDataset); }
  
private:
  std::string m_osFilename;
  GDALDataset* m_hDataset;
  bool m_bReadOnly;
  bool m_bShared;
  bool m_bQuiet;
  Rcpp::CharacterVector m_openOptions;
  Rcpp::CharacterVector m_infoOptions;
  
  // Internal open with options
  bool openInternal();
  
  // Check dataset validity
  void checkOpen() const;
};

// Enable Rcpp::wrap() / Rcpp::as() for module classes (needed for
// returning these types from methods and building lists of them)
RCPP_EXPOSED_CLASS(GDALExtendedDataTypeR)
RCPP_EXPOSED_CLASS(GDALDimensionR)
RCPP_EXPOSED_CLASS(GDALAttributeR)
RCPP_EXPOSED_CLASS(GDALMDArrayR)
RCPP_EXPOSED_CLASS(GDALGroupR)
RCPP_EXPOSED_CLASS(GDALMultiDimRaster)

// ============================================================================
// Helper functions (non-member)
// ============================================================================

// Convert GDALDataType to R type name
std::string gdalTypeToRType(GDALDataType eType);

// Convert GDAL resample algorithm string to enum
GDALRIOResampleAlg stringToResampleAlg(const std::string& alg);

// Convert character vector to CSLConstList (caller must free with CSLDestroy)
char** charVecToCSL(Rcpp::CharacterVector cv);

// Get list of drivers supporting GDAL_DCAP_MULTIDIM_RASTER
Rcpp::CharacterVector getMultiDimDrivers();

#endif // GDALMULTIDIMRASTER_H
