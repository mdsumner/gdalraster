# =============================================================================
# Test script for VRT source methods
# Run after building gdalraster with the new methods
# =============================================================================

library(gdalraster)

# -----------------------------------------------------------------------------
# Test 1: Basic VRT creation with addSimpleSource
# -----------------------------------------------------------------------------
test_simple_source <- function() {
  cat("Test 1: addSimpleSource()\n")
  
  # Use a sample raster from gdalraster
  src_file <- system.file("extdata/storml_elev.tif", package = "gdalraster")
  
  # Get source dimensions
  src_ds <- new(GDALRaster, src_file)
  src_xsize <- src_ds$getRasterXSize()
  src_ysize <- src_ds$getRasterYSize()
  src_gt <- src_ds$getGeoTransform()
  src_srs <- src_ds$getProjection()
  src_ds$close()
  
  # Create a VRT with same dimensions
  vrt_file <- tempfile(fileext = ".vrt")
  
  # Create empty VRT
  vrt <- create(
    vrt_file,
    format = "VRT",
    xsize = src_xsize,
    ysize = src_ysize,
    nbands = 0,
    dataType = "Int16"  # Matches source dtype
  )
  
  # Set geotransform and projection
  vrt$setGeoTransform(src_gt)
  vrt$setProjection(src_srs)
  

  # Add a band
  vrt$addBand("Int16")
  
  # Add source
  result <- vrt$addSimpleSource(
    band = 1,
    src_filename = src_file,
    src_band = 1,
    src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
    dst_xoff = -1, dst_yoff = -1, dst_xsize = -1, dst_ysize = -1,
    resampling = "near",
    nodata = NA
  )
  
  cat("  addSimpleSource returned:", result, "\n")
  
  # Get and print XML
  xml <- vrt$getVRTXML()
  cat("  VRT XML length:", nchar(xml), "chars\n")
  
  # Flush and close
  vrt$flushCache()
  vrt$close()
  
  # Re-open and test reading
  vrt2 <- new(GDALRaster, vrt_file)
  cat("  VRT dimensions:", vrt2$getRasterXSize(), "x", vrt2$getRasterYSize(), "\n")
  cat("  VRT bands:", vrt2$getRasterCount(), "\n")
  
  # Read a small window
  data <- vrt2$read(
    band = 1,
    xoff = 0, yoff = 0,
    xsize = 10, ysize = 10,
    out_xsize = 10, out_ysize = 10
  )
  cat("  Sample data range:", range(data), "\n")
  
  vrt2$close()
  unlink(vrt_file)
  
  cat("  PASSED\n\n")
}


# -----------------------------------------------------------------------------
# Test 2: VRT mosaic with multiple sources
# -----------------------------------------------------------------------------
test_mosaic <- function() {
  cat("Test 2: VRT mosaic with multiple sources\n")
  
  src_file <- system.file("extdata/storml_elev.tif", package = "gdalraster")
  
  src_ds <- new(GDALRaster, src_file)
  src_xsize <- src_ds$getRasterXSize()
  src_ysize <- src_ds$getRasterYSize()
  src_ds$close()
  
  # Create VRT double the width (side-by-side mosaic)
  vrt_file <- tempfile(fileext = ".vrt")
  
  vrt <- create(
    filename = vrt_file,
    driver = "VRT",
    xsize = src_xsize * 2,
    ysize = src_ysize,
    nbands = 0,
    dtype = "Int16"
  )
  
  vrt$addBand("Int16")
  
  # Left half
  vrt$addSimpleSource(
    band = 1,
    src_filename = src_file,
    src_band = 1,
    src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
    dst_xoff = 0, dst_yoff = 0, dst_xsize = src_xsize, dst_ysize = src_ysize,
    resampling = "near",
    nodata = NA
  )
  
  # Right half (same source, creates duplicate)
  vrt$addSimpleSource(
    band = 1,
    src_filename = src_file,
    src_band = 1,
    src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
    dst_xoff = src_xsize, dst_yoff = 0, dst_xsize = src_xsize, dst_ysize = src_ysize,
    resampling = "near",
    nodata = NA
  )
  
  vrt$flushCache()
  
  # Check XML has two SimpleSource elements
  xml <- vrt$getVRTXML()
  n_sources <- length(gregexpr("<SimpleSource>", xml)[[1]])
  cat("  Number of SimpleSource elements:", n_sources, "\n")
  
  vrt$close()
  
  # Verify dimensions
  vrt2 <- new(GDALRaster, vrt_file)
  cat("  VRT dimensions:", vrt2$getRasterXSize(), "x", vrt2$getRasterYSize(), "\n")
  stopifnot(vrt2$getRasterXSize() == src_xsize * 2)
  vrt2$close()
  
  unlink(vrt_file)
  cat("  PASSED\n\n")
}


