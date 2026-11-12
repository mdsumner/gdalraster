/* Expose an OGRCoordinateTransformation as a wk_trans object, so that any
 wk-handleable geometry can be transformed with wk::wk_transform() using
 GDAL/PROJ via gdalraster, with no dependency on the PROJ R package.
 
 The pattern follows GDALVector::getArrowStream() in gdalvector.cpp:
 a C struct of callbacks defined by another package's C header is filled
 in here, and the consuming package (wk) owns the external pointer and
 its finalizer contract.
 
 Requires `LinkingTo: wk` in DESCRIPTION (the package already has
 `Imports: wk`). No change to Makevars.
 
 Public R wrappers in R/wk_trans.R.
 Copyright (c) 2023-2025 gdalraster authors
 */

// Rcpp.h first: it defines R_NO_REMAP before pulling in Rinternals.h, and
// wk-v1.h includes Rinternals.h directly.
#include <Rcpp.h>

#include <ogr_core.h>
#include <ogr_srs_api.h>
#include <ogr_spatialref.h>

#include <wk-v1.h>

// Only the wk_trans_t struct definition from wk-v1.h is needed. The helper
// implementations in wk-v1-impl.c (wk_trans_create() etc.) are plain C and
// do not compile as C++, so the equivalent few lines are done here: allocate
// the struct, set the defaults wk expects (api_version 1001, NA use_z/use_m,
// unbounded output), and register a finalizer on the external pointer.
// wk::wk_transform() only requires an EXTPTRSXP whose address is a
// wk_trans_t with these fields set.

#include <cstdlib>
#include <memory>
#include <string>

#include "gdalraster.h"
#include "srs_api.h"

namespace {

// trans_data payload: owns the OGRCoordinateTransformation
struct GDALWkTrans {
    OGRCoordinateTransformation *poCT = nullptr;
    R_xlen_t num_failed = 0;
    // the SRS definitions as given by the user, for display only
    std::string srs_from{};
    std::string srs_to{};
};
    
    // wk_trans_t::trans
    // Called once per coordinate. xyzm_in[2] / xyzm_in[3] are NaN when the
    // input geometry has no z / m (see wk_trans_filter_coord() in wk).
    // Only x, y and (if present) z are passed to GDAL; m is passed through
    // unchanged.
    int gdal_wk_trans_trans(R_xlen_t feature_id, const double *xyzm_in,
                            double *xyzm_out, void *trans_data) {
        
        (void) feature_id;
        auto data = static_cast<GDALWkTrans *>(trans_data);
        
        double x = xyzm_in[0];
        double y = xyzm_in[1];
        double z = xyzm_in[2];
        const bool has_z = !ISNAN(z);
        
        xyzm_out[3] = xyzm_in[3];
        
        // follows transform_xy(): NA in -> NA out, counted separately from
        // failed transforms (which are also NA out, with a warning at the end
        // of the vector)
        if (ISNAN(x) || ISNAN(y)) {
            xyzm_out[0] = NA_REAL;
            xyzm_out[1] = NA_REAL;
            xyzm_out[2] = has_z ? NA_REAL : z;
            return WK_CONTINUE;
        }
        
        int success = FALSE;
        data->poCT->Transform(1, &x, &y, has_z ? &z : nullptr, nullptr,
                              &success);
        
        if (success) {
            xyzm_out[0] = x;
            xyzm_out[1] = y;
            xyzm_out[2] = z;
        }
        else {
            data->num_failed += 1;
            xyzm_out[0] = NA_REAL;
            xyzm_out[1] = NA_REAL;
            xyzm_out[2] = has_z ? NA_REAL : z;
        }
        
        return WK_CONTINUE;
    }
    
    // wk_trans_t::vector_end
    // Emit the per-vector failure count as a warning, matching transform_xy()
    void gdal_wk_trans_vector_end(void *trans_data) {
        auto data = static_cast<GDALWkTrans *>(trans_data);
        if (data->num_failed > 0) {
            Rcpp::warning(std::to_string(data->num_failed) +
                " point(s) failed to transform, NA returned in that case");
            data->num_failed = 0;
        }
    }
    
