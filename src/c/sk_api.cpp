#include "include/core/SkCanvas.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkSurface.h"
#include "include/core/SkEncodedImageFormat.h"
#include "include/c/sk_api.h"

SkSurface * createSurface( int w, int h ) {
    return SkSurface::MakeRasterN32Premul( w, h ).release();
}

SkData * makeSnapshot( SkSurface *srf ) {
    sk_sp<SkImage>img = srf->makeImageSnapshot();
    return img->encodeToData( SkEncodedImageFormat::kPNG, 100 ).release();
}



