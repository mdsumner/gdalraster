#include "gdalmultidimraster.h"


// [[Rcpp::export]]
SEXP mdim_array_read(SEXP arr,
                     Rcpp::Nullable<Rcpp::NumericVector> start = R_NilValue,
                     Rcpp::Nullable<Rcpp::NumericVector> count = R_NilValue,
                     Rcpp::Nullable<Rcpp::NumericVector> step = R_NilValue,
                     bool decode = true) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    Rcpp::stop("Invalid array");
  }
  
  Rcpp::NumericVector startVec = start.isNull() ?
  Rcpp::NumericVector() : Rcpp::NumericVector(start);
  Rcpp::NumericVector countVec = count.isNull() ?
  Rcpp::NumericVector() : Rcpp::NumericVector(count);
  Rcpp::NumericVector stepVec = step.isNull() ?
  Rcpp::NumericVector() : Rcpp::NumericVector(step);
  
  SEXP result = pArr->read(startVec, countVec, stepVec,
                           Rcpp::NumericVector(), R_NilValue);
  
  if (Rf_isNull(result)) {
    return result;
  }
  
  // Get data type info for type decisions
  Rcpp::XPtr<GDALExtendedDataTypeR> pDT(pArr->getDataType());
  int gdalType = pDT->getNumericDataType();  // GDALDataType enum
  std::string dtName = pDT->getNumericDataTypeAsString();
  
  // GDALDataType values:
  // GDT_Byte=1, GDT_UInt16=2, GDT_Int16=3, GDT_UInt32=4, GDT_Int32=5,
  // GDT_Float32=6, GDT_Float64=7, GDT_UInt64=12, GDT_Int64=13, GDT_Int8=14
  
  // Type categories for R output:
  // - Byte (1) → raw
  // - Int8 (14), Int16 (3), UInt16 (2), Int32 (5) → integer
  // - UInt32 (4), Int64 (13), UInt64 (12), Float32 (6), Float64 (7) → numeric
  //   (UInt32 can exceed R's signed int max ~2.1B)
  bool isByteType = (gdalType == 1);  // GDT_Byte
  bool isIntegerType = (gdalType == 2 || gdalType == 3 || 
                        gdalType == 5 || gdalType == 14);  // exclude UInt32
  // UInt32, 64-bit integers and floats → numeric
  
  // Check for CF decoding parameters
  Rcpp::NumericVector nodata = pArr->getNoDataValueAsDouble();
  Rcpp::NumericVector scale = pArr->getScale();
  Rcpp::NumericVector offset = pArr->getOffset();
  
  bool hasNodata = nodata.size() > 0;
  bool hasScale = scale.size() > 0 && scale[0] != 1.0;
  bool hasOffset = offset.size() > 0 && offset[0] != 0.0;
  bool needsDecode = decode && (hasNodata || hasScale || hasOffset);
  
  // Prepare output data
  SEXP data;
  
  if (needsDecode) {
    // Decoding requires numeric (double) output
    Rcpp::NumericVector numData = Rcpp::as<Rcpp::NumericVector>(result);
    double dfNodata = hasNodata ? nodata[0] : NA_REAL;
    double dfScale = hasScale ? scale[0] : 1.0;
    double dfOffset = hasOffset ? offset[0] : 0.0;
    
    for (R_xlen_t i = 0; i < numData.size(); ++i) {
      if (hasNodata && numData[i] == dfNodata) {
        numData[i] = NA_REAL;
      } else if (hasScale || hasOffset) {
        numData[i] = numData[i] * dfScale + dfOffset;
      }
    }
    numData.attr("dim") = R_NilValue;  // flat vector
    data = numData;
  } else if (isByteType) {
    // Byte → raw
    Rcpp::RawVector rawData = Rcpp::as<Rcpp::RawVector>(result);
    rawData.attr("dim") = R_NilValue;  // flat vector
    data = rawData;
  } else if (isIntegerType) {
    // Int8/Int16/UInt16/Int32 → integer
    Rcpp::IntegerVector intData = Rcpp::as<Rcpp::IntegerVector>(result);
    intData.attr("dim") = R_NilValue;  // flat vector
    data = intData;
  } else {
    // UInt32/Float32/Float64/Int64/UInt64 → numeric
    Rcpp::NumericVector numData = Rcpp::as<Rcpp::NumericVector>(result);
    numData.attr("dim") = R_NilValue;  // flat vector
    data = numData;
  }
  
  // Build $gis attribute (always included)
  Rcpp::List rawDims = pArr->getDimensions();
  int ndim = rawDims.size();
  
  // Build dim and dim_names in R order (reversed from GDAL C-order)
  Rcpp::NumericVector rdim(ndim);
  Rcpp::CharacterVector dim_names(ndim);
  Rcpp::List coords(ndim);
  
  // Track spatial dims for bbox
  int lon_r_idx = -1, lat_r_idx = -1;
  double xmin = NA_REAL, xmax = NA_REAL, ymin = NA_REAL, ymax = NA_REAL;
  
  for (int i = 0; i < ndim; ++i) {
    int r_idx = ndim - 1 - i;  // reverse for R order
    Rcpp::XPtr<GDALDimensionR> pDim(rawDims[i]);
    
    std::string dimName = pDim->getName();
    GUInt64 dimSize = pDim->getSize();
    std::string dimType = pDim->getType();
    
    dim_names[r_idx] = dimName;
    
    // Compute actual size for this read
    // count IS the output size; step affects which source indices are read
    GUInt64 actualSize = dimSize;
    if (countVec.size() > (R_xlen_t)i) {
      actualSize = static_cast<GUInt64>(countVec[i]);
    }
    rdim[r_idx] = static_cast<double>(actualSize);
    
    // Read coordinate values
    SEXP coordArr = pDim->getIndexingVariable();
    if (!Rf_isNull(coordArr)) {
      Rcpp::XPtr<GDALMDArrayR> pCoord(coordArr);
      
      // Check if coord is 1D or 2D
      size_t coordNdim = pCoord->getDimensionCount();
      
      if (coordNdim == 1) {
        // 1D coord - subset if we're subsetting the data
        Rcpp::NumericVector coordStart, coordCount, coordStep;
        if (startVec.size() > (R_xlen_t)i) {
          coordStart = Rcpp::NumericVector::create(startVec[i]);
        }
        if (countVec.size() > (R_xlen_t)i) {
          coordCount = Rcpp::NumericVector::create(countVec[i]);
        }
        if (stepVec.size() > (R_xlen_t)i) {
          coordStep = Rcpp::NumericVector::create(stepVec[i]);
        }
        coords[r_idx] = pCoord->read(coordStart, coordCount, coordStep,
                                     Rcpp::NumericVector(), R_NilValue);
      } else {
        // 2D coord (curvilinear) - read full for now
        // TODO: subset based on which dims match
        coords[r_idx] = pCoord->read(Rcpp::NumericVector(), Rcpp::NumericVector(),
                                     Rcpp::NumericVector(), Rcpp::NumericVector(), 
                                     R_NilValue);
      }
      
      // Track spatial dims for bbox (1D coords only)
      if (coordNdim == 1) {
        Rcpp::NumericVector cv = Rcpp::as<Rcpp::NumericVector>(coords[r_idx]);
        if (dimType == "HORIZONTAL_X") {
          lon_r_idx = r_idx;
          xmin = Rcpp::min(cv);
          xmax = Rcpp::max(cv);
        } else if (dimType == "HORIZONTAL_Y") {
          lat_r_idx = r_idx;
          ymin = Rcpp::min(cv);
          ymax = Rcpp::max(cv);
        }
      }
    } else {
      // No coord variable - generate 0:(n-1)
      int n = static_cast<int>(actualSize);
      Rcpp::NumericVector seq(n);
      for (int j = 0; j < n; ++j) seq[j] = j;
      coords[r_idx] = seq;
    }
  }
  coords.names() = dim_names;
  
  // Build gis list (matching read_ds() structure)
  Rcpp::List gisAttr = Rcpp::List::create(
    Rcpp::Named("type") = "multidim",
    Rcpp::Named("dim") = rdim,
    Rcpp::Named("dim_names") = dim_names,
    Rcpp::Named("coords") = coords,
    Rcpp::Named("srs") = pArr->getSpatialRef(),
    Rcpp::Named("datatype") = dtName
  );
  
  // Add CF encoding parameters as scalars (useful for manual decode)
  if (nodata.size() > 0) {
    gisAttr["nodata"] = nodata[0];
  }
  if (scale.size() > 0) {
    gisAttr["scale"] = scale[0];
  }
  if (offset.size() > 0) {
    gisAttr["offset"] = offset[0];
  }
  
  // Add bbox if we found both spatial dims with 1D coords
  if (lon_r_idx >= 0 && lat_r_idx >= 0) {
    // Adjust for cell centers to cell edges (half cell)
    Rcpp::NumericVector lonCoords = Rcpp::as<Rcpp::NumericVector>(coords[lon_r_idx]);
    Rcpp::NumericVector latCoords = Rcpp::as<Rcpp::NumericVector>(coords[lat_r_idx]);
    
    double dx = 0, dy = 0;
    if (lonCoords.size() > 1) {
      dx = std::abs(lonCoords[1] - lonCoords[0]) / 2.0;
    }
    if (latCoords.size() > 1) {
      dy = std::abs(latCoords[1] - latCoords[0]) / 2.0;
    }
    
    gisAttr["bbox"] = Rcpp::NumericVector::create(
      xmin - dx, ymin - dy, xmax + dx, ymax + dy
    );
  }
  
  // Attach gis attribute
  Rf_setAttrib(data, Rf_install("gis"), gisAttr);
  
  return data;
}
// [[Rcpp::export]]
Rcpp::List mdim_array_info(SEXP arr) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    return Rcpp::List();
  }
  
  // Get dimensions as R-friendly list
  Rcpp::List dims;
  Rcpp::List rawDims = pArr->getDimensions();
  Rcpp::IntegerVector shape(rawDims.size());
  Rcpp::CharacterVector dimNames(rawDims.size());
  
  for (R_xlen_t i = 0; i < rawDims.size(); ++i) {
    Rcpp::XPtr<GDALDimensionR> pDim(rawDims[i]);
    dimNames[i] = pDim->getName();
    shape[i] = static_cast<int>(pDim->getSize());
    
    dims.push_back(Rcpp::List::create(
        Rcpp::Named("name") = pDim->getName(),
        Rcpp::Named("size") = static_cast<double>(pDim->getSize()),
        Rcpp::Named("type") = pDim->getType(),
        Rcpp::Named("direction") = pDim->getDirection()
    ));
  }
  
  // Get data type info
  Rcpp::XPtr<GDALExtendedDataTypeR> pDT(pArr->getDataType());
  
  return Rcpp::List::create(
    Rcpp::Named("name") = pArr->getName(),
    Rcpp::Named("fullname") = pArr->getFullName(),
    Rcpp::Named("dims") = dims,
    Rcpp::Named("dim_names") = dimNames,
    Rcpp::Named("shape") = shape,
    Rcpp::Named("dtype") = pDT->getNumericDataTypeAsString(),
    Rcpp::Named("unit") = pArr->getUnit(),
    Rcpp::Named("nodata") = pArr->getNoDataValueAsDouble(),
    Rcpp::Named("scale") = pArr->getScale(),
    Rcpp::Named("offset") = pArr->getOffset(),
    Rcpp::Named("spatial_ref") = pArr->getSpatialRef()
  );
}

