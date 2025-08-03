# R interface to the GDAL CLI algorithms, convenience functions for using
# exposed class GDALAlg (see R/gdalalg.R and src/gdalalg.h)
# Chris Toney <chris.toney at gmail.com>

#' Convenience functions for using the \dQuote{gdal} CLI algorithms from R
#'
#' This set of functions can be used to access and run GDAL utilities as
#' \dQuote{gdal} Command Line Interface (CLI) algorithms.
#' **Requires GDAL >= 3.11.**
#'
#' @name gdal_cli
#' @details
#' These functions are convenient for accessing and running GDAL CLI algorithms
#' by way of the C++ exposed class \code{\link{GDALAlg}}. See the class
#' documentation for additional information.
#'
#' **Experimental**\cr
#' The GDAL Command Line Interface Modernization was first introduced in the
#' \href{https://github.com/OSGeo/gdal/releases/tag/v3.11.0}{GDAL 3.11.0 release}
#' (2025-05-09). The GDAL project states that the new CLI \dQuote{is
#' provisionally provided as an alternative interface to GDAL and OGR command
#' line utilities. The project reserves the right to modify, rename,
#' reorganize, and change the behavior until it is officially frozen via PSC
#' vote in a future major GDAL release.... Your usage of it should have no
#' expectation of compatibility until that time.}
#' Initial bindings to enable programmatic use of the new CLI algorithms from R
#' were added in \pkg{gdalraster} 2.2.0, and will evolve over future releases.
#' The bindings are considered experimental until the upstream API is declared
#' stable. Breaking changes in minor version releases are possible until then.
#'
#' `gdal_commands()` prints a list of commands and their descriptions to the
#' console, and returns (invisibly) a data frame with columns `command_string`,
#' `description`, `URL`. The `starts_with` argument can be used to filter the
#' output, e.g., `gdal_commands("vector")` to return only commands for working
#' with vector inputs.
#'
#' `gdal_run()` executes a GDAL algorithm and returns it as an object class
#' `GDALAlg`. A list containing algorithm output(s) can be accessed by calling
#' the \code{$outputs()} (plural) method on the returned object, or, more
#' conveniently in most cases, by calling \code{$output()} (singular) to return
#' the the single output value when there is only one. After assigning the
#' output, or otherwise completing work with the `GDALAlg` object, the
#' \code{$release()} method can be called to close datasets and free resources.
#'
#' `gdal_alg()` instantiates and returns an object of class `GDALAlg` without
#' running it. Passing argumengts to the requested CLI algorithm is optional.
#' This may be useful for obtaining command usage (\code{$usageAsJSON()}),
#' algorithm properties (\code{$info()}), or properties of algoritm arguments
#' (\code{$argInfo(arg_name)}). By default, parsing arguments will be attempted
#' if they are given, but this can be overriden by passing `parse = FALSE`.
#' The returned algorithm can then be executed (optionally) with its
#' \code{$run()} method.
#'
#' @param starts_with Optional character string for filtering output to certain
#' commands, e.g., `gdal_commnands("vsi")` to return only the Virtual System
#' Interface (VSI) commands, or `gdal_commnands("raster overview")` to return
#' only commands for managing overviews of a raster dataset.
#' @param cout Logical value, `TRUE` to print a list of commands along with
#' their descriptions and help URLS to the console (the de be executedfault).
#' @param cmd A character string or character vector containing the path to the
#' algorithm, e.g., `"raster reproject"` or `c("raster", "reproject")`.
#' @param args Either a character vector or a named list containing input
#' arguments of the algorithm (see section \dQuote{Argument syntax}).
#' @param parse Logical value, `TRUE` to attempt parsing `args` if they are
#' given in `gdal_alg()` (the default). Set to `FALSE` to instantiate the
#' algorithm without parsing arguments. The \code{$parseCommandLineArgs()}
#' method on the returned object can be called to parse arguments and obtain
#' the result of that, with potentially useful error messages.
#'
#' @section Argument syntax:
#' Arguments are given in R as a character vector or named list, but
#' otherwise syntax basically matches the GDAL specification for arguments as
#' they are given on the command line. Those specifications are listed here
#' along with some amendments regarding the character vector and named list
#' formats. Programmitc usage also allows passing and receiving datasets as
#' objects (i.e., `GDALRaster` or `GDALVector`), in addition to dataset names.
#'
#' * Commands accept one or several positional arguments, typically for dataset
#' names (or, in R, as `GDALRaster` or `GDALVector` datasets). The order is
#' input(s) first, output last. Positional arguments can also be specified as
#' named arguments, if preferred to avoid any ambiguity.
#' * Named arguments have:
#'   * at least one "long" name, preceded by two dash characters when specified
#'   on the command line,
#'   * optionally, auxiliary long names (i.e., aliases),
#'   * and optionally a one-letter short name, preceded by a single dash
#'   character on the command line, e.g., `-f, --of, --format, --output-format
#'   <OUTPUT-FORMAT>`
#' * Boolean arguments are specified by just specifying the argument name. In
#' R list format, the named element must be assigned a value of logical `TRUE`.
#' * Arguments that require a value are specified like:
#'   * `-f VALUE` for one-letter short names.
#'   * `--format VALUE` or `--format=VALUE` for long names.
#'   * in a named list, this would look like:\cr
#'   `arg_list$format <- VALUE`
#' * Some arguments can be multi-valued. Some of them require all values to be
#' packed together and separated with comma. This is, e.g., the case of:
#' `--bbox <BBOX> Clipping bounding box as xmin,ymin,xmax,ymax`, e.g.,
#' `--bbox=2.1,49.1,2.9,49.9`.
#' * Others accept each value to be preceded by a new mention of the argument
#' name, e.g., `--co COMPRESS=LZW --co TILED=YES`. For that one, if the value
#' of the argument does not contain commas, the packed form is also accepted:
#' `--co COMPRESS=LZW,TILED=YES`. Note that repeated mentions of an argument
#' are possible in the character vector format for argument input, whereas
#' arguments given in named list format must used argument long names as the
#' list element names, and the packed format for the values (which can be a
#' character vector or numeric vector of values).
#' * Named arguments can be placed before or after positional arguments.
#'
#' See the examples here, and also those for class \code{\link{GDALAlg}},
#' for variations in argument specification.
#'
#' @note
#'
#' @seealso
#' [`GDALAlg-class`][GDALAlg]
#'
#' \dQuote{gdal} Command Line Interface (CLI):\cr
#' \url{https://gdal.org/en/stable/programs/index.html}
#'
#' @examplesIf gdal_version_num() >= gdal_compute_version(3, 11, 0)
#' # print Virtual System Interface (VSI) commands and return in a data frame
#' vsi_df <- gdal_commands("vsi")
#'
#' head(vsi_df)
#'
#'
#' @export
gdal_commands <- function(starts_with = "", cout = TRUE) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 0))
        stop("gdal_commands() requires GDAL >= 3.11", call. = FALSE)

    if (missing(starts_with) || is.null(starts_with) || all(is.na(starts_with)))
        starts_with <- ""
    if (!(is.character(starts_with) && length(starts_with) == 1))
        stop("'starts_with' must be a length-1 character vector", call. = FALSE)

    if (missing(cout) || is.null(cout) || all(is.na(cout)))
        cout <- TRUE
    if (!(is.logical(cout) && length(cout) == 1))
        stop("'cout' must be a length-1 logical vector", call. = FALSE)

    return(invisible(.gdal_commands()))
}