    // wk_trans_t::finalizer
    // Called from wk_trans_destroy() via the external pointer finalizer
    void gdal_wk_trans_finalizer(void *trans_data) {
        auto data = static_cast<GDALWkTrans *>(trans_data);
        if (data == nullptr)
            return;
        if (data->poCT != nullptr)
            OGRCoordinateTransformation::DestroyCT(data->poCT);
        delete data;
    }
    
    // external pointer finalizer: equivalent of wk_trans_destroy_xptr() in
    // wk-v1-impl.c
    void gdal_wk_trans_destroy_xptr(SEXP xptr) {
        auto trans = static_cast<wk_trans_t *>(R_ExternalPtrAddr(xptr));
        if (trans == nullptr)
            return;
        if (trans->finalizer != nullptr)
            trans->finalizer(trans->trans_data);
        std::free(trans);
        R_ClearExternalPtr(xptr);
    }
    
    // wrap an owned OGRCoordinateTransformation* as a wk_trans external pointer
    SEXP make_wk_trans_xptr(OGRCoordinateTransformation *poCT, int use_z,
                            int use_m, const std::string &srs_from,
                            const std::string &srs_to) {
        
        auto trans = static_cast<wk_trans_t *>(std::malloc(sizeof(wk_trans_t)));
        if (trans == nullptr) {
            OGRCoordinateTransformation::DestroyCT(poCT);
            Rcpp::stop("failed to allocate wk_trans_t");
        }
        
        // defaults as set by wk_trans_create() in wk-v1-impl.c
        trans->api_version = 1001;
        trans->use_z = use_z;
        trans->use_m = use_m;
        for (int i = 0; i < 4; ++i) {
            trans->xyzm_out_min[i] = R_NegInf;
            trans->xyzm_out_max[i] = R_PosInf;
        }
        
        auto data = new GDALWkTrans();
        data->poCT = poCT;
        data->srs_from = srs_from;
        data->srs_to = srs_to;
        
        trans->trans_data = data;
        trans->trans = &gdal_wk_trans_trans;
        trans->vector_end = &gdal_wk_trans_vector_end;
        trans->finalizer = &gdal_wk_trans_finalizer;
        
        SEXP xptr = PROTECT(R_MakeExternalPtr(trans, R_NilValue, R_NilValue));
        // two-argument form: GDAL's cpl_port.h redefines FALSE as int 0, which
        // does not convert to Rboolean under C++
        R_RegisterCFinalizer(xptr, &gdal_wk_trans_destroy_xptr);
        UNPROTECT(1);
        return xptr;
    }
    
    GDALWkTrans *get_wk_trans_data(const Rcpp::RObject &trans) {
        if (TYPEOF(trans) != EXTPTRSXP)
            Rcpp::stop("'trans' must be an external pointer to a wk_trans");
        
        auto wk_trans = static_cast<wk_trans_t *>(R_ExternalPtrAddr(trans));
        if (wk_trans == nullptr || wk_trans->trans != &gdal_wk_trans_trans)
            Rcpp::stop("'trans' was not created by gdalraster");
        
        return static_cast<GDALWkTrans *>(wk_trans->trans_data);
    }
    
}  // namespace