// [[Rcpp::export]]
Rcpp::CharacterVector mdim_array_attr_names(SEXP arr) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    return Rcpp::CharacterVector();
  }
  return pArr->getAttributeNames();
}

// [[Rcpp::export]]
SEXP mdim_array_attr(SEXP arr, std::string name) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    return R_NilValue;
  }
  
  SEXP attrPtr = pArr->getAttribute(name);
  if (Rf_isNull(attrPtr)) return R_NilValue;
  
  Rcpp::XPtr<GDALAttributeR> pAttr(attrPtr);
  
  // Get the data type to determine how to read
  SEXP dtPtr = pAttr->getDataType();
  if (Rf_isNull(dtPtr)) return R_NilValue;
  Rcpp::XPtr<GDALExtendedDataTypeR> pDT(dtPtr);
  
  int dtClass = pDT->getClass();  // 0=NUMERIC, 1=STRING, 2=COMPOUND
  GUInt64 nElems = pAttr->getTotalElementsCount();
  
  if (dtClass == 1) {  // GEDTC_STRING
    // String type - read as string
    if (nElems == 1) {
      return Rcpp::wrap(pAttr->readAsString());
    } else {
      return pAttr->readAsStringArray();
    }
  } else if (dtClass == 0) {  // GEDTC_NUMERIC
    // Numeric type
    if (nElems == 1) {
      return Rcpp::wrap(pAttr->readAsDouble());
    } else {
      return pAttr->readAsDoubleArray();
    }
  } else {
    // Compound or unknown - return as raw
    return pAttr->readAsRaw();
  }
}