#' @name gdal_cli
#' @export
gdal_run <- function(cmd, args = NULL) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 0))
        stop("gdal_run() requires GDAL >= 3.11", call. = FALSE)

    if (missing(cmd) || is.null(cmd) || all(is.na(cmd)))
        stop("'cmd' is required", call. = FALSE)
    if (!is.character(cmd))
        stop("'cmd' must be a character vector", call. = FALSE)

    if (!is.null(args)) {
        if (!is.character(args) && !is.list(args)) {
            stop("'args must be a character or named list", call. = FALSE)
        }
    }

    alg <- new(GDALAlg, cmd, args)

    if (!alg$parseCommandLineArguments()) {
        cat("parseCommandLineArguments() failed\n")
        alg$release()
        stop("failed to parse argument and set their values", call. = FALSE)
    }

    if (!alg$run()) {
        cat("run() failed\n")
        alg$release()
        stop("failed to execute the command", call. = FALSE)
    }

    return(alg)
}

#' @name gdal_cli
#' @export
gdal_alg <- function(cmd, args = NULL, parse = TRUE) {
    if (gdal_version_num() < gdal_compute_version(3, 11, 0))
        stop("gdal_alg() requires GDAL >= 3.11", call. = FALSE)

    if (missing(cmd) || is.null(cmd) || all(is.na(cmd)))
        stop("'cmd' is required", call. = FALSE)
    if (!is.character(cmd))
        stop("'cmd' must be a character vector", call. = FALSE)

    if (missing(parse) || is.null(parse) || all(is.na(parse)))
        parse <- TRUE
    if (!(is.logical(parse) && length(parse) == 1))
        stop("'parse' must be a length-1 logical vector", call. = FALSE)

    has_args <- FALSE
    if (!is.null(args)) {
        if (!is.character(args) && !is.list(args)) {
            stop("'args must be a character vector or named list",
                 call. = FALSE)
        } else {
            has_args <- TRUE
        }
    }

    alg <- new(GDALAlg, cmd, args)

    if (has_args && parse) {
        if (!alg$parseCommandLineArguments()) {
            cat("parseCommandLineArguments() failed\n")
            alg$release()
            stop("failed to parse arguments and set their values",
                 call. = FALSE)
        }
    }

    return(alg)
}
