# GDAL Multidimensional Raster convenience functions
# User-facing wrappers around the module class / C++ exports
# These take dsn + array_name and handle open/close internally,
# following the pattern of mdim_as_classic(), mdim_info(), mdim_translate()
#
# The underlying C++ exports (mdim_array_read, mdim_array_info, etc.)
# remain available for programmatic use with module objects.

# helper: open dataset + array, with input validation
# returns list(ds, arr) — caller must close ds
.mdim_open_array <- function(dsn, array_name, open_options = NULL) {
    if (missing(dsn) || is.null(dsn) || all(is.na(dsn)))
        stop("'dsn' is required", call. = FALSE)
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a character string", call. = FALSE)
    
    if (missing(array_name) || is.null(array_name) || all(is.na(array_name)))
        stop("'array_name' is required", call. = FALSE)
    if (!(is.character(array_name) && length(array_name) == 1))
        stop("'array_name' must be a character string", call. = FALSE)
    
    oo <- if (is.null(open_options)) character(0) else open_options
    ds <- new(GDALMultiDimRaster, dsn, TRUE, oo, TRUE)
    arr <- ds$openArray(array_name, character(0))
    if (is.null(arr))
        stop("array '", array_name, "' not found in '", dsn, "'",
             call. = FALSE)
    
    list(ds = ds, arr = arr)
}

#' Read data from a multidimensional array
#'
#' `mdim_read()` reads data from a named array in a multidimensional raster
#' dataset, with optional CF convention decoding (nodata masking,
#' scale/offset). The returned vector has a `gis` attribute containing
#' dimension names, coordinate values, spatial reference, and bounding box.
#'
#' @param dsn Character string containing the data source name.
#' @param array_name Character string giving the name of the array to read.
#' @param start Optional numeric vector of 0-based start indices for each
#'   dimension. Default reads from the origin.
#' @param count Optional numeric vector of element counts for each
#'   dimension. Default reads all remaining elements.
#' @param step Optional numeric vector of step sizes for each dimension.
#'   Default step is 1.
#' @param decode Logical. `TRUE` (default) to apply CF convention decoding:
#'   nodata values are replaced with `NA` and scale/offset are applied.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A numeric, integer, or raw vector with a `gis` attribute
#'   containing a list with components: `type`, `dim`, `dim_names`,
#'   `coords`, `srs`, `datatype`, and optionally `nodata`, `scale`,
#'   `offset`, `bbox`.
#'
#' @note
#' Dimensions are returned in R column-major order (reversed from GDAL's
#' C row-major order). The first GDAL dimension (slowest varying) becomes
#' the last R dimension.
#'
#' @seealso
#' [mdim_info()], [mdim_as_classic()], [mdim_translate()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' ## read the full array
#' data <- mdim_read(f, "Band1")
#' str(data)
#' attr(data, "gis")
#'
#' ## subset read: first 10 rows
#' sub <- mdim_read(f, "Band1", start = c(0, 0), count = c(10, 20))
#' length(sub)
#'
#' ## read without CF decoding
#' raw_data <- mdim_read(f, "Band1", decode = FALSE)
#' @export
mdim_read <- function(dsn, array_name, start = NULL, count = NULL,
                      step = NULL, decode = TRUE, open_options = NULL) {
    
    h <- .mdim_open_array(dsn, array_name, open_options)
    on.exit(h$ds$close())
    
    mdim_array_read(h$arr, start, count, step, decode)
}


#' Get information about a multidimensional array
#'
#' `mdim_read_info()` returns metadata about a named array in a
#' multidimensional raster dataset, including dimension names, shape,
#' data type, CF convention parameters, and spatial reference.
#'
#' @param dsn Character string containing the data source name.
#' @param array_name Character string giving the name of the array.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A list with components: `name`, `fullname`, `dims` (list of
#'   dimension info), `dim_names`, `shape`, `dtype`, `unit`, `nodata`,
#'   `scale`, `offset`, `spatial_ref`.
#'
#' @seealso
#' [mdim_read()], [mdim_info()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' mdim_read_info(f, "Band1")
#' @export
mdim_read_info <- function(dsn, array_name, open_options = NULL) {
    
    h <- .mdim_open_array(dsn, array_name, open_options)
    on.exit(h$ds$close())
    
    mdim_array_info(h$arr)
}


