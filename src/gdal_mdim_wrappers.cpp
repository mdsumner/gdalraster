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

  if (!decode || Rf_isNull(result)) {
    return result;
  }

  // Apply CF decoding
  Rcpp::NumericVector nodata = pArr->getNoDataValueAsDouble();
  Rcpp::NumericVector scale = pArr->getScale();
  Rcpp::NumericVector offset = pArr->getOffset();

  bool hasNodata = nodata.size() > 0;
  bool hasScale = scale.size() > 0;
  bool hasOffset = offset.size() > 0;

  if (!hasNodata && !hasScale && !hasOffset) {
    return result;  // nothing to decode
  }

  // Convert to numeric if needed and apply transformations
  Rcpp::NumericVector data = Rcpp::as<Rcpp::NumericVector>(result);
  double dfNodata = hasNodata ? nodata[0] : NA_REAL;
  double dfScale = hasScale ? scale[0] : 1.0;
  double dfOffset = hasOffset ? offset[0] : 0.0;

  for (R_xlen_t i = 0; i < data.size(); ++i) {
    if (hasNodata && data[i] == dfNodata) {
      data[i] = NA_REAL;
    } else {
      data[i] = data[i] * dfScale + dfOffset;
    }
  }

  // Preserve dim attribute
  if (Rf_getAttrib(result, R_DimSymbol) != R_NilValue) {
    data.attr("dim") = Rf_getAttrib(result, R_DimSymbol);
  }

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

  // Return value based on type - try string first, then numeric
  GUInt64 nElems = pAttr->getTotalElementsCount();
  if (nElems == 1) {
    // Try as double first
    double val = pAttr->readAsDouble();
    if (!ISNA(val)) return Rcpp::wrap(val);
    // Fall back to string
    return Rcpp::wrap(pAttr->readAsString());
  } else {
    // Multiple elements - try as array
    Rcpp::NumericVector dblArr = pAttr->readAsDoubleArray();
    if (dblArr.size() > 0) return dblArr;
    return pAttr->readAsStringArray();
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
