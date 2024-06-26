/* Misc. utility functions for internal use
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "rcpp_util.h"

//' convert data frame to numeric matrix in Rcpp
//' @noRd
Rcpp::NumericMatrix df_to_matrix_(const Rcpp::DataFrame& df) {
    Rcpp::NumericMatrix m(df.nrows(), df.size());
    for (R_xlen_t i=0; i < df.size(); ++i) {
        m.column(i) = Rcpp::NumericVector(df[i]);
    }
    return m;
}

//' convert data frame to integer matrix in Rcpp
//' @noRd
Rcpp::IntegerMatrix df_to_int_matrix_(const Rcpp::DataFrame& df) {
    Rcpp::IntegerMatrix m(df.nrows(), df.size());
    for (R_xlen_t i=0; i < df.size(); ++i) {
        m.column(i) = Rcpp::IntegerVector(df[i]);
    }
    return m;
}

//' wrapper for base R path.expand()
//' @noRd
Rcpp::CharacterVector path_expand_(Rcpp::CharacterVector path) {

    Rcpp::Function f("path.expand");
    return f(path);
}

//' wrapper for base R normalizePath()
//' int must_work should be NA_LOGICAL (the default), 0 or 1
//' @noRd
Rcpp::CharacterVector normalize_path_(Rcpp::CharacterVector path,
        int must_work) {

    Rcpp::Function f("normalizePath");
    return f(path, Rcpp::Named("mustWork") = must_work);
}

//' wrapper for base R enc2utf8()
//' @noRd
Rcpp::CharacterVector enc_to_utf8_(Rcpp::CharacterVector x) {
    Rcpp::Function f("enc2utf8");
    return f(x);
}

//' std::string to uppercase
//' @noRd
std::string str_toupper_(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}
