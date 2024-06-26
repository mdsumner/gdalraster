% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{inv_geotransform}
\alias{inv_geotransform}
\title{Invert geotransform}
\usage{
inv_geotransform(gt)
}
\arguments{
\item{gt}{Numeric vector of length six containing the geotransform to
invert.}
}
\value{
Numeric vector of length six containing the inverted
geotransform. The output vector will contain NAs if the input geotransform
is uninvertable.
}
\description{
\code{inv_geotransform()} inverts a vector of geotransform coefficients. This
converts the equation from being:\cr
raster pixel/line (column/row) -> geospatial x/y coordinate\cr
to:\cr
geospatial x/y coordinate -> raster pixel/line (column/row)
}
\examples{
elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
ds <- new(GDALRaster, elev_file)
invgt <- ds$getGeoTransform() |> inv_geotransform()
ds$close()

ptX = 324181.7
ptY = 5103901.4

## for a point x, y in the spatial reference system of elev_file
## raster pixel (column number):
pixel <- floor(invgt[1] +
               invgt[2] * ptX +
               invgt[3] * ptY)

## raster line (row number):
line <- floor(invgt[4] +
              invgt[5] * ptX +
              invgt[6] * ptY)

## get_pixel_line() applies this conversion
}
\seealso{
\code{\link[=GDALRaster]{GDALRaster$getGeoTransform()}}, \code{\link[=get_pixel_line]{get_pixel_line()}}
}