// [[Rcpp::export]]
Rcpp::NumericVector mdim_dim_values(SEXP arr, int dim_index) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    Rcpp::stop("Invalid array");
  }
  
  Rcpp::List dims = pArr->getDimensions();
  if (dim_index < 0 || dim_index >= dims.size()) {
    Rcpp::stop("dim_index out of range (0 to %d)", dims.size() - 1);
  }
  
  Rcpp::XPtr<GDALDimensionR> pDim(dims[dim_index]);
  SEXP coordArr = pDim->getIndexingVariable();
  
  if (Rf_isNull(coordArr)) {
    // No coordinate variable - return sequence 0:(size-1)
    int n = static_cast<int>(pDim->getSize());
    Rcpp::NumericVector seq(n);
    for (int i = 0; i < n; ++i) seq[i] = i;
    return seq;
  }
  
  // Read the coordinate array
  Rcpp::XPtr<GDALMDArrayR> pCoord(coordArr);
  return Rcpp::as<Rcpp::NumericVector>(
    pCoord->read(Rcpp::NumericVector(), Rcpp::NumericVector(),
                 Rcpp::NumericVector(), Rcpp::NumericVector(), R_NilValue));
}

// ============================================================================
// Group-level attribute functions (Issue #3)
// ============================================================================

// [[Rcpp::export]]
Rcpp::CharacterVector mdim_group_attr_names(SEXP group) {
  Rcpp::XPtr<GDALGroupR> pGroup(group);
  if (!pGroup->isValid()) {
    Rcpp::stop("Invalid group");
  }
  return pGroup->getAttributeNames();
}

