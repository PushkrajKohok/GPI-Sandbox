#include "diff.h"
#include <cmath>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/stb_image.h"

bool diff_png_vs_png(const std::string& exp, const std::string& act,
                     int abs_thr, DiffResult& out) {
  int w1,h1,c1, w2,h2,c2;
  unsigned char *a = stbi_load(exp.c_str(), &w1,&h1,&c1, 4);
  unsigned char *b = stbi_load(act.c_str(), &w2,&h2,&c2, 4);
  if (!a || !b || w1!=w2 || h1!=h2) { 
    if(a) stbi_image_free(a); 
    if(b) stbi_image_free(b); 
    return false; 
  }

  const int N = w1*h1;
  long long sumsq = 0;
  int over = 0;
  for (int i=0;i<N;i++){
    int d0 = (int)a[4*i+0] - (int)b[4*i+0];
    int d1 = (int)a[4*i+1] - (int)b[4*i+1];
    int d2 = (int)a[4*i+2] - (int)b[4*i+2];
    // ignore alpha
    sumsq += (long long)(d0*d0 + d1*d1 + d2*d2);
    if (std::abs(d0)>abs_thr || std::abs(d1)>abs_thr || std::abs(d2)>abs_thr) over++;
  }
  out.w=w1; out.h=h1;
  double mse = (double)sumsq / (double)(N * 3);
  out.rmse = std::sqrt(mse);
  out.pct_over_threshold = 100.0 * (double)over / (double)N;
  stbi_image_free(a); stbi_image_free(b);
  return true;
}
