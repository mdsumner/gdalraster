dsn_cop30 <- "/vsicurl/https://opentopography.s3.sdsc.edu/raster/COP30/COP30_hh.vrt"
dsn_image <- "WMTS:https://services.arcgisonline.com/arcgis/rest/services/World_Imagery/MapServer/WMTS/1.0.0/WMTSCapabilities.xml"
b <- 4000  ## radius around a point
zf <- 10   ## z factor for elevation exag
a <- .55   ## alpha for transparency of image overlay

## random, or specific city on earth
city <- "" ## or "Auckland"
if (!nzchar(city)) {
    pt <- maps::world.cities |>
        dplyr::sample_n(1) |>
        dplyr::select(long, lat, "name")
} else {
    px <- tidygeocoder::geo(city)
    pt <- data.frame(long = px$long, lat = px$lat, name = city)
}
library(gdalraster)

prj <- sprintf("+proj=laea +lon_0=%f +lat_0=%f +datum=WGS84", pt[1], pt[2])
xy <- transform_xy(pt[,1:2, drop = F], srs_to = prj, srs_from = "EPSG:4326")
bb <- c(-1, -1, 1, 1) * b

## warp to an elevation temp file
warp(dsn_cop30, elev_dsn <- tempfile(fileext = ".vrt", tmpdir = "/vsimem"),
     prj, cl_arg = c("-ts", 1024, 0, "-te", bb[1], bb[2], bb[3], bb[4], "-r", "bilinear"))


args <- list(input = ds <- new(GDALRaster, elev_dsn),
             output_format = "MEM",
             output = "", zfactor = as.character(zf))

alg <- gdal_run("raster hillshade", args)
(ds_hillshade <- alg$output())
alg$release()

warp(dsn_image, im_dsn <- tempfile(fileext = ".vrt", tmpdir = "/vsimem"),
     prj, cl_arg = c("-t_srs", prj-ts", 1024, 0, "-te", bb[1], bb[2], bb[3], bb[4], "-r", "bilinear"))

## add a band, and then write to an actual tif
im <- new(GDALRaster, im_dsn, read_only = FALSE)
im$addBand(im$getDataTypeName(1L), NULL)
translate(im, tf <- tempfile(fileext = ".tif"))
im$close()
im2 <- new(GDALRaster, tf, read_only = FALSE)

## put in a new alpha band
im2$fillRaster(4L, a*255, 0)
im2$close()

#plot_raster(im2)
op <- par(mfrow = c(2, 2), mar = c(0, .2, 0, 0))
plot_raster(ds_hillshade, axes = F)
title(sprintf("%s, hillshade", pt$name), line = -2)

plot_raster(ds_hillshade, axes = F)

title(pt$name, line = -2)
ximage::ximage(vapour::gdal_raster_image(tf, bands = 1:3), add = T)

plot_raster(ds_hillshade, axes = F)
title(sprintf("%s %s, hillshade*%s", pt$name, as.character(a), as.character(zf)), line = -2)
ximage::ximage(vapour::gdal_raster_image(tf), add = T)

ds_hillshade$close()

par(op)
