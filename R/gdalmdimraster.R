#' @name GDALMDIMRaster-class
#'
#' @aliases
#' Rcpp_GDALMDIMRaster Rcpp_GDALMDIMRaster-class GDALMDIMRaster
#'
#' @title Class wrapping the GDAL Multidim API
#' @description
#' `GDALMDIMRaster` provides bindings to the GDAL Dataset API in MULTDIM mode.
#'
#'
#' @param dsn Character string containing the filename to open. It may be
#' a file in a regular local filesystem, or a filename with a GDAL /vsiPREFIX/
#' (see \url{https://gdal.org/user/virtual_file_systems.html}).
NULL


Rcpp::loadModule("mod_GDALMDIMRaster", TRUE)
