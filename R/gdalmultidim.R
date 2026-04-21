# GDAL Multidimensional Raster class
# Rcpp Module interface to GDAL Multidim Raster API

#' @name GDALMultiDimRaster-class
#'
#' @aliases
#' Rcpp_GDALMultiDimRaster Rcpp_GDALMultiDimRaster-class GDALMultiDimRaster
#'
#' @title Class encapsulating a GDAL multidimensional raster dataset
#'
#' @description
#' `GDALMultiDimRaster` provides an interface for accessing a dataset opened
#' with `GDAL_OF_MULTIDIM_RASTER`, and calling methods on the underlying
#' `GDALDataset`, `GDALGroup`, `GDALMDArray`, `GDALDimension`,
#' `GDALAttribute` and `GDALExtendedDataType` objects. See the GDAL
#' Multidimensional Raster Data Model:
#' \url{https://gdal.org/en/stable/user/multidim_raster_data_model.html}
#' and the C++ API:
#' \url{https://gdal.org/en/stable/api/gdalmdarray_cpp.html}.
#'
#' `GDALMultiDimRaster` is a C++ class exposed directly to \R (via
#' `RCPP_EXPOSED_CLASS`). Fields and methods of the class are accessed using
#' the `$` operator. The class provides access to the root group of a
#' multidimensional dataset, from which arrays, dimensions, attributes, and
#' subgroups can be navigated. Supporting objects returned by class methods
#' include `GDALGroupR`, `GDALMDArrayR`, `GDALDimensionR`, `GDALAttributeR`
#' and `GDALExtendedDataTypeR`, documented in the sections below.
#'
#' **Note that all arguments to class methods are required and must be given in
#' the order documented.** Naming the arguments is optional but may be preferred
#' for readability.
#'
#' @param filename Character string containing the file name of a
#' multidimensional raster dataset to open, as full path or relative to the
#' current working directory. May contain a URL, /vsiPREFIX/, etc. See GDAL
#' multidimensional raster driver descriptions.
#' @param read_only Logical. `TRUE` to open the dataset read-only (the default),
#' or `FALSE` to open with write access.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#' specifying dataset open options.
#' @param shared Logical. `FALSE` to open the dataset without using shared
#' mode. Default is `TRUE`.
#' @returns An object of class `GDALMultiDimRaster`, which contains a pointer
#' to the opened dataset. Class methods that operate on the dataset and its
#' contents are described in Details, along with a set of writable fields
#' for per-object settings. Values may be assigned to the class fields as
#' needed during the lifetime of the object (i.e., by regular `<-` or `=`
#' assignment).
#'
#' @section Usage (see Details):
#' ```
#' ## Constructors
#' # read-only by default:
#' ds <- new(GDALMultiDimRaster, filename)
#' # for update access:
#' ds <- new(GDALMultiDimRaster, filename, read_only = FALSE)
#' # to specify dataset open options:
#' ds <- new(GDALMultiDimRaster, filename, read_only, open_options)
#' # to open without using shared mode:
#' ds <- new(GDALMultiDimRaster, filename, read_only, open_options,
#' #          shared = FALSE)
#'
#' ## Read/write fields (per-object settings)
#' ds$filename
#' ds$readOnly
#' ds$quiet
#' ds$infoOptions
#'
#' ## Dataset methods
#' ds$open(read_only)
#' ds$close()
#' ds$isOpen()
#' ds$info(options)
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#' ds$getFileList()
#' ds$getRootGroup()
#' ds$flushCache()
#'
#' ## Metadata methods
#' ds$getMetadata(domain)
#' ds$getMetadataItem(name, domain)
#' ds$setMetadata(metadata, domain)
#' ds$setMetadataItem(name, value, domain)
#' ds$getMetadataDomainList()
#'
#' ## Convenience methods (root group shortcuts)
#' ds$getArrayNames()
#' ds$openArray(name, options)
#' ds$openArrayFromFullname(fullname, options)
#' ds$getSubGroupNames()
#' ds$openSubGroup(name, options)
#'
#' ## ---- GDALGroupR methods ----
#' grp$isValid()
#' grp$getName()
#' grp$getFullName()
#' grp$getGroupNames(options)
#' grp$openGroup(name, options)
#' grp$openGroupFromFullname(fullname, options)
#' grp$createGroup(name, options)
#' grp$deleteGroup(name, options)
#' grp$getMDArrayNames(options)
#' grp$openMDArray(name, options)
#' grp$openMDArrayFromFullname(fullname, options)
#' grp$createMDArray(name, dimensions, dataType, options)
#' grp$deleteMDArray(name, options)
#' grp$getDimensions(options)
#' grp$createDimension(name, type, direction, size, options)
#' grp$getAttributeNames(options)
#' grp$getAttribute(name)
#' grp$getAttributes(options)
#' grp$createAttribute(name, dimensions, dataType, options)
#' grp$deleteAttribute(name, options)
#' grp$getVectorLayerNames(options)
#' grp$getStructuralInfo()
#' grp$rename(newName)
#'
#' ## ---- GDALMDArrayR methods ----
#' arr$isValid()
#' arr$getName()
#' arr$getFullName()
#' arr$getTotalElementsCount()
#' arr$getDimensionCount()
#' arr$getDimensions()
#' arr$getDataType()
#' arr$getSpatialRef()
#' arr$setSpatialRef(wkt)
#' arr$getUnit()
#' arr$setUnit(unit)
#' arr$getNoDataValueAsDouble()
#' arr$getNoDataValueAsRaw()
#' arr$setNoDataValue(nodata)
#' arr$setNoDataValueRaw(nodata)
#' arr$deleteNoDataValue()
#' arr$getOffset()
#' arr$getScale()
#' arr$setOffset(offset)
#' arr$setScale(scale)
#' arr$getBlockSize()
#' arr$getProcessingChunkSize(maxChunkMemory)
#' arr$getStructuralInfo()
#' arr$getAttributeNames()
#' arr$getAttribute(name)
#' arr$getAttributes()
#' arr$createAttribute(name, dimensions, dataType, options)
#' arr$deleteAttribute(name, options)
#' arr$read(arrayStartIdx, count, arrayStep, bufferStride, bufferDataType)
#' arr$write(data, arrayStartIdx, count, arrayStep, bufferStride,
#' #         bufferDataType)
#' arr$adviseRead(arrayStartIdx, count, options)
#' arr$getView(viewExpr)
#' arr$transpose(mapNewAxisToOldAxis)
#' arr$getUnscaled()
#' arr$getMask(options)
#' arr$getResampled(newDims, resampleAlg, targetSRS, options)
#' arr$asClassicDataset(iXDim, iYDim, rootGroup, options)
#' arr$cache(options)
#' arr$computeStatistics(approxOK, force, options)
#' arr$resize(newDimSizes, options)
#' arr$rename(newName)
#'
#' ## ---- GDALDimensionR methods ----
#' dim$isValid()
#' dim$getName()
#' dim$getFullName()
#' dim$getType()
#' dim$getDirection()
#' dim$getSize()
#' dim$getIndexingVariable()
#' dim$setIndexingVariable(array)
#' dim$rename(newName)
#'
#' ## ---- GDALAttributeR methods ----
#' att$isValid()
#' att$getName()
#' att$getFullName()
#' att$getTotalElementsCount()
#' att$getDimensionCount()
#' att$getDimensionsSize()
#' att$getDataType()
#' att$readAsRaw()
#' att$readAsString()
#' att$readAsInt()
#' att$readAsDouble()
#' att$readAsStringArray()
#' att$readAsIntArray()
#' att$readAsDoubleArray()
#' att$write(data)
#' att$writeString(value)
#' att$writeInt(value)
#' att$writeDouble(value)
#' att$writeStringArray(values)
#' att$writeIntArray(values)
#' att$writeDoubleArray(values)
#' att$rename(newName)
#'
#' ## ---- GDALExtendedDataTypeR methods ----
#' edt$getName()
#' edt$getClass()
#' edt$getClassAsString()
#' edt$getNumericDataType()
#' edt$getNumericDataTypeAsString()
#' edt$getSize()
#' edt$getMaxStringLength()
#' edt$getComponents()
#' edt$canConvertTo(other)
#' edt$equals(other)
#'
#' ## ---- Stand-alone functions ----
#' getMultiDimDrivers()
#' edtCreate(dataType)
#' edtCreateString(maxLength)
#' edtCreateCompound(name, totalSize, components)
#' mdimCreate(filename, driverName, options)
#' mdim_array_read(arr, start, count, step, decode)
#' mdim_array_info(arr)
#' mdim_array_attr_names(arr)
#' mdim_array_attr(arr, name)
#' mdim_dim_values(arr, dim_index)
#' mdim_group_attr_names(group)
#' mdim_group_attr(group, name)
#' mdim_group_attrs(group)
#' mdim_coord_info(arr, dim_index)
#' ```
#'
#' @details
#'
#' ## GDALMultiDimRaster class
#'
#' ### Fields
#'
#' `$filename` (read-only)\cr
#' Character string. The source filename or connection string.
#'
#' `$readOnly` (read-only)\cr
#' Logical. `TRUE` if the dataset was opened in read-only mode.
#'
#' `$quiet` (read/write)\cr
#' Logical. Set to `TRUE` to suppress messages. Default is `FALSE`.
#'
#' `$infoOptions` (read/write)\cr
#' Character vector. Default options for the `$info()` method, given as
#' `gdalmdiminfo` options (e.g., `"-detailed"`, `"-limit 5"`).
#'
#' ### Constructor
#'
#' `new(GDALMultiDimRaster, filename, read_only, open_options, shared)`
#' opens a multidimensional raster dataset. `filename` is required;
#' remaining arguments have defaults described in the parameter
#' documentation. The dataset is opened with `GDAL_OF_MULTIDIM_RASTER`.
#'
#' ### Dataset methods
#'
#' `$open(read_only)`\cr
#' Re-open the dataset with the specified access mode. If the dataset is
#' already open, it is closed first. Returns (invisibly) on success.
#'
#' `$close()`\cr
#' Close the dataset, releasing the underlying GDAL handles.
#'
#' `$isOpen()`\cr
#' Returns logical `TRUE` if the dataset is currently open.
#'
#' `$info(options)`\cr
#' Returns a character string of JSON output describing the dataset
#' structure (equivalent to `gdalmdiminfo -json`). `options` is an optional
#' character vector of additional `gdalmdiminfo` options. The options stored
#' in `$infoOptions` are always included.
#'
#' `$getDriverShortName()`\cr
#' Returns the short name of the format driver (e.g., `"netCDF"`, `"HDF5"`,
#' `"Zarr"`).
#'
#' `$getDriverLongName()`\cr
#' Returns the long name of the format driver.
#'
#' `$getFileList()`\cr
#' Returns a character vector of files associated with the dataset.
#'
#' `$getRootGroup()`\cr
#' Returns a `GDALGroupR` object representing the root group of the dataset.
#' This is the primary entry point for navigating the dataset structure. See
#' section **GDALGroupR class** below.
#'
#' `$flushCache()`\cr
#' Flush any pending writes to disk.
#'
#' ### Metadata methods
#'
#' `$getMetadata(domain)`\cr
#' Returns a character vector of `NAME=VALUE` pairs from the specified
#' metadata `domain`. Pass `""` (default) for the default domain.
#'
#' `$getMetadataItem(name, domain)`\cr
#' Returns the value of metadata item `name` from the given `domain`.
#' Returns `""` if the item is not found.
#'
#' `$setMetadata(metadata, domain)`\cr
#' Sets metadata for the specified `domain`. `metadata` is a character
#' vector of `NAME=VALUE` pairs. Returns `TRUE` on success.
#'
#' `$setMetadataItem(name, value, domain)`\cr
#' Sets a single metadata item. Returns `TRUE` on success.
#'
#' `$getMetadataDomainList()`\cr
#' Returns a character vector of available metadata domain names.
#'
#' ### Convenience methods
#'
#' These provide shortcuts through the root group and are equivalent to
#' calling `ds$getRootGroup()$...()`.
#'
#' `$getArrayNames()`\cr
#' Returns a character vector of array names in the root group.
#'
#' `$openArray(name, options)`\cr
#' Opens a multidimensional array by name from the root group. Returns a
#' `GDALMDArrayR` object, or `NULL` if not found. `options` is an optional
#' character vector of driver-specific options.
#'
#' `$openArrayFromFullname(fullname, options)`\cr
#' Opens a multidimensional array by its full path (e.g., `"/group1/temp"`).
#' Returns a `GDALMDArrayR` object, or `NULL` if not found.
#'
#' `$getSubGroupNames()`\cr
#' Returns a character vector of subgroup names in the root group.
#'
#' `$openSubGroup(name, options)`\cr
#' Opens a subgroup by name from the root group. Returns a `GDALGroupR`
#' object, or `NULL` if not found.
#'
#' ## GDALGroupR class
#'
#' Objects of class `GDALGroupR` are obtained from `ds$getRootGroup()`,
#' `ds$openSubGroup()`, or `grp$openGroup()`. A group is a named container
#' of arrays, dimensions, attributes, and subgroups, following the HDF5 data
#' model.
#'
#' `$isValid()`\cr
#' Returns logical `TRUE` if the group handle is valid.
#'
#' `$getName()`\cr
#' Returns the group name (leaf name only).
#'
#' `$getFullName()`\cr
#' Returns the fully qualified group name (e.g., `"/group1/subgroup"`).
#'
#' ### Subgroups
#'
#' `$getGroupNames(options)`\cr
#' Returns a character vector of subgroup names.
#'
#' `$openGroup(name, options)`\cr
#' Opens a subgroup by name. Returns a `GDALGroupR` object.
#'
#' `$openGroupFromFullname(fullname, options)`\cr
#' Opens a subgroup by its full path. Returns a `GDALGroupR` object.
#'
#' `$createGroup(name, options)`\cr
#' Creates a new subgroup. Returns a `GDALGroupR` object. Requires write
#' access. Optionally supported by drivers.
#'
#' `$deleteGroup(name, options)`\cr
#' Deletes a subgroup. Returns `TRUE` on success. Optionally supported.
#'
#' ### Arrays
#'
#' `$getMDArrayNames(options)`\cr
#' Returns a character vector of array names in the group.
#'
#' `$openMDArray(name, options)`\cr
#' Opens a multidimensional array by name. Returns a `GDALMDArrayR` object.
#'
#' `$openMDArrayFromFullname(fullname, options)`\cr
#' Opens a multidimensional array by its full path. Returns a
#' `GDALMDArrayR` object.
#'
#' `$createMDArray(name, dimensions, dataType, options)`\cr
#' Creates a new array in the group. `dimensions` is a list of
#' `GDALDimensionR` objects. `dataType` is a `GDALExtendedDataTypeR` object.
#' Returns a `GDALMDArrayR` object. Requires write access.
#'
#' `$deleteMDArray(name, options)`\cr
#' Deletes an array. Returns `TRUE` on success. Optionally supported.
#'
#' ### Dimensions
#'
#' `$getDimensions(options)`\cr
#' Returns a list of `GDALDimensionR` objects defined at the group level.
#' Not all drivers implement group-level dimensions; see
#' `GDALMDArrayR$getDimensions()` for array-level dimensions.
#'
#' `$createDimension(name, type, direction, size, options)`\cr
#' Creates a new dimension. `type` is one of `"HORIZONTAL_X"`,
#' `"HORIZONTAL_Y"`, `"VERTICAL"`, `"TEMPORAL"`, or `""`. `direction` is
#' typically `"EAST"`, `"NORTH"`, `"UP"`, or `""`. `size` is the dimension
#' size. Returns a `GDALDimensionR` object.
#'
#' ### Attributes
#'
#' `$getAttributeNames(options)`\cr
#' Returns a character vector of attribute names.
#'
#' `$getAttribute(name)`\cr
#' Returns a `GDALAttributeR` object, or `NULL` if not found.
#'
#' `$getAttributes(options)`\cr
#' Returns a list of all `GDALAttributeR` objects.
#'
#' `$createAttribute(name, dimensions, dataType, options)`\cr
#' Creates a new attribute. `dimensions` is a numeric vector of dimension
#' sizes (empty for a scalar). `dataType` is a `GDALExtendedDataTypeR`
#' object. Returns a `GDALAttributeR` object.
#'
#' `$deleteAttribute(name, options)`\cr
#' Deletes an attribute. Returns `TRUE` on success.
#'
#' ### Other
#'
#' `$getVectorLayerNames(options)`\cr
#' Returns a character vector of vector layer names contained in the group,
#' if supported by the driver.
#'
#' `$getStructuralInfo()`\cr
#' Returns a character string with driver-specific structural information.
#'
#' `$rename(newName)`\cr
#' Renames the group. Returns `TRUE` on success. Optionally supported.
#'
#' ## GDALMDArrayR class
#'
#' Objects of class `GDALMDArrayR` are obtained from `ds$openArray()`,
#' `grp$openMDArray()`, or the various view/transform methods. An MDArray
#' is a named multidimensional array with associated dimensions, data type,
#' attributes, spatial reference, and optional CF convention properties
#' (nodata, scale, offset, unit).
#'
#' ### Properties
#'
#' `$isValid()`\cr
#' Returns logical `TRUE` if the array handle is valid.
#'
#' `$getName()`\cr
#' Returns the array name.
#'
#' `$getFullName()`\cr
#' Returns the fully qualified array name.
#'
#' `$getTotalElementsCount()`\cr
#' Returns the total number of elements in the array.
#'
#' `$getDimensionCount()`\cr
#' Returns the number of dimensions.
#'
#' `$getDimensions()`\cr
#' Returns a list of `GDALDimensionR` objects describing each dimension.
#'
#' `$getDataType()`\cr
#' Returns a `GDALExtendedDataTypeR` object describing the array data type.
#'
#' `$getSpatialRef()`\cr
#' Returns the spatial reference system as a WKT string, or `""` if not set.
#'
#' `$setSpatialRef(wkt)`\cr
#' Sets the spatial reference from a WKT string. Returns `TRUE` on success.
#'
#' `$getUnit()`\cr
#' Returns the unit string (e.g., `"m"`, `"K"`, `"days since 1970-01-01"`),
#' or `""` if not set.
#'
#' `$setUnit(unit)`\cr
#' Sets the unit string. Returns `TRUE` on success.
#'
#' `$getNoDataValueAsDouble()`\cr
#' Returns the nodata value as a length-1 numeric vector, or length-0 if
#' no nodata value is set.
#'
#' `$getNoDataValueAsRaw()`\cr
#' Returns the nodata value as raw bytes.
#'
#' `$setNoDataValue(nodata)`\cr
#' Sets the nodata value as a double. Returns `TRUE` on success.
#'
#' `$setNoDataValueRaw(nodata)`\cr
#' Sets the nodata value from a raw vector. Returns `TRUE` on success.
#'
#' `$deleteNoDataValue()`\cr
#' Deletes the nodata value. Returns `TRUE` on success.
#'
#' `$getOffset()`\cr
#' Returns the offset value as a length-1 numeric vector, or length-0 if
#' not set. Used with `$getScale()` for CF convention decoding:
#' `value = raw_value * scale + offset`.
#'
#' `$getScale()`\cr
#' Returns the scale value as a length-1 numeric vector, or length-0 if
#' not set.
#'
#' `$setOffset(offset)`\cr
#' Sets the offset value. Returns `TRUE` on success.
#'
#' `$setScale(scale)`\cr
#' Sets the scale value. Returns `TRUE` on success.
#'
#' `$getBlockSize()`\cr
#' Returns a numeric vector giving the block (chunk) size for each
#' dimension. A value of `0` indicates that the dimension is not chunked.
#'
#' `$getProcessingChunkSize(maxChunkMemory)`\cr
#' Returns a recommended chunk size for processing, given a memory budget
#' in bytes.
#'
#' `$getStructuralInfo()`\cr
#' Returns driver-specific structural information.
#'
#' ### Attributes
#'
#' `$getAttributeNames()`\cr
#' Returns a character vector of attribute names on the array.
#'
#' `$getAttribute(name)`\cr
#' Returns a `GDALAttributeR` object.
#'
#' `$getAttributes()`\cr
#' Returns a list of all `GDALAttributeR` objects.
#'
#' `$createAttribute(name, dimensions, dataType, options)`\cr
#' Creates a new attribute on the array.
#'
#' `$deleteAttribute(name, options)`\cr
#' Deletes an attribute. Returns `TRUE` on success.
#'
#' ### I/O
#'
#' `$read(arrayStartIdx, count, arrayStep, bufferStride, bufferDataType)`\cr
#' Read array data as an R vector or array. All parameters are optional.
#' `arrayStartIdx` is a numeric vector of 0-based start indices for each
#' dimension. `count` is a numeric vector of element counts per dimension
#' (default: read all remaining). `arrayStep` is a numeric vector of step
#' sizes per dimension (default: 1). `bufferStride` is a numeric vector of
#' buffer strides (default: `NULL` for C-style row-major). `bufferDataType`
#' is a `GDALExtendedDataTypeR` to request type conversion during read.
#'
#' For arrays with more than one dimension, the returned R object has a
#' `dim` attribute set. **Note:** GDAL uses row-major (C) dimension ordering,
#' while R uses column-major (Fortran) ordering. The `$read()` method
#' reverses the dimension order so that the result follows R conventions,
#' with the first GDAL dimension becoming the last R dimension.
#'
#' The return type depends on the data type: integer vector/array for
#' `Byte`, `Int8`, `Int16`, `UInt16`, `Int32`; numeric vector/array for
#' `Float32`, `Float64`, `UInt32`, `Int64`, `UInt64`; character
#' vector/array for string types; and raw vector for compound types (with
#' an `element_size` attribute).
#'
#' `$write(data, arrayStartIdx, count, arrayStep, bufferStride, bufferDataType)`\cr
#' Write data to the array. `data` is an R numeric, integer, character, or
#' raw vector. The remaining parameters have the same meaning as in
#' `$read()`. Requires write access. Returns `TRUE` on success.
#'
#' `$adviseRead(arrayStartIdx, count, options)`\cr
#' Notify the driver of an upcoming read region for optimization. Some
#' drivers (e.g., netCDF via DAP) use this to pre-cache data. Returns
#' `TRUE` on success.
#'
#' ### Views and transforms
#'
#' `$getView(viewExpr)`\cr
#' Returns a new `GDALMDArrayR` that is a view of this array using
#' NumPy-style basic slicing and indexing syntax. For example,
#' `"[0:10,...]"`, `"[:,::2]"`, or `"['field_name']"`. See
#' [mdim_as_classic()] for the full view expression syntax.
#' The returned array is a view (not a copy) and holds a reference to the
#' original.
#'
#' `$transpose(mapNewAxisToOldAxis)`\cr
#' Returns a transposed view of the array. `mapNewAxisToOldAxis` is an
#' integer vector mapping each new axis position to its original position.
#' For example, to swap the first two dimensions of a 3D array:
#' `c(1L, 0L, 2L)`.
#'
#' `$getUnscaled()`\cr
#' Returns a view with scale and offset applied (unscaled values). The
#' returned array applies `value = raw * scale + offset` on read.
#'
#' `$getMask(options)`\cr
#' Returns a mask array where valid values are `1` and invalid (nodata)
#' values are `0`.
#'
#' `$getResampled(newDims, resampleAlg, targetSRS, options)`\cr
#' Returns a resampled and/or reprojected view. `newDims` is a list of
#' `GDALDimensionR` objects (or `NULL` entries for unchanged dimensions).
#' `resampleAlg` is one of `"nearest"`, `"bilinear"`, `"cubic"`,
#' `"cubicspline"`, `"lanczos"`, `"average"`, `"rms"`, `"mode"`,
#' `"gauss"`. `targetSRS` is a WKT SRS string (empty for no reprojection).
#'
#' `$asClassicDataset(iXDim, iYDim, rootGroup, options)`\cr
#' Returns a classic 2D `GDALRaster` object. `iXDim` and `iYDim` are
#' 0-based dimension indices for the X and Y axes. For arrays with more
#' than 2 dimensions, additional dimensions are represented as raster bands.
#' `rootGroup` is an optional `GDALGroupR` object.
#' See also [mdim_as_classic()].
#'
#' ### Other
#'
#' `$cache(options)`\cr
#' Cache the array data (e.g., to speed up access to an expensive view such
#' as a transposed array). The cache is stored as a `.gmac` file alongside
#' the source. Returns `TRUE` on success.
#'
#' `$computeStatistics(approxOK, force, options)`\cr
#' Compute statistics. Returns a list with components `$min`, `$max`,
#' `$mean`, `$stddev`, `$valid_count`.
#'
#' `$resize(newDimSizes, options)`\cr
#' Resize the array to new dimension sizes. `newDimSizes` is a numeric
#' vector. Returns `TRUE` on success. Optionally supported.
#'
#' `$rename(newName)`\cr
#' Renames the array. Returns `TRUE` on success. Optionally supported.
#'
#' ## GDALDimensionR class
#'
#' Objects of class `GDALDimensionR` are obtained from
#' `arr$getDimensions()` or `grp$getDimensions()`. A dimension has a name,
#' size, type, and direction, and may have an associated indexing variable
#' (a 1D array of coordinate values).
#'
#' `$isValid()`\cr
#' Returns `TRUE` if the dimension handle is valid.
#'
#' `$getName()`\cr
#' Returns the dimension name.
#'
#' `$getFullName()`\cr
#' Returns the fully qualified dimension name.
#'
#' `$getType()`\cr
#' Returns the dimension type as a character string. Possible values include
#' `"HORIZONTAL_X"`, `"HORIZONTAL_Y"`, `"VERTICAL"`, `"TEMPORAL"`,
#' `"PARAMETRIC"`, or `""` (unknown).
#'
#' `$getDirection()`\cr
#' Returns the dimension direction (e.g., `"EAST"`, `"NORTH"`, `"UP"`,
#' or `""`).
#'
#' `$getSize()`\cr
#' Returns the dimension size as a numeric value.
#'
#' `$getIndexingVariable()`\cr
#' Returns the indexing (coordinate) variable as a `GDALMDArrayR` object,
#' or `NULL` if no indexing variable is associated. The indexing variable
#' is a 1D array containing the coordinate values along this dimension.
#'
#' `$setIndexingVariable(array)`\cr
#' Sets the indexing variable. `array` is a `GDALMDArrayR` object. Returns
#' `TRUE` on success. Optionally supported.
#'
#' `$rename(newName)`\cr
#' Renames the dimension. Returns `TRUE` on success. Optionally supported.
#'
#' ## GDALAttributeR class
#'
#' Objects of class `GDALAttributeR` are obtained from
#' `arr$getAttribute()`, `grp$getAttribute()`, or their `$getAttributes()`
#' variants. An attribute has a name and a typed value, used for metadata
#' on arrays and groups (e.g., CF convention attributes like `"units"`,
#' `"calendar"`, `"_FillValue"`).
#'
#' `$isValid()`\cr
#' Returns `TRUE` if the attribute handle is valid.
#'
#' `$getName()`\cr
#' Returns the attribute name.
#'
#' `$getFullName()`\cr
#' Returns the fully qualified attribute name.
#'
#' `$getTotalElementsCount()`\cr
#' Returns the total number of elements.
#'
#' `$getDimensionCount()`\cr
#' Returns the number of dimensions (0 for scalar, 1 for vector).
#'
#' `$getDimensionsSize()`\cr
#' Returns a numeric vector of dimension sizes.
#'
#' `$getDataType()`\cr
#' Returns a `GDALExtendedDataTypeR` object.
#'
#' ### Read methods
#'
#' `$readAsRaw()`\cr
#' Read the value as a raw vector.
#'
#' `$readAsString()`\cr
#' Read a scalar string value.
#'
#' `$readAsInt()`\cr
#' Read a scalar integer value.
#'
#' `$readAsDouble()`\cr
#' Read a scalar numeric value.
#'
#' `$readAsStringArray()`\cr
#' Read the value as a character vector.
#'
#' `$readAsIntArray()`\cr
#' Read the value as an integer vector.
#'
#' `$readAsDoubleArray()`\cr
#' Read the value as a numeric vector.
#'
#' ### Write methods
#'
#' `$write(data)`\cr
#' Write raw bytes. `data` is a raw vector.
#'
#' `$writeString(value)`, `$writeInt(value)`, `$writeDouble(value)`\cr
#' Write a scalar value.
#'
#' `$writeStringArray(values)`, `$writeIntArray(values)`,
#' `$writeDoubleArray(values)`\cr
#' Write a vector value.
#'
#' `$rename(newName)`\cr
#' Renames the attribute. Returns `TRUE` on success.
#'
#' ## GDALExtendedDataTypeR class
#'
#' Objects of class `GDALExtendedDataTypeR` are obtained from
#' `arr$getDataType()` or `att$getDataType()`. Instances can also be
#' created using the factory functions `edtCreate()`, `edtCreateString()`,
#' and `edtCreateCompound()`. An extended data type describes the type of
#' individual elements in an array or attribute, and may be numeric, string,
#' or compound.
#'
#' `$getName()`\cr
#' Returns the type name (e.g., `"Float64"`, `"String"`).
#'
#' `$getClass()`\cr
#' Returns the type class as an integer: `0` for `NUMERIC`, `1` for
#' `STRING`, `2` for `COMPOUND`.
#'
#' `$getClassAsString()`\cr
#' Returns the type class as a character string.
#'
#' `$getNumericDataType()`\cr
#' For numeric types, returns the `GDALDataType` integer code.
#'
#' `$getNumericDataTypeAsString()`\cr
#' For numeric types, returns the data type name (e.g., `"Float64"`,
#' `"Int16"`).
#'
#' `$getSize()`\cr
#' Returns the element size in bytes.
#'
#' `$getMaxStringLength()`\cr
#' For string types, returns the maximum string length (0 = unlimited).
#'
#' `$getComponents()`\cr
#' For compound types, returns a list of component descriptions.
#'
#' `$canConvertTo(other)`\cr
#' Returns `TRUE` if this type can be converted to `other` (a
#' `GDALExtendedDataTypeR` object).
#'
#' `$equals(other)`\cr
#' Returns `TRUE` if this type is equal to `other`.
#'
#' ## Stand-alone functions
#'
#' `getMultiDimDrivers()`\cr
#' Returns a character vector of GDAL driver short names that support
#' multidimensional raster.
#'
#' `edtCreate(dataType)`\cr
#' Create a `GDALExtendedDataTypeR` from a `GDALDataType` integer code
#' (e.g., `6L` for `GDT_Float32`). See [data_type_helpers].
#'
#' `edtCreateString(maxLength)`\cr
#' Create a string extended data type. `maxLength` of `0` (default) means
#' unlimited.
#'
#' `edtCreateCompound(name, totalSize, components)`\cr
#' Create a compound extended data type.
#'
#' `mdimCreate(filename, driverName, options)`\cr
#' Create a new multidimensional dataset. Returns a `GDALMultiDimRaster`
#' external pointer.
#'
#' `mdim_array_read(arr, start, count, step, decode)`\cr
#' Read array data with optional CF decoding (nodata masking, scale/offset).
#' `arr` is a `GDALMDArrayR` external pointer. `start`, `count`, `step` are
#' optional numeric vectors (0-based). If `decode` is `TRUE` (default),
#' nodata values are replaced with `NA` and scale/offset are applied.
#'
#' `mdim_array_info(arr)`\cr
#' Returns a list describing the array: `name`, `fullname`, `dims`,
#' `dim_names`, `shape`, `dtype`, `unit`, `nodata`, `scale`, `offset`,
#' `spatial_ref`.
#'
#' `mdim_array_attr_names(arr)`\cr
#' Returns attribute names on the array.
#'
#' `mdim_array_attr(arr, name)`\cr
#' Read an array attribute value, auto-detecting the type.
#'
#' `mdim_dim_values(arr, dim_index)`\cr
#' Read the coordinate values for dimension `dim_index` (0-based).
#' If no indexing variable exists, returns a 0-based integer sequence.
#'
#' `mdim_group_attr_names(group)`, `mdim_group_attr(group, name)`,
#' `mdim_group_attrs(group)`\cr
#' Attribute access on a `GDALGroupR` object.
#'
#' `mdim_coord_info(arr, dim_index)`\cr
#' Returns coordinate metadata for dimension `dim_index` (0-based):
#' `name`, `size`, `type`, `direction`, `has_coord_var`, `units`,
#' `calendar`.
#'
#' @note
#' Requires GDAL >= 3.1.0 for multidimensional raster support. Some
#' methods require later GDAL versions (e.g., `$getResampled()` requires
#' GDAL >= 3.4). Methods that are optionally supported by drivers (e.g.,
#' `$createGroup()`, `$resize()`, `$rename()`) may return `FALSE` or raise
#' an error if the driver does not support the operation.
#'
#' The main drivers supporting the multidimensional API are `netCDF`,
#' `HDF5`, `Zarr`, `MEM`, `TileDB`, and `VRT`. Use
#' `getMultiDimDrivers()` to list available drivers on the current system.
#'
#' The `$read()` method returns data with dimensions reversed relative to
#' GDAL's C ordering, to match R's column-major array convention. The
#' first GDAL dimension (slowest varying) becomes the last R dimension.
#'
#' The following GDAL Multidim API features are not yet exposed and may be
#' added in future versions:
#' `GDALMDArray::GetCoordinateVariables()` (retrieve coordinate variables),
#' `GDALMDArray::IsRegularlySpaced()` (test for regular spacing),
#' `GDALMDArray::GuessGeoTransform()` (infer geotransform from dimensions),
#' `GDALMDArray::GetStatistics()` (read cached statistics),
#' `GDALMDArray::GetGridded()` (on-the-fly gridding, GDAL >= 3.4),
#' `GDALMDArray::GetMeshGrid()` (static, GDAL >= 3.12),
#' `GDALGroup::CopyFrom()`,
#' `GDALGroup::SubsetDimensionFromSelection()` (GDAL >= 3.4).
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [mdim_as_classic()], [mdim_info()],
#' [mdim_translate()], [data_type_helpers]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' ## Open a multidimensional dataset
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#' ds <- new(GDALMultiDimRaster, f, TRUE, character(0), TRUE)
#'
#' ds$getDriverShortName()
#' ds$getArrayNames()
#'
#' ## Open an array and inspect
#' arr <- ds$openArray("Band1", character(0))
#' arr$getName()
#' arr$getDimensionCount()
#' arr$getDataType()$getNumericDataTypeAsString()
#'
#' ## Read dimension info
#' dims <- arr$getDimensions()
#' 
#' ## Read data
#' data <- arr$read(1L)
#' str(data)
#'
#' ## Subset read: first 10 elements along first dimension
#' sub <- arr$read(arrayStartIdx = c(0, 0), count = c(10, 20))
#' dim(sub)
#'
#' ## View expression
#' view <- arr$getView("[0:10,...]")
#' view$read()
#'
#' ## Using convenience wrapper with CF decoding
#' data2 <- mdim_array_read(ds$openArray("Band1"))
#'
#' ## Array info as a list
#' mdim_array_info(ds$openArray("Band1"))
#'
#' ## Coordinate info for a dimension
#' mdim_coord_info(ds$openArray("Band1"), 0)
#'
#' ## Convert to classic 2D dataset
#' classic <- arr$asClassicDataset(1, 0)
#' # 'classic' is a GDALRaster object:
#' classic$dim()
#'
#' ds$close()
#' @export
Rcpp::loadModule("mod_GDALMultiDimRaster", TRUE)
