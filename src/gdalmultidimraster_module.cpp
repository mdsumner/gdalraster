/* GDAL Multidimensional Raster Interface for R
   Rcpp Module definitions for exposing C++ classes to R
   Part of gdalraster package: https://github.com/firelab/gdalraster
   
   This file defines the Rcpp module interface that makes the C++ classes
   accessible from R. It should be included in the package's RcppExports.cpp
   or a dedicated module file.
*/

#include "gdalmultidimraster.h"

using namespace Rcpp;

// ============================================================================
// Module definition
// ============================================================================

RCPP_MODULE(mod_GDALMultiDimRaster) {
    
    // -------------------------------------------------------------------------
    // GDALExtendedDataTypeR class
    // -------------------------------------------------------------------------
    class_<GDALExtendedDataTypeR>("GDALExtendedDataTypeR")
        .constructor("Create a GDALExtendedDataType wrapper")
        
        // Static factory methods exposed as regular functions
        .method("getName", &GDALExtendedDataTypeR::getName,
                "Get the type name")
        .method("getClass", &GDALExtendedDataTypeR::getClass,
                "Get the type class (0=NUMERIC, 1=STRING, 2=COMPOUND)")
        .method("getClassAsString", &GDALExtendedDataTypeR::getClassAsString,
                "Get the type class as string")
        .method("getNumericDataType", &GDALExtendedDataTypeR::getNumericDataType,
                "Get the numeric data type (GDALDataType)")
        .method("getNumericDataTypeAsString", &GDALExtendedDataTypeR::getNumericDataTypeAsString,
                "Get the numeric data type name")
        .method("getSize", &GDALExtendedDataTypeR::getSize,
                "Get the size in bytes")
        .method("getMaxStringLength", &GDALExtendedDataTypeR::getMaxStringLength,
                "Get max string length (0 = unlimited)")
        .method("getComponents", &GDALExtendedDataTypeR::getComponents,
                "Get compound type components")
        .method("canConvertTo", &GDALExtendedDataTypeR::canConvertTo,
                "Check if can convert to another type")
        .method("equals", &GDALExtendedDataTypeR::equals,
                "Check equality with another type")
    ;
    
    // -------------------------------------------------------------------------
    // GDALDimensionR class  
    // -------------------------------------------------------------------------
    class_<GDALDimensionR>("GDALDimensionR")
        .constructor("Create an empty GDALDimension wrapper")
        
        .method("isValid", &GDALDimensionR::isValid,
                "Check if dimension is valid")
        .method("getName", &GDALDimensionR::getName,
                "Get the dimension name")
        .method("getFullName", &GDALDimensionR::getFullName,
                "Get the full dimension name")
        .method("getType", &GDALDimensionR::getType,
                "Get the dimension type (HORIZONTAL_X, HORIZONTAL_Y, VERTICAL, TEMPORAL, ...)")
        .method("getDirection", &GDALDimensionR::getDirection,
                "Get the dimension direction")
        .method("getSize", &GDALDimensionR::getSize,
                "Get the dimension size")
        .method("getIndexingVariable", &GDALDimensionR::getIndexingVariable,
                "Get the indexing variable (GDALMDArrayR or NULL)")
        .method("setIndexingVariable", &GDALDimensionR::setIndexingVariable,
                "Set the indexing variable")
        .method("rename", &GDALDimensionR::rename,
                "Rename the dimension")
    ;
    
    // -------------------------------------------------------------------------
    // GDALAttributeR class
    // -------------------------------------------------------------------------
    class_<GDALAttributeR>("GDALAttributeR")
        .constructor("Create an empty GDALAttribute wrapper")
        
        .method("isValid", &GDALAttributeR::isValid,
                "Check if attribute is valid")
        .method("getName", &GDALAttributeR::getName,
                "Get the attribute name")
        .method("getFullName", &GDALAttributeR::getFullName,
                "Get the full attribute name")
        .method("getTotalElementsCount", &GDALAttributeR::getTotalElementsCount,
                "Get total number of elements")
        .method("getDimensionCount", &GDALAttributeR::getDimensionCount,
                "Get the number of dimensions")
        .method("getDimensionsSize", &GDALAttributeR::getDimensionsSize,
                "Get the size of each dimension")
        .method("getDataType", &GDALAttributeR::getDataType,
                "Get the data type")
        
        // Read methods
        .method("readAsRaw", &GDALAttributeR::readAsRaw,
                "Read value as raw bytes")
        .method("readAsString", &GDALAttributeR::readAsString,
                "Read value as string")
        .method("readAsInt", &GDALAttributeR::readAsInt,
                "Read value as integer")
        .method("readAsDouble", &GDALAttributeR::readAsDouble,
                "Read value as double")
        .method("readAsStringArray", &GDALAttributeR::readAsStringArray,
                "Read value as character vector")
        .method("readAsIntArray", &GDALAttributeR::readAsIntArray,
                "Read value as integer vector")
        .method("readAsDoubleArray", &GDALAttributeR::readAsDoubleArray,
                "Read value as numeric vector")
        
        // Write methods
        .method("write", &GDALAttributeR::write,
                "Write raw bytes")
        .method("writeString", &GDALAttributeR::writeString,
                "Write string value")
        .method("writeInt", &GDALAttributeR::writeInt,
                "Write integer value")
        .method("writeDouble", &GDALAttributeR::writeDouble,
                "Write double value")
        .method("writeStringArray", &GDALAttributeR::writeStringArray,
                "Write character vector")
        .method("writeIntArray", &GDALAttributeR::writeIntArray,
                "Write integer vector")
        .method("writeDoubleArray", &GDALAttributeR::writeDoubleArray,
                "Write numeric vector")
        .method("rename", &GDALAttributeR::rename,
                "Rename the attribute")
    ;
    
    // -------------------------------------------------------------------------
    // GDALMDArrayR class
    // -------------------------------------------------------------------------
    class_<GDALMDArrayR>("GDALMDArrayR")
        .constructor("Create an empty GDALMDArray wrapper")
        
        .method("isValid", &GDALMDArrayR::isValid,
                "Check if array is valid")
        .method("getName", &GDALMDArrayR::getName,
                "Get the array name")
        .method("getFullName", &GDALMDArrayR::getFullName,
                "Get the full array name")
        .method("getTotalElementsCount", &GDALMDArrayR::getTotalElementsCount,
                "Get total number of elements")
        .method("getDimensionCount", &GDALMDArrayR::getDimensionCount,
                "Get the number of dimensions")
        .method("getDimensions", &GDALMDArrayR::getDimensions,
                "Get list of dimensions")
        .method("getDataType", &GDALMDArrayR::getDataType,
                "Get the data type")
        .method("getSpatialRef", &GDALMDArrayR::getSpatialRef,
                "Get spatial reference as WKT")
        .method("setSpatialRef", &GDALMDArrayR::setSpatialRef,
                "Set spatial reference from WKT")
        .method("getUnit", &GDALMDArrayR::getUnit,
                "Get the unit string")
        .method("setUnit", &GDALMDArrayR::setUnit,
                "Set the unit string")
        .method("getNoDataValueAsDouble", &GDALMDArrayR::getNoDataValueAsDouble,
                "Get nodata value as double (empty if not set)")
        .method("getNoDataValueAsRaw", &GDALMDArrayR::getNoDataValueAsRaw,
                "Get nodata value as raw bytes")
        .method("setNoDataValue", &GDALMDArrayR::setNoDataValue,
                "Set nodata value")
        .method("setNoDataValueRaw", &GDALMDArrayR::setNoDataValueRaw,
                "Set nodata value from raw bytes")
        .method("deleteNoDataValue", &GDALMDArrayR::deleteNoDataValue,
                "Delete nodata value")
        .method("getOffset", &GDALMDArrayR::getOffset,
                "Get offset value (empty if not set)")
        .method("getScale", &GDALMDArrayR::getScale,
                "Get scale value (empty if not set)")
        .method("setOffset", &GDALMDArrayR::setOffset,
                "Set offset value")
        .method("setScale", &GDALMDArrayR::setScale,
                "Set scale value")
        .method("getBlockSize", &GDALMDArrayR::getBlockSize,
                "Get block sizes for each dimension")
        .method("getProcessingChunkSize", &GDALMDArrayR::getProcessingChunkSize,
                "Get recommended chunk size for processing")
        .method("getStructuralInfo", &GDALMDArrayR::getStructuralInfo,
                "Get structural info string")
        
        // Attributes
        .method("getAttributeNames", &GDALMDArrayR::getAttributeNames,
                "Get attribute names")
        .method("getAttribute", &GDALMDArrayR::getAttribute,
                "Get attribute by name")
        .method("getAttributes", &GDALMDArrayR::getAttributes,
                "Get list of all attributes")
        .method("createAttribute", &GDALMDArrayR::createAttribute,
                "Create a new attribute")
        .method("deleteAttribute", &GDALMDArrayR::deleteAttribute,
                "Delete an attribute")
        
        // I/O
        .method("read", &GDALMDArrayR::read,
                "Read array data")
        .method("write", &GDALMDArrayR::write,
                "Write array data")
        .method("adviseRead", &GDALMDArrayR::adviseRead,
                "Advise read for optimization")
        
        // Views and transforms
        .method("getView", &GDALMDArrayR::getView,
                "Get a view of the array")
        .method("transpose", &GDALMDArrayR::transpose,
                "Get transposed view")
        .method("getUnscaled", &GDALMDArrayR::getUnscaled,
                "Get unscaled view (apply scale/offset)")
        .method("getMask", &GDALMDArrayR::getMask,
                "Get mask array")
        .method("getResampled", &GDALMDArrayR::getResampled,
                "Get resampled/reprojected view")
        .method("asClassicDataset", &GDALMDArrayR::asClassicDataset,
                "Convert to classic 2D dataset")
        
        // Other
        .method("cache", &GDALMDArrayR::cache,
                "Cache array data")
        .method("computeStatistics", &GDALMDArrayR::computeStatistics,
                "Compute statistics")
        .method("resize", &GDALMDArrayR::resize,
                "Resize the array")
        .method("rename", &GDALMDArrayR::rename,
                "Rename the array")
        
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
        // Raw block info (chunk byte-range references)
        .method("getRawBlockInfo", &GDALMDArrayR::getRawBlockInfo,
                "Get raw block info for a single chunk (filename, offset, size, info)")
        .method("getRawBlockRefs", &GDALMDArrayR::getRawBlockRefs,
                "Scan all chunks and return data.frame of byte-range references")
#endif
    ;
    
    // -------------------------------------------------------------------------
    // GDALGroupR class
    // -------------------------------------------------------------------------
    class_<GDALGroupR>("GDALGroupR")
        .constructor("Create an empty GDALGroup wrapper")
        
        .method("isValid", &GDALGroupR::isValid,
                "Check if group is valid")
        .method("getName", &GDALGroupR::getName,
                "Get the group name")
        .method("getFullName", &GDALGroupR::getFullName,
                "Get the full group name")
        
        // Subgroups
        .method("getGroupNames", &GDALGroupR::getGroupNames,
                "Get subgroup names")
        .method("openGroup", &GDALGroupR::openGroup,
                "Open a subgroup by name")
        .method("openGroupFromFullname", &GDALGroupR::openGroupFromFullname,
                "Open a subgroup by full path")
        .method("createGroup", &GDALGroupR::createGroup,
                "Create a new subgroup")
        .method("deleteGroup", &GDALGroupR::deleteGroup,
                "Delete a subgroup")
        
        // Arrays
        .method("getMDArrayNames", &GDALGroupR::getMDArrayNames,
                "Get array names")
        .method("openMDArray", &GDALGroupR::openMDArray,
                "Open an array by name")
        .method("openMDArrayFromFullname", &GDALGroupR::openMDArrayFromFullname,
                "Open an array by full path")
        .method("createMDArray", &GDALGroupR::createMDArray,
                "Create a new array")
        .method("deleteMDArray", &GDALGroupR::deleteMDArray,
                "Delete an array")
        
        // Dimensions
        .method("getDimensions", &GDALGroupR::getDimensions,
                "Get group dimensions")
        .method("createDimension", &GDALGroupR::createDimension,
                "Create a new dimension")
        
        // Attributes
        .method("getAttributeNames", &GDALGroupR::getAttributeNames,
                "Get attribute names")
        .method("getAttribute", &GDALGroupR::getAttribute,
                "Get attribute by name")
        .method("getAttributes", &GDALGroupR::getAttributes,
                "Get list of all attributes")
        .method("createAttribute", &GDALGroupR::createAttribute,
                "Create a new attribute")
        .method("deleteAttribute", &GDALGroupR::deleteAttribute,
                "Delete an attribute")
        
        // Vector layers
        .method("getVectorLayerNames", &GDALGroupR::getVectorLayerNames,
                "Get vector layer names")
        
        // Other
        .method("getStructuralInfo", &GDALGroupR::getStructuralInfo,
                "Get structural info string")
        .method("rename", &GDALGroupR::rename,
                "Rename the group")
    ;
    
    // -------------------------------------------------------------------------
    // GDALMultiDimRaster class (main class)
    // -------------------------------------------------------------------------
    class_<GDALMultiDimRaster>("GDALMultiDimRaster")
        .constructor("Create an empty GDALMultiDimRaster object")
        .constructor<CharacterVector, bool, CharacterVector, bool>(
            "Open a multidimensional raster dataset")
        
        // Dataset operations
        .method("open", &GDALMultiDimRaster::open,
                "Open/reopen the dataset")
        .method("close", &GDALMultiDimRaster::close,
                "Close the dataset")
        .method("isOpen", &GDALMultiDimRaster::isOpen,
                "Check if dataset is open")
        .method("info", &GDALMultiDimRaster::info,
                "Get dataset info as JSON string")
        .method("getRootGroup", &GDALMultiDimRaster::getRootGroup,
                "Get the root group")
        .method("getDriverShortName", &GDALMultiDimRaster::getDriverShortName,
                "Get driver short name")
        .method("getDriverLongName", &GDALMultiDimRaster::getDriverLongName,
                "Get driver long name")
        .method("getFileList", &GDALMultiDimRaster::getFileList,
                "Get list of files")
        
        // Metadata
        .method("getMetadata", &GDALMultiDimRaster::getMetadata,
                "Get metadata")
        .method("getMetadataItem", &GDALMultiDimRaster::getMetadataItem,
                "Get metadata item")
        .method("setMetadata", &GDALMultiDimRaster::setMetadata,
                "Set metadata")
        .method("setMetadataItem", &GDALMultiDimRaster::setMetadataItem,
                "Set metadata item")
        .method("getMetadataDomainList", &GDALMultiDimRaster::getMetadataDomainList,
                "Get metadata domain list")
        .method("flushCache", &GDALMultiDimRaster::flushCache,
                "Flush cache to disk")
        
        // Convenience methods
        .method("getArrayNames", &GDALMultiDimRaster::getArrayNames,
                "Get array names from root group")
        .method("openArray", &GDALMultiDimRaster::openArray,
                "Open array from root group")
        .method("openArrayFromFullname", &GDALMultiDimRaster::openArrayFromFullname,
                "Open array by full path")
        .method("getSubGroupNames", &GDALMultiDimRaster::getSubGroupNames,
                "Get subgroup names from root group")
        .method("openSubGroup", &GDALMultiDimRaster::openSubGroup,
                "Open subgroup from root group")
        
        // Properties
        .property("filename", &GDALMultiDimRaster::getFilename,
                  "Source filename")
        .property("readOnly", &GDALMultiDimRaster::getReadOnly,
                  "Read-only flag")
        .property("quiet", &GDALMultiDimRaster::getQuiet, &GDALMultiDimRaster::setQuiet,
                  "Quiet mode for messages")
        .property("infoOptions", &GDALMultiDimRaster::getInfoOptions, 
                  &GDALMultiDimRaster::setInfoOptions,
                  "Default options for info() method")
    ;
    
    // -------------------------------------------------------------------------
    // Module-level functions
    // -------------------------------------------------------------------------
    function("getMultiDimDrivers", &getMultiDimDrivers,
             "Get list of drivers supporting multidimensional raster");
    // function("gdalTypeToRType", &gdalTypeToRType,
    //          "Convert GDALDataType to R type name");
}

// ============================================================================
// Exported functions for creating extended data types (used as static methods)
// ============================================================================

// [[Rcpp::export]]
SEXP edtCreate(int dataType) {
    return Rcpp::wrap(GDALExtendedDataTypeR(static_cast<GDALDataType>(dataType)));
}

// [[Rcpp::export]]
SEXP edtCreateString(int maxLength = 0) {
    return Rcpp::wrap(
        GDALExtendedDataTypeR::CreateString(static_cast<size_t>(maxLength)));
}

// [[Rcpp::export]]
SEXP edtCreateCompound(std::string name, int totalSize, Rcpp::List components) {
    return Rcpp::wrap(GDALExtendedDataTypeR::CreateCompound(
        name, static_cast<size_t>(totalSize), components));
}

// [[Rcpp::export]]
SEXP mdimCreate(std::string filename, std::string driverName,
                Rcpp::CharacterVector options = Rcpp::CharacterVector()) {
    return Rcpp::wrap(GDALMultiDimRaster::createMultiDimensional(
        filename, driverName, R_NilValue, options));
}
