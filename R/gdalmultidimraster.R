Rcpp::loadModule("mod_GDALMultiDimRaster", TRUE)


setMethod("show", "Rcpp_GDALMultiDimRaster", function(object) {
  extra <- ""
  nn <- 6L
  if (length(aa <- object$getArrayNames()) > nn) {
    extra <- sprintf("... first %i of %i, use $getArrayNames()", nn, length(aa))
  }
  dms <- lapply(aa, object$getDimensionNames)
  dnames <- object$getDimensionNames(aa[which.max(lengths(dms))])
  ## fixme, longest list of dimension is not necessarily ALL the dimensions (getDimensionNames() should work with no var name)
  cat("C++ object of class GDALMultiDimRaster\n",
      "     Driver: ", object$getDriverLongName()," (", object$getDriverShortName(), ")\n",
      "        DSN: ", object$getDescription(), "\n",
      "     Arrays: ", sprintf("%s %s", paste0(head(aa, nn), collapse = ", "), extra), "\n",
      " Dimensions: ", paste0(dnames, collapse = ", "), "\n",
      sep = ""
  )
})