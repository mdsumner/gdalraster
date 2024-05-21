#include "gdal.h"
#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

#include "vsifile.h"



Rcpp::RawVector vsi_read_file(Rcpp::CharacterVector src_file,
                              bool show_progress = false) {
  
  GDALProgressFunc pfnProgress = nullptr;
  std::string src_file_in;
  src_file_in = Rcpp::as<std::string>(_check_gdal_filename(src_file));
  
  if (show_progress)
    pfnProgress = GDALTermProgressR;
  
  
  VSILFILE *fp = VSIFOpenL(src_file_in.c_str(), "rb");
  //ASSERT_TRUE(fp != nullptr);
  VSIFSeekL(fp, 0, SEEK_END);
  int64_t nSize = (int64_t)VSIFTellL(fp);
  VSIFSeekL(fp, 0, SEEK_SET);
  //void *pRefBuf = CPLMalloc(nSize);
  Rcpp::RawVector out(nSize); 
  void* buffer = CPLMalloc(nSize);
  VSIFReadL(buffer, 1, nSize, fp);
  memcpy(&(out[0]), buffer, nSize);
  VSIFCloseL(fp);
  
  return out; 
}
