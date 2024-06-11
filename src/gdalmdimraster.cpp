/* Implementation of class GDALMDIMRaster
   Encapsulates a subset of GDALDataset of GDAL_OF_MULTIDIM_RASTER mode.

   Michael Sumner <mdsumner@gmail.com>
   Copyright (c) 2023-2024 gdalraster authors
*/


#include "gdal.h"
#include "gdalmdimraster.h"
#include "cpl_string.h"

 GDALMDIMRaster::GDALMDIMRaster() :
             dsn_in(""),
             eAccess(GA_ReadOnly),
             open_options_in(Rcpp::CharacterVector::create()),
             hDataset(nullptr),
             hActiveMDArray(nullptr),
             b_mdarray_open(false),
             rootGroupName(Rcpp::CharacterVector::create("abc"))
             {}

 GDALMDIMRaster::GDALMDIMRaster(Rcpp::CharacterVector dsn) :
   GDALMDIMRaster(dsn,
                  true,
                  Rcpp::CharacterVector::create(),
                  false)
 {
   b_mdarray_open = false;
 }
 GDALMDIMRaster::GDALMDIMRaster(Rcpp::CharacterVector dsn,
                                bool read_only,
                                Rcpp::Nullable<Rcpp::CharacterVector> open_options,
                                bool shared_in) :
   dsn_in(dsn),
   open_options_in(open_options),
   hDataset(nullptr),
   b_mdarray_open(false)
 {
   rootGroupName = Rcpp::CharacterVector::create(NA_STRING);
   if (read_only) {
     eAccess = GA_ReadOnly;
   } else {
     eAccess = GA_Update;
   }
   open(read_only);

 }

 void GDALMDIMRaster::open(bool read_only) {
   if (dsn_in[0] == "")
     Rcpp::stop("'dsn' is not set");

   if (hDataset != nullptr) {
    //ouchi!!  close();
   }
   std::vector<char *> dsoo(open_options_in.size() + 1);
   if (open_options_in.size() > 0) {
     for (R_xlen_t i = 0; i < open_options_in.size(); ++i) {
       dsoo[i] = (char *) (open_options_in[i]);
     }
   }
   dsoo.push_back(nullptr);

   unsigned int nOpenFlags = GDAL_OF_MULTIDIM_RASTER;
   if (read_only) {
     eAccess = GA_ReadOnly;
     nOpenFlags |= GDAL_OF_READONLY;
   }
   else {
     eAccess = GA_Update;
     nOpenFlags |= GDAL_OF_UPDATE;
   }
   if (shared_in)
     nOpenFlags |= GDAL_OF_SHARED;

   hDataset = GDALOpenEx(dsn_in[0], nOpenFlags, nullptr,
                         dsoo.data(), nullptr);

   hRootGroup = GDALDatasetGetRootGroup(hDataset);
   rootGroupName = Rcpp::CharacterVector::create(GDALGroupGetName(hRootGroup));
   if (hDataset == nullptr)
     Rcpp::stop("open raster failed");
 }

 void GDALMDIMRaster::close() {
   // make sure caches are flushed when access was GA_Update:
   // since the dataset was opened shared, and could still have a shared
   // read-only handle (not recommended), or may be re-opened for read and
   // is on a /vsicurl/ filesystem,
   if (eAccess == GA_Update) {
     //flushCache();
     //vsi_curl_clear_cache(true, fname_in, true);
   }

#if GDAL_VERSION_NUM >= 3070000
   if (GDALClose(hDataset) != CE_None)
     Rcpp::warning("error occurred during GDALClose()!");
#else
   GDALClose(hDataset);
#endif

   hDataset = nullptr;
 }

 Rcpp::CharacterVector GDALMDIMRaster::MDArrayNames() const {

   char **papszArrays = GDALGroupGetMDArrayNames(hRootGroup, nullptr);
   int n_items = CSLCount(papszArrays);
   Rcpp::CharacterVector array_names(n_items);
   for (int i=0; i < n_items; ++i) {
     array_names(i) = papszArrays[i];
   }
   CSLDestroy(papszArrays);
   return array_names;
 }

 void GDALMDIMRaster::openMDArray(Rcpp::CharacterVector array_name) {
   hActiveMDArray = GDALGroupOpenMDArray(hRootGroup, array_name[0], NULL);
   b_mdarray_open = true;
 }
 Rcpp::CharacterVector GDALMDIMRaster::activeMDArrayName() {
  /// crashes
//    if (hActiveMDArray == nullptr) return Rcpp::CharacterVector::create(NA_STRING);
    if (!b_mdarray_open) return Rcpp::CharacterVector::create(NA_STRING);

   // implicit converision of const char* with Rcpp::wrap()

   return GDALMDArrayGetName(hActiveMDArray);
 }
// this is no good because we release the dataset opened above, what about reopening?

// yes ok but don't try opening a non-existent array
// x$shape("elev")
//    ERROR 10: Pointer 'hArray' is NULL in 'GDALMDArrayGetDimensions'.
//
//  Error: cannot allocate vector of size 350340.4 Gb
//
 Rcpp::IntegerVector GDALMDIMRaster::shape(Rcpp::CharacterVector array_name) {
   //if (!b_mdarray_open) return Rcpp::IntegerVector::create(NA_INTEGER);

   GDALGroupH hGroup = GDALDatasetGetRootGroup(hDataset);
   GDALReleaseDataset(hDataset);
   GDALMDArrayH hVar = GDALGroupOpenMDArray(hGroup, array_name[0], NULL);
   GDALGroupRelease(hGroup);
   size_t nDimCount;
   if (hVar == nullptr) {
     open(true);
     Rcpp::warning("array of that name does not exist");
     return Rcpp::IntegerVector::create(NA_INTEGER);
   }
   GDALDimensionH* dims = GDALMDArrayGetDimensions(hVar, &nDimCount);

   size_t nValues;
   size_t i;
   nValues = 1;
   Rcpp::IntegerVector out(nDimCount);

   for( i = 0; i < nDimCount; i++ )
   {
     out[i] = GDALDimensionGetSize(dims[i]);
     nValues *= out[i];
   }
   GDALReleaseDimensions(dims, nDimCount);
   GDALMDArrayRelease(hVar);
   open(true);
   return out;
 }
RCPP_MODULE(mod_GDALMDIMRaster) {
    Rcpp::class_<GDALMDIMRaster>("GDALMDIMRaster")

   .constructor
        ("Default constructor, no dataset opened")
   .constructor<Rcpp::CharacterVector>
        ("Usage: new(GDALMDIMRaster, dsn)")
   .constructor<Rcpp::CharacterVector, bool, Rcpp::Nullable<Rcpp::CharacterVector>, bool>
        ("Usage: new(GDALMDIMRaster, dsn, read_only, open_options, shared_in)")

   // exposed read/write fields
   .field("rootGroupName", &GDALMDIMRaster::rootGroupName)

   .method("MDArrayNames", &GDALMDIMRaster::MDArrayNames)
   .method("openMDArray", &GDALMDIMRaster::openMDArray)
   .method("activeMDArrayName", &GDALMDIMRaster::activeMDArrayName)
   .method("close", &GDALMDIMRaster::close)
   .method("shape", &GDALMDIMRaster::shape)

    ;
}


 //.constructor<Rcpp::CharacterVector>
 //  ("Usage: new(GDALRaster, filename)")
