# Public R wrappers for the internal functions defined in src/wk_trans.cpp
# Expose a GDAL coordinate transformation as a wk_trans object

#' Create a coordinate transformation object for use with wk
#'
#' `transform_create()` creates a reusable coordinate transformation object
#' that can be applied to any wk-handleable geometry with
#' [wk::wk_transform()] or [wk::wk_transform_filter()] (e.g., `wk::xy()`,
#' `wk::wkb()`, `wk::wkt()`, `wk::rct()`, `sf::st_sfc()`, `geos::geos_geometry()`).
#' Wrapper of `OGRCreateCoordinateTransformation()` in the GDAL Spatial
#' Reference System C++ API, returned as a `wk_trans` external pointer.
#'
#' @param srs_from Character string specifying the source spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param srs_to Character string specifying the target spatial reference
#' system. May be in WKT format or any of the formats supported by
#' [srs_to_wkt()].
#' @param use_z,use_m Used to declare the output type. Use `NA` to inherit
#' the dimensionality of the input, `TRUE` to force the output to have the
#' dimension, or `FALSE` to drop it (see [wk::wk_trans_explicit()]).
#' @param area_of_interest Optional numeric vector of length 4 giving the
#' area of interest as `c(west_lon, south_lat, east_lon, north_lat)` in
#' degrees, used to help select the most appropriate transformation.
#' @param coord_op Optional character string specifying the coordinate
#' operation to use, as a PROJ pipeline string or WKT representation of a
#' coordinate operation, overriding the default operation selected by PROJ.
#' @param coord_op_inverse Logical value, `TRUE` to use the inverse of
#' `coord_op`. Ignored if `coord_op` is `NULL`.
#' @param allow_ballpark Logical value, whether "ballpark" transformations
#' (those without a known accuracy) are allowed. `FALSE` requires GDAL >= 3.3.
#' @param accuracy Optional numeric value giving the desired accuracy in
#' metres, used to filter candidate transformations. Negative (the default)
#' means no filtering. Requires GDAL >= 3.3.
#' @returns An object of class `c("gdalraster_trans", "wk_trans")`, an
#' external pointer to a `wk_trans` object that can be passed as the `trans`
#' argument of [wk::wk_transform()].
#'
#' @details
#' A `gdalraster_trans` object holds an `OGRCoordinateTransformation` that
#' is released when the R object is garbage collected. The object is not
#' thread safe and should not be shared across threads.
#'
#' Only the x, y and (if present) z coordinates are passed to GDAL. The m
#' coordinate, if present, is passed through unchanged.
#'
#' [wk::wk_trans_inverse()] returns the inverse transformation (requires
#' GDAL >= 3.3).
#'
#' @note
#' `transform_create()` uses traditional GIS order for the input and output
#' xy (i.e., longitude/latitude ordered for geographic coordinates), as does
#' [transform_xy()].
#'
#' Input coordinates that contain missing values (`NA`) will be assigned `NA`
#' in the output. Coordinates that fail to transform with the GDAL API call
#' will also be assigned `NA` in the output, with a warning emitted once per
#' vector giving the number of failed points.
#'
#' @seealso
#' [transform_xy()], [transform_bounds()], [srs_to_wkt()],
#' [wk::wk_transform()], [wk::wk_trans_inverse()]
#' @examples
#' trans <- transform_create("EPSG:4326", "EPSG:3031")
#' trans
#'
#' pts <- wk::xy(c(147, 110, 0), c(-42, -66, -90))
#' wk::wk_transform(pts, trans)
#'
#' # any wk-handleable geometry
#' wkt <- wk::wkt("POLYGON ((140 -70, 150 -70, 150 -60, 140 -60, 140 -70))")
#' wk::wk_transform(wkt, trans)
#'
#' # the inverse
#' inv <- wk::wk_trans_inverse(trans)
#' wk::wk_transform(wk::wk_transform(pts, trans), inv)
#' @export
transform_create <- function(srs_from, srs_to, use_z = NA, use_m = NA,
                             area_of_interest = NULL, coord_op = NULL,
                             coord_op_inverse = FALSE, allow_ballpark = TRUE,
                             accuracy = -1) {

    if (missing(srs_from) || is.null(srs_from))
        stop("'srs_from' is required", call. = FALSE)
    if (!is.character(srs_from) || length(srs_from) > 1)
        stop("'srs_from' must be a character string", call. = FALSE)
    if (missing(srs_to) || is.null(srs_to))
        stop("'srs_to' is required", call. = FALSE)
    if (!is.character(srs_to) || length(srs_to) > 1)
        stop("'srs_to' must be a character string", call. = FALSE)

    if (!is.logical(use_z) || length(use_z) != 1)
        stop("'use_z' must be a single logical value or NA", call. = FALSE)
    if (!is.logical(use_m) || length(use_m) != 1)
        stop("'use_m' must be a single logical value or NA", call. = FALSE)

    if (!is.null(area_of_interest)) {
        if (!is.numeric(area_of_interest) || length(area_of_interest) != 4)
            stop("'area_of_interest' must be a numeric vector of length 4",
                 call. = FALSE)
    }

    if (!is.null(coord_op)) {
        if (!is.character(coord_op) || length(coord_op) != 1)
            stop("'coord_op' must be a character string", call. = FALSE)
    }

    if (!is.logical(coord_op_inverse) || length(coord_op_inverse) != 1 ||
        is.na(coord_op_inverse)) {
        stop("'coord_op_inverse' must be TRUE or FALSE", call. = FALSE)
    }

    if (!is.logical(allow_ballpark) || length(allow_ballpark) != 1 ||
        is.na(allow_ballpark)) {
        stop("'allow_ballpark' must be TRUE or FALSE", call. = FALSE)
    }

    if (!is.numeric(accuracy) || length(accuracy) != 1 || is.na(accuracy))
        stop("'accuracy' must be a single numeric value", call. = FALSE)

    # wk encodes use_z / use_m as NA_INTEGER, 0 or 1 in the wk_trans_t
    ptr <- .transform_create(srs_from, srs_to,
                             as.integer(use_z), as.integer(use_m),
                             area_of_interest, coord_op, coord_op_inverse,
                             allow_ballpark, as.numeric(accuracy))

    wk::new_wk_trans(ptr, "gdalraster_trans")
}

#' @export
#' @method wk_trans_inverse gdalraster_trans
wk_trans_inverse.gdalraster_trans <- function(trans, use_z = NA, use_m = NA,
                                              ...) {

    ptr <- .transform_inverse(trans, as.integer(use_z), as.integer(use_m))
    wk::new_wk_trans(ptr, "gdalraster_trans")
}

#' @export
#' @method format gdalraster_trans
format.gdalraster_trans <- function(x, ...) {
    info <- .transform_info(x)
    sprintf("<gdalraster_trans: %s => %s>", info[["srs_from"]],
            info[["srs_to"]])
}

#' @export
#' @method print gdalraster_trans
print.gdalraster_trans <- function(x, ...) {
    cat(format(x, ...), sep = "\n")
    invisible(x)
}
