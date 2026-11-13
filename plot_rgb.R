# - 3 or 4 band integer or raw input, general numeric can be `stretch`ed optionally
# - will set alpha as a constant, if 3 bands b adding that, if 4 bands by replace the existing one in the data
# - adding to an existing plot
# - doesn't accept character input (and GDAL doesn't return that anyway)
# - doesn't do any colour mapping or colour models beyond basic RGB/A
#
# There's a real mix of missing functionality in R here, and a lot of external helpers (scales, farver, nara, ximage) that
# make it problematic to lean on. Setting alpha is a pain because you have to decode to do it in base, but scales::alpha is
# very handy (maybe I should port what that does).
#
# I'd also like GDAL to be able to return character hex, and potentially R's nativeRaster encoding - I think doing that adds
# more value than loading up plot functions in an R package. But, still an open explore for me - ximage doesn't have ability to set alpha, but does accept it as data, so I was caught without a good workaround here.
#
#

.stretch <- function(x, newlim) {
    ## internal function not for normal use
    #if (newlim[2] == newlim[1]) {stop("invalid 'newlim'")}
    rg <- range(x, na.rm = TRUE)
    (x - rg[1])/diff(rg) * diff(newlim) + newlim[1]
}
plot_rgb <- function(data, alpha = 1, interpolate = FALSE,
                     asp = 1,
                     axes=TRUE, main="",
                     xlab="x", ylab="y", xaxs="i", yaxs="i",
                     stretch = FALSE, add = FALSE, ...) {
    gis <- attr(data, "gis")
    dm <- gis$dim
    alpha <- alpha[1L]
    ## FIXME: what if we have non raw or non 0,255 data
    if (is.raw(data)) {
        ## as.raster won't take raw
        data <- as.integer(data)
    }
    ## rescale anything but raw to 0,255
    if (stretch) {
        data <- .stretch(data, c(0, 255))
    }
    if (anyNA(data)) {
        data[is.na(data)] <- 0
    }
    ## from here we might already have alpha
    r <- as.raster(aperm(array(data, dm)/255, c(2, 1, 3)))

    if (!is.na(alpha) && !is.null(alpha) && alpha < 1) {
        if (alpha > 0) {
            alphahex <- sprintf("%02x", as.integer(round(alpha * 255)))
            if (nchar(r[1]) == 7) {
                r <- sprintf("%s%s", r, alphahex)
            } else {
                r <- sprintf("%s%s", substr(r, 1, 7), alphahex)
            }
             dim(r) <- dm[2:1]
             class(r) <- "raster"

        }
    }
    xlim <- gis$bbox[c(1, 3)]
    ylim <- gis$bbox[c(2, 4)]
    if (!add) {
      graphics::plot.new()
      graphics::plot.window(xlim=xlim, ylim=ylim, asp=asp,
                          xaxs=xaxs, yaxs=yaxs, ...)
    }
    graphics::rasterImage(r, xlim[1], ylim[1], xlim[2], ylim[2],
                          interpolate=interpolate)

    if (axes) {
        graphics::title(main=main, xlab=xlab, ylab=ylab)
        graphics::axis(1)
        graphics::axis(2)
    } else {
        graphics::title(main=main)
    }
    invisible()

}
library(gdalraster)
dsn <- sprintf("GPKG:%s:%s", system.file("extdata/small_world_and_byte.gpkg", package = "gdalraster"), "small_world_and_byte")
ds <- new(GDALRaster, dsn)
# 3 bands
data <- read_ds(ds, as_raw = F, bands = 1:3)
ds$close()

maps::map(col = "firebrick", lwd = 4, xlim = c(0, 100))
plot_rgb(data, alpha = .5, add = T)

ds <- new(GDALRaster, dsn)
## 4 bands
data <- read_ds(ds, as_raw = F)
ds$close()

maps::map(col = "firebrick", lwd = 4, xlim = c(0, 100))
plot_rgb(data, alpha = .5, add = T)

# [1] "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2A_T55GDM_20250826T001245_L2A/TCI.tif"
# [2] "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2A_T55GDN_20250826T001245_L2A/TCI.tif"
# [3] "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2B_T55GDM_20250826T000223_L2A/TCI.tif"
# [4] "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2B_T55GDN_20250826T000223_L2A/TCI.tif"
dsn <- "/vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2C_T55GDM_20250824T001248_L2A/TCI.tif"
#dsn <- "/vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2C_T55GDN_20250824T001248_L2A/TCI.tif"
#"/vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2C_T55GDM_20250821T000645_L2A/TCI.tif"
#dsn <- "/vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2C_T55GDN_20250821T000645_L2A/TCI.tif"
#dsn <- "/vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2B_T55GDM_20250819T001343_L2A/TCI.tif"
#"https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2B_T55GDN_20250819T001343_L2A/TCI.tif"
# "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2B_T55GDM_20250816T000223_L2A/TCI.tif"
#"https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2B_T55GDN_20250816T000223_L2A/TCI.tif"
# "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DM/2025/8/S2C_T55GDM_20250814T001513_L2A/TCI.tif"
# "https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/DN/2025/8/S2C_T55GDN_20250814T001513_L2A/TCI.tif"

# a <- scene::scene(cbind(147.6, -42.8), dry_run = T)
# dsn <- sprintf("/vsicurl/%s", a$visual[3])

r_g_b <- sprintf("vrt:///vsicurl/https://e84-earth-search-sentinel-data.s3.us-west-2.amazonaws.com/sentinel-2-c1-l2a/55/G/EN/2025/8/S2B_T55GEN_20250816T000223_L2A/%s.tif?unscale=false",
        c("B04", "B03", "B02"))
gdalraster::buildVRT(dsn <- "/vsimem/sentinel2.vrt", r_g_b, cl_arg = c("-separate"))


ds <- new(GDALRaster, dsn)
data <- read_ds(ds, as_raw = F, xsize = 1098 * 2, ysize = 1098 * 2)
ds$close()

## error
#plot_rgb(data, stretch = F)

cbrt <- function(x) x^(1/3)
honcfun <- function(x) cbrt(x  * 0.6)
rescale_ <- function(x, to = c(-0.3, 4.5) * 255, out = c(0, 255), honc = FALSE) {

  from <- range(x, na.rm = TRUE)
  if (honc)  x <- honcfun(x)
  xout <- scales::rescale(x, to = to, from = from)
  clamp <- function(x) {
    x[x < out[1]] <- out[1]
    x[x > out[2]] <- out[2]
    x
  }
  xout <- clamp(xout)
  attributes(xout) <- attributes(x)
  xout
}


plot_rgb(rescale_(data, c(-.5, 15) * 255))
