# =============================================================================
# R Documentation for VRT Source Methods
# Add to R/gdalraster.R in the GDALRaster-class documentation
# =============================================================================

#' @section VRT Source Methods:
#' The following methods are only available for VRT format datasets.
#' They allow programmatic construction of VRT files by adding sources
#' to VRT raster bands.
#'
#' \describe{
#'
#' \item{\code{$addSimpleSource(band, src_filename, src_band,
#'   src_xoff, src_yoff, src_xsize, src_ysize,
#'   dst_xoff, dst_yoff, dst_xsize, dst_ysize,
#'   resampling, nodata)}}{
#' Add a simple source to a VRT raster band. A simple source reads pixels
#' from a source band and maps them to a destination window in the VRT band.
#' Use \code{-1} for offset/size parameters to use defaults (0 for offsets,
#' full size for dimensions).
#' \code{resampling} can be \code{"near"}, \code{"bilinear"}, \code{"cubic"},
#' \code{"cubicspline"}, \code{"lanczos"}, \code{"average"}, or \code{"mode"}.
#' \code{nodata} is the nodata value for the source (use \code{NA} for none).
#' Returns \code{TRUE} on success, \code{FALSE} on failure.
#' Only supported for VRT datasets.
#' }
#'
#' \item{\code{$addComplexSource(band, src_filename, src_band,
#'   src_xoff, src_yoff, src_xsize, src_ysize,
#'   dst_xoff, dst_yoff, dst_xsize, dst_ysize,
#'   scale_offset, scale_ratio, nodata, color_table_component)}}{
#' Add a complex source to a VRT raster band. A complex source is like a
#' simple source but supports linear scaling of values:
#' \code{output = (input * scale_ratio) + scale_offset}.
#' Set \code{color_table_component} to 1-4 to extract a component from a
#' paletted source (1=red, 2=green, 3=blue, 4=alpha), or 0 for no extraction.
#' Returns \code{TRUE} on success, \code{FALSE} on failure.
#' Only supported for VRT datasets.
#' }
#'
#' \item{\code{$getVRTXML()}}{
#' Serialize the VRT dataset to an XML string. Useful for debugging or for
#' embedding a VRT definition as a string. Only supported for VRT datasets.
#' Returns a character string containing the VRT XML.
#' }
#'
#' }
#'
#' @examples
#' \dontrun{
#' # Create a VRT that mosaics two rasters
#' vrt_file <- tempfile(fileext = ".vrt")
#' vrt <- create(vrt_file, format = "VRT",
#'               xsize = 512, ysize = 512, nbands = 0, dataType = "Byte", return_obj = TRUE)
#' vrt$addBand("Byte", options = NULL)
#'
#' # Add first source to left half
#' vrt$addSimpleSource(
#'   band = 1,
#'   src_filename = "left.tif",
#'   src_band = 1,
#'   src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
#'   dst_xoff = 0, dst_yoff = 0, dst_xsize = 256, dst_ysize = 512,
#'   resampling = "near",
#'   nodata = NA
#' )
#'
#' # Add second source to right half
#' vrt$addSimpleSource(
#'   band = 1,
#'   src_filename = "right.tif",
#'   src_band = 1,
#'   src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
#'   dst_xoff = 256, dst_yoff = 0, dst_xsize = 256, dst_ysize = 512,
#'   resampling = "near",
#'   nodata = NA
#' )
#'
#' # Check the XML
#' cat(vrt$getVRTXML())
#'
#' # Flush to disk
#' vrt$flushCache()
#' vrt$close()
#'
#'
#' # Create a VRT with scaled values (e.g., DN to reflectance)
#' vrt2 <- create("/vsimem/scaled.vrt", format = "VRT",
#'                xsize = 1000, ysize = 1000, nbands = 0, dataType = "Float32")
#' vrt2$addBand("Float32", options = NULL)
#'
#' # Scale: reflectance = DN * 0.0001
#' vrt2$addComplexSource(
#'   band = 1,
#'   src_filename = "landsat_b4.tif",
#'   src_band = 1,
#'   src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
#'   dst_xoff = -1, dst_yoff = -1, dst_xsize = -1, dst_ysize = -1,
#'   scale_offset = 0,
#'   scale_ratio = 0.0001,
#'   nodata = 0,
#'   color_table_component = 0
#' )
#'
#' vrt2$close()
#' }