#' Read coordinate values for a dimension
#'
#' `mdim_read_dim_values()` reads the coordinate (indexing) variable values
#' for a specified dimension of a multidimensional array. If the dimension
#' has no associated coordinate variable, a 0-based integer sequence is
#' returned.
#'
#' @param dsn Character string containing the data source name.
#' @param array_name Character string giving the name of the array.
#' @param dim_index Integer. The 0-based index of the dimension.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A numeric vector of coordinate values.
#'
#' @seealso
#' [mdim_read()], [mdim_read_coord_info()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' ## read x coordinates (dimension 1)
#' mdim_read_dim_values(f, "Band1", 1)
#' @export
mdim_read_dim_values <- function(dsn, array_name, dim_index,
                                 open_options = NULL) {
    
    h <- .mdim_open_array(dsn, array_name, open_options)
    on.exit(h$ds$close())
    
    if (missing(dim_index) || is.null(dim_index) || all(is.na(dim_index)))
        stop("'dim_index' is required", call. = FALSE)
    if (!(is.numeric(dim_index) && length(dim_index) == 1))
        stop("'dim_index' must be a numeric value (integer)", call. = FALSE)
    
    mdim_dim_values(h$arr, as.integer(dim_index))
}


#' Get coordinate metadata for a dimension
#'
#' `mdim_read_coord_info()` returns metadata about the coordinate
#' (indexing) variable associated with a dimension, including its name,
#' size, type, direction, units, and CF calendar attribute.
#'
#' @param dsn Character string containing the data source name.
#' @param array_name Character string giving the name of the array.
#' @param dim_index Integer. The 0-based index of the dimension.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A list with components: `name`, `size`, `type`, `direction`,
#'   `has_coord_var` (logical), `units`, `calendar`. The `units` and
#'   `calendar` components are `NULL` if not available.
#'
#' @seealso
#' [mdim_read_dim_values()], [mdim_read_info()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' mdim_read_coord_info(f, "Band1", 0)
#' mdim_read_coord_info(f, "Band1", 1)
#' @export
mdim_read_coord_info <- function(dsn, array_name, dim_index,
                                 open_options = NULL) {
    
    h <- .mdim_open_array(dsn, array_name, open_options)
    on.exit(h$ds$close())
    
    if (missing(dim_index) || is.null(dim_index) || all(is.na(dim_index)))
        stop("'dim_index' is required", call. = FALSE)
    if (!(is.numeric(dim_index) && length(dim_index) == 1))
        stop("'dim_index' must be a numeric value (integer)", call. = FALSE)
    
    mdim_coord_info(h$arr, as.integer(dim_index))
}


#' Read attributes from a multidimensional array
#'
#' `mdim_read_array_attrs()` reads all attributes from a named array,
#' returning a named list. Each attribute is auto-typed to the appropriate
#' R type (character, numeric, integer, or raw for compound types).
#'
#' @param dsn Character string containing the data source name.
#' @param array_name Character string giving the name of the array.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A named list of attribute values.
#'
#' @seealso
#' [mdim_read_group_attrs()], [mdim_read_info()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' mdim_read_array_attrs(f, "Band1")
#' @export
mdim_read_array_attrs <- function(dsn, array_name, open_options = NULL) {
    
    h <- .mdim_open_array(dsn, array_name, open_options)
    on.exit(h$ds$close())
    
    nms <- mdim_array_attr_names(h$arr)
    result <- vector("list", length(nms))
    names(result) <- nms
    for (nm in nms) {
        result[[nm]] <- mdim_array_attr(h$arr, nm)
    }
    result
}


#' Read attributes from the root group
#'
#' `mdim_read_group_attrs()` reads all attributes from the root group
#' (or a named subgroup) of a multidimensional raster dataset.
#'
#' @param dsn Character string containing the data source name.
#' @param group_name Optional character string giving the name of a
#'   subgroup. Default (`NULL`) reads attributes from the root group.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#'   specifying dataset open options.
#' @returns A named list of attribute values.
#'
#' @seealso
#' [mdim_read_array_attrs()], [mdim_info()]
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 1, 0) && isTRUE(gdal_formats("netCDF")$multidim_raster)
#' f <- system.file("extdata/byte.nc", package="gdalraster")
#'
#' mdim_read_group_attrs(f)
#' @export
mdim_read_group_attrs <- function(dsn, group_name = NULL,
                                  open_options = NULL) {
    
    if (missing(dsn) || is.null(dsn) || all(is.na(dsn)))
        stop("'dsn' is required", call. = FALSE)
    if (!(is.character(dsn) && length(dsn) == 1))
        stop("'dsn' must be a character string", call. = FALSE)
    
    oo <- if (is.null(open_options)) character(0) else open_options
    ds <- new(GDALMultiDimRaster, dsn, TRUE, oo, TRUE)
    on.exit(ds$close())
    
    grp <- ds$getRootGroup()
    if (is.null(grp))
        stop("failed to get root group from '", dsn, "'", call. = FALSE)
    
    if (!is.null(group_name) && nzchar(group_name)) {
        grp <- grp$openGroup(group_name, character(0))
        if (is.null(grp))
            stop("group '", group_name, "' not found", call. = FALSE)
    }
    
    mdim_group_attrs(grp)
}