// [[Rcpp::export]]
SEXP mdim_group_attr(SEXP group, std::string name) {
  Rcpp::XPtr<GDALGroupR> pGroup(group);
  if (!pGroup->isValid()) {
    Rcpp::stop("Invalid group");
  }
  
  SEXP attrPtr = pGroup->getAttribute(name);
  if (Rf_isNull(attrPtr)) return R_NilValue;
  
  Rcpp::XPtr<GDALAttributeR> pAttr(attrPtr);
  
  // Get the data type to determine how to read
  SEXP dtPtr = pAttr->getDataType();
  if (Rf_isNull(dtPtr)) return R_NilValue;
  Rcpp::XPtr<GDALExtendedDataTypeR> pDT(dtPtr);
  
  int dtClass = pDT->getClass();  // 0=NUMERIC, 1=STRING, 2=COMPOUND
  GUInt64 nElems = pAttr->getTotalElementsCount();
  
  if (dtClass == 1) {  // GEDTC_STRING
    if (nElems == 1) {
      return Rcpp::wrap(pAttr->readAsString());
    } else {
      return pAttr->readAsStringArray();
    }
  } else if (dtClass == 0) {  // GEDTC_NUMERIC
    if (nElems == 1) {
      return Rcpp::wrap(pAttr->readAsDouble());
    } else {
      return pAttr->readAsDoubleArray();
    }
  } else {
    return pAttr->readAsRaw();
  }
}

// [[Rcpp::export]]
Rcpp::List mdim_group_attrs(SEXP group) {
  Rcpp::XPtr<GDALGroupR> pGroup(group);
  if (!pGroup->isValid()) {
    Rcpp::stop("Invalid group");
  }
  
  Rcpp::CharacterVector names = pGroup->getAttributeNames();
  
  Rcpp::List result;
  for (R_xlen_t i = 0; i < names.size(); ++i) {
    std::string attrName = Rcpp::as<std::string>(names[i]);
    SEXP val = mdim_group_attr(group, attrName);
    result[attrName] = val;
  }
  
  return result;
}

// ============================================================================
// Coordinate info helper (Wishlist W2: coordinate type detection)
// ============================================================================

// [[Rcpp::export]]
Rcpp::List mdim_coord_info(SEXP arr, int dim_index) {
  Rcpp::XPtr<GDALMDArrayR> pArr(arr);
  if (!pArr->isValid()) {
    Rcpp::stop("Invalid array");
  }
  
  Rcpp::List dims = pArr->getDimensions();
  if (dim_index < 0 || dim_index >= dims.size()) {
    Rcpp::stop("dim_index out of range (0 to %d)", dims.size() - 1);
  }
  
  Rcpp::XPtr<GDALDimensionR> pDim(dims[dim_index]);
  
  // Basic dimension info
  Rcpp::List result = Rcpp::List::create(
    Rcpp::Named("name") = pDim->getName(),
    Rcpp::Named("size") = static_cast<double>(pDim->getSize()),
    Rcpp::Named("type") = pDim->getType(),
    Rcpp::Named("direction") = pDim->getDirection(),
    Rcpp::Named("has_coord_var") = false,
    Rcpp::Named("units") = R_NilValue,
    Rcpp::Named("calendar") = R_NilValue
  );
  
  // Try to get the indexing variable (coordinate array)
  SEXP coordArr = pDim->getIndexingVariable();
  if (Rf_isNull(coordArr)) {
    return result;
  }
  
  result["has_coord_var"] = true;
  
  Rcpp::XPtr<GDALMDArrayR> pCoord(coordArr);
  
  // Get unit from the coordinate array
  std::string unit = pCoord->getUnit();
  if (!unit.empty()) {
    result["units"] = unit;
  }
  
  // Try to get calendar attribute (for CF time)
  SEXP calAttr = pCoord->getAttribute("calendar");
  if (!Rf_isNull(calAttr)) {
    Rcpp::XPtr<GDALAttributeR> pCalAttr(calAttr);
    SEXP dtPtr = pCalAttr->getDataType();
    if (!Rf_isNull(dtPtr)) {
      Rcpp::XPtr<GDALExtendedDataTypeR> pDT(dtPtr);
      if (pDT->getClass() == 1) {  // STRING
        result["calendar"] = pCalAttr->readAsString();
      }
    }
  }
  
  return result;
}