# -----------------------------------------------------------------------------
# Test 3: addComplexSource with scaling
# -----------------------------------------------------------------------------
test_complex_source <- function() {
  cat("Test 3: addComplexSource() with scaling\n")
  
  src_file <- system.file("extdata/storml_elev.tif", package = "gdalraster")
  
  src_ds <- new(GDALRaster, src_file)
  src_xsize <- src_ds$getRasterXSize()
  src_ysize <- src_ds$getRasterYSize()
  src_ds$close()
  
  # Create VRT with Float32 output
  vrt_file <- tempfile(fileext = ".vrt")
  
  vrt <- create(
    filename = vrt_file,
    driver = "VRT",
    xsize = src_xsize,
    ysize = src_ysize,
    nbands = 0,
    dtype = "Float32"
  )
  
  vrt$addBand("Float32")
  
  # Add complex source with scaling: output = input * 0.001 + 100
  result <- vrt$addComplexSource(
    band = 1,
    src_filename = src_file,
    src_band = 1,
    src_xoff = -1, src_yoff = -1, src_xsize = -1, src_ysize = -1,
    dst_xoff = -1, dst_yoff = -1, dst_xsize = -1, dst_ysize = -1,
    scale_offset = 100,
    scale_ratio = 0.001,
    nodata = NA,
    color_table_component = 0
  )
  
  cat("  addComplexSource returned:", result, "\n")
  
  # Check XML has ComplexSource with scaling
  xml <- vrt$getVRTXML()
  has_complex <- grepl("<ComplexSource>", xml)
  has_scale <- grepl("<ScaleOffset>", xml) || grepl("<ScaleRatio>", xml)
  cat("  Has ComplexSource:", has_complex, "\n")
  cat("  Has scaling params:", has_scale, "\n")
  
  vrt$flushCache()
  vrt$close()
  
  # Read and verify scaling was applied
  vrt2 <- new(GDALRaster, vrt_file)
  orig <- new(GDALRaster, src_file)
  
  vrt_data <- vrt2$read(band = 1, xoff = 0, yoff = 0,
                         xsize = 5, ysize = 5,
                         out_xsize = 5, out_ysize = 5)
  orig_data <- orig$read(band = 1, xoff = 0, yoff = 0,
                          xsize = 5, ysize = 5,
                          out_xsize = 5, out_ysize = 5)
  
  expected <- orig_data * 0.001 + 100
  cat("  Original range:", range(orig_data), "\n")
  cat("  Scaled range:", range(vrt_data), "\n")
  cat("  Expected range:", range(expected), "\n")
  
  vrt2$close()
  orig$close()
  unlink(vrt_file)
  
  cat("  PASSED\n\n")
}


# -----------------------------------------------------------------------------
# Test 4: Error handling - non-VRT dataset
# -----------------------------------------------------------------------------
test_error_handling <- function() {
  cat("Test 4: Error handling for non-VRT datasets\n")
  
  src_file <- system.file("extdata/storml_elev.tif", package = "gdalraster")
  ds <- new(GDALRaster, src_file)
  
  # Should error because source is not a VRT
  error_caught <- tryCatch({
    ds$addSimpleSource(1, src_file, 1, -1, -1, -1, -1, -1, -1, -1, -1, "near", NA)
    FALSE
  }, error = function(e) {
    cat("  Expected error caught:", conditionMessage(e), "\n")
    TRUE
  })
  
  ds$close()
  
  stopifnot(error_caught)
  cat("  PASSED\n\n")
}


# -----------------------------------------------------------------------------
# Test 5: In-memory VRT (/vsimem/)
# -----------------------------------------------------------------------------
test_vsimem <- function() {
  cat("Test 5: In-memory VRT with /vsimem/\n")
  
  src_file <- system.file("extdata/storml_elev.tif", package = "gdalraster")
  
  src_ds <- new(GDALRaster, src_file)
  src_xsize <- src_ds$getRasterXSize()
  src_ysize <- src_ds$getRasterYSize()
  src_ds$close()
  
  # Create in-memory VRT
  vrt <- create(
    filename = "/vsimem/test_vrt.vrt",
    driver = "VRT",
    xsize = src_xsize,
    ysize = src_ysize,
    nbands = 0,
    dtype = "Int16"
  )
  
  vrt$addBand("Int16")
  vrt$addSimpleSource(1, src_file, 1, -1, -1, -1, -1, -1, -1, -1, -1, "near", NA)
  vrt$flushCache()
  
  # Get XML before closing
  xml <- vrt$getVRTXML()
  cat("  In-memory VRT XML length:", nchar(xml), "\n")
  
  vrt$close()
  
  # Verify we can re-open from /vsimem/
  vrt2 <- new(GDALRaster, "/vsimem/test_vrt.vrt")
  cat("  Re-opened dimensions:", vrt2$getRasterXSize(), "x", vrt2$getRasterYSize(), "\n")
  vrt2$close()
  
  # Clean up
  vsi_unlink("/vsimem/test_vrt.vrt")
  
  cat("  PASSED\n\n")
}


# -----------------------------------------------------------------------------
# Run all tests
# -----------------------------------------------------------------------------
main <- function() {
  cat("=== Testing VRT Source Methods ===\n\n")
  
  test_simple_source()
  test_mosaic()
  test_complex_source()
  test_error_handling()
  test_vsimem()
  
  cat("=== All tests passed! ===\n")
}

main()