//' Create a wk_trans object for a GDAL coordinate transformation
 //'
 //' public wrapper in R/wk_trans.R
 //' @noRd
 // [[Rcpp::export(name = ".transform_create")]]
 SEXP transform_create(const std::string &srs_from,
                       const std::string &srs_to,
                       int use_z,
                       int use_m,
                       const Rcpp::Nullable<Rcpp::NumericVector> &area_of_interest,
                       const Rcpp::Nullable<Rcpp::CharacterVector> &coord_op,
                       bool coord_op_inverse,
                       bool allow_ballpark,
                       double accuracy) {
     
     // SRS handling as in transform_xy(): accept anything srs_to_wkt()
     // accepts, and use traditional GIS axis order on both ends
     std::string srs_from_in = srs_to_wkt(srs_from, false);
     std::string srs_to_in = srs_to_wkt(srs_to, false);
     
     OGRSpatialReference oSourceSRS{}, oDestSRS{};
     OGRErr err = OGRERR_NONE;
     
     err = oSourceSRS.importFromWkt(srs_from_in.c_str());
     if (err != OGRERR_NONE)
         Rcpp::stop("failed to import source SRS from WKT string");
     
     oSourceSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
     
     err = oDestSRS.importFromWkt(srs_to_in.c_str());
     if (err != OGRERR_NONE)
         Rcpp::stop("failed to import destination SRS from WKT string");
     
     oDestSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
     
     OGRCoordinateTransformationOptions options;
     
     if (area_of_interest.isNotNull()) {
         Rcpp::NumericVector aoi(area_of_interest);
         if (aoi.size() != 4)
             Rcpp::stop("'area_of_interest' must have length 4 (W, S, E, N)");
         if (!options.SetAreaOfInterest(aoi[0], aoi[1], aoi[2], aoi[3]))
             Rcpp::stop("failed to set area of interest");
     }
     
     if (coord_op.isNotNull()) {
         Rcpp::CharacterVector co(coord_op);
         if (co.size() != 1 || Rcpp::CharacterVector::is_na(co[0]))
             Rcpp::stop("'coord_op' must be a single character string");
         std::string co_str = Rcpp::as<std::string>(co[0]);
         if (!options.SetCoordinateOperation(co_str.c_str(),
                                             coord_op_inverse)) {
             Rcpp::stop("failed to set coordinate operation");
         }
     }
     
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 3, 0)
     if (!allow_ballpark)
         options.SetBallparkAllowed(false);
     if (accuracy >= 0)
         options.SetDesiredAccuracy(accuracy);
#else
     if (!allow_ballpark)
         Rcpp::stop("'allow_ballpark = FALSE' requires GDAL >= 3.3");
     if (accuracy >= 0)
         Rcpp::stop("'accuracy' requires GDAL >= 3.3");
#endif
     
     OGRCoordinateTransformation *poCT =
         OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS, options);
     
     if (poCT == nullptr)
         Rcpp::stop("failed to create coordinate transformation");
     
     return make_wk_trans_xptr(poCT, use_z, use_m, srs_from, srs_to);
 }

//' Inverse of a gdalraster wk_trans object
 //'
 //' wk_trans_inverse() S3 method in R/wk_trans.R
 //' @noRd
 // [[Rcpp::export(name = ".transform_inverse")]]
 SEXP transform_inverse(const Rcpp::RObject &trans, int use_z, int use_m) {
     
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
     Rcpp::stop("inverse transformation requires GDAL >= 3.3");
     
#else
     GDALWkTrans *data = get_wk_trans_data(trans);
     
     OGRCoordinateTransformation *poInvCT = data->poCT->GetInverse();
     if (poInvCT == nullptr)
         Rcpp::stop("failed to create inverse coordinate transformation");
     
     return make_wk_trans_xptr(poInvCT, use_z, use_m, data->srs_to,
                               data->srs_from);
#endif
 }

//' Source and target SRS names of a gdalraster wk_trans object
 //'
 //' used by the print method in R/wk_trans.R
 //' @noRd
 // [[Rcpp::export(name = ".transform_info")]]
 Rcpp::CharacterVector transform_info(const Rcpp::RObject &trans) {
     
     GDALWkTrans *data = get_wk_trans_data(trans);
     
     // prefer the SRS name (e.g. from an EPSG code or WKT), but fall back to
     // the definition the user gave when there is no name, as for a PROJ
     // string like "+proj=laea" whose SRS name is "unknown"
     auto display_name = [](const OGRSpatialReference *poSRS,
                            const std::string &user_def) -> std::string {
                                if (poSRS != nullptr && poSRS->GetName() != nullptr) {
                                    std::string name = poSRS->GetName();
                                    if (!name.empty() && name != "unknown")
                                        return name;
                                }
                                return user_def;
                            };
     
     Rcpp::CharacterVector ret(2);
     ret[0] = display_name(data->poCT->GetSourceCS(), data->srs_from);
     ret[1] = display_name(data->poCT->GetTargetCS(), data->srs_to);
     ret.names() = Rcpp::CharacterVector::create("srs_from", "srs_to");
     return ret;
 }