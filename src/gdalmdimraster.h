/* R interface to a subset of the GDAL C API for multidim raster. A class for
 * GDALDataset in GDAL_OF_MULTIDIM_RASTER type.
 * https://gdal.org/user/multidim_raster_data_model.html
 Michael Sumner <mdsumner@gmail.com>
*/

#ifndef SRC_GDALMDIMRASTER_H_
#define SRC_GDALMDIMRASTER_H_

#include "Rcpp.h"

#include "rcpp_util.h"


// Predeclare some GDAL types until the public header is included
#ifndef GDAL_H_INCLUDED
typedef void *GDALDatasetH;
typedef void *GDALMDArrayH;
typedef void *GDALGroupH;

typedef enum {GA_ReadOnly = 0, GA_Update = 1} GDALAccess;
#endif

class GDALMDIMRaster {
 private:
  Rcpp::CharacterVector dsn_in;
  GDALAccess eAccess;
  Rcpp::CharacterVector open_options_in;
  GDALDatasetH hDataset;
  bool shared_in;
  GDALMDArrayH hActiveMDArray;
  bool b_mdarray_open = false;
  GDALGroupH hRootGroup;

 public:
   GDALMDIMRaster();
   explicit GDALMDIMRaster(Rcpp::CharacterVector dsn);
   GDALMDIMRaster(Rcpp::CharacterVector dsn,
                  bool read_only,
                  Rcpp::Nullable<Rcpp::CharacterVector> open_options,
                  bool shared_in);

   Rcpp::CharacterVector rootGroupName;
   Rcpp::CharacterVector MDArrayNames() const;
   void open(bool read_only);
   void close();
   void openMDArray(Rcpp::CharacterVector array_name);
   Rcpp::CharacterVector activeMDArrayName();

   Rcpp::IntegerVector shape(Rcpp::CharacterVector array_name);
};

RCPP_EXPOSED_CLASS(GDALMDIMRaster)

#endif  // SRC_GDALMDIMRASTER_H_

