Rcpp::loadModule("mod_GDALMultiDimRaster", TRUE)


setMethod("show", "Rcpp_GDALMultiDimRaster", function(object) {
  cat("C++ object of class GDALMultiDimRaster\n",
      "  Driver: ", object$getDriverLongName()," (", object$getDriverShortName(), ")\n",
      "  DSN:    ", object$getDescription(band = 0), "\n",
      #"  Dimensions:   ", object$dim()[1], ", ", object$dim()[2], ", ", object$dim()[3], "\n",
      sep = ""
  )
})