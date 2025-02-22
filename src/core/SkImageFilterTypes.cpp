/*
 * Copyright 2019 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/core/SkImageFilterTypes.h"

#include "src/core/SkImageFilter_Base.h"
#include "src/core/SkMatrixPriv.h"

// This exists to cover up issues where infinite precision would produce integers but float
// math produces values just larger/smaller than an int and roundOut/In on bounds would produce
// nearly a full pixel error. One such case is crbug.com/1313579 where the caller has produced
// near integer CTM and uses integer crop rects that would grab an extra row/column of the
// input image when using a strict roundOut.
static constexpr float kRoundEpsilon = 1e-3f;

// Both [I]Vectors and Sk[I]Sizes are transformed as non-positioned values, i.e. go through
// mapVectors() not mapPoints().
static SkIVector map_as_vector(int32_t x, int32_t y, const SkMatrix& matrix) {
    SkVector v = SkVector::Make(SkIntToScalar(x), SkIntToScalar(y));
    matrix.mapVectors(&v, 1);
    return SkIVector::Make(SkScalarRoundToInt(v.fX), SkScalarRoundToInt(v.fY));
}

static SkVector map_as_vector(SkScalar x, SkScalar y, const SkMatrix& matrix) {
    SkVector v = SkVector::Make(x, y);
    matrix.mapVectors(&v, 1);
    return v;
}

// If m is epsilon within the form [1 0 tx], this returns true and sets out to [tx, ty]
//                                 [0 1 ty]
//                                 [0 0 1 ]
// TODO: Use this in decomposeCTM() (and possibly extend it to support is_nearly_scale_translate)
// to be a little more forgiving on matrix types during layer configuration.
static bool is_nearly_integer_translation(const skif::LayerSpace<SkMatrix>& m,
                                          skif::LayerSpace<SkIPoint>* out=nullptr) {
    float tx = SkScalarRoundToScalar(sk_ieee_float_divide(m.rc(0,2), m.rc(2,2)));
    float ty = SkScalarRoundToScalar(sk_ieee_float_divide(m.rc(1,2), m.rc(2,2)));
    SkMatrix expected = SkMatrix::MakeAll(1.f, 0.f, tx,
                                          0.f, 1.f, ty,
                                          0.f, 0.f, 1.f);
    for (int i = 0; i < 9; ++i) {
        if (!SkScalarNearlyEqual(expected.get(i), m.get(i), kRoundEpsilon)) {
            return false;
        }
    }

    if (out) {
        *out = skif::LayerSpace<SkIPoint>({(int) tx, (int) ty});
    }
    return true;
}

namespace skif {

SkIRect RoundOut(SkRect r) { return r.makeInset(kRoundEpsilon, kRoundEpsilon).roundOut(); }

SkIRect RoundIn(SkRect r) { return r.makeOutset(kRoundEpsilon, kRoundEpsilon).roundIn(); }

bool Mapping::decomposeCTM(const SkMatrix& ctm, const SkImageFilter* filter,
                           const skif::ParameterSpace<SkPoint>& representativePt) {
    SkMatrix remainder, layer;
    SkSize decomposed;
    using MatrixCapability = SkImageFilter_Base::MatrixCapability;
    MatrixCapability capability =
            filter ? as_IFB(filter)->getCTMCapability() : MatrixCapability::kComplex;
    if (capability == MatrixCapability::kTranslate) {
        // Apply the entire CTM post-filtering
        remainder = ctm;
        layer = SkMatrix::I();
    } else if (ctm.isScaleTranslate() || capability == MatrixCapability::kComplex) {
        // Either layer space can be anything (kComplex) - or - it can be scale+translate, and the
        // ctm is. In both cases, the layer space can be equivalent to device space.
        remainder = SkMatrix::I();
        layer = ctm;
    } else if (ctm.decomposeScale(&decomposed, &remainder)) {
        // This case implies some amount of sampling post-filtering, either due to skew or rotation
        // in the original matrix. As such, keep the layer matrix as simple as possible.
        layer = SkMatrix::Scale(decomposed.fWidth, decomposed.fHeight);
    } else {
        // Perspective, which has a non-uniform scaling effect on the filter. Pick a single scale
        // factor that best matches where the filter will be evaluated.
        SkScalar scale = SkMatrixPriv::DifferentialAreaScale(ctm, SkPoint(representativePt));
        if (SkScalarIsFinite(scale) && !SkScalarNearlyZero(scale)) {
            // Now take the sqrt to go from an area scale factor to a scaling per X and Y
            // FIXME: It would be nice to be able to choose a non-uniform scale.
            scale = SkScalarSqrt(scale);
        } else {
            // The representative point was behind the W = 0 plane, so don't factor out any scale.
            // NOTE: This makes remainder and layer the same as the MatrixCapability::Translate case
            scale = 1.f;
        }

        remainder = ctm;
        remainder.preScale(SkScalarInvert(scale), SkScalarInvert(scale));
        layer = SkMatrix::Scale(scale, scale);
    }

    SkMatrix invRemainder;
    if (!remainder.invert(&invRemainder)) {
        // Under floating point arithmetic, it's possible to decompose an invertible matrix into
        // a scaling matrix and a remainder and have the remainder be non-invertible. Generally
        // when this happens the scale factors are so large and the matrix so ill-conditioned that
        // it's unlikely that any drawing would be reasonable, so failing to make a layer is okay.
        return false;
    } else {
        fParamToLayerMatrix = layer;
        fLayerToDevMatrix = remainder;
        fDevToLayerMatrix = invRemainder;
        return true;
    }
}

bool Mapping::adjustLayerSpace(const SkMatrix& layer) {
    SkMatrix invLayer;
    if (!layer.invert(&invLayer)) {
        return false;
    }
    fParamToLayerMatrix.postConcat(layer);
    fDevToLayerMatrix.postConcat(layer);
    fLayerToDevMatrix.preConcat(invLayer);
    return true;
}

// Instantiate map specializations for the 6 geometric types used during filtering
template<>
SkRect Mapping::map<SkRect>(const SkRect& geom, const SkMatrix& matrix) {
    return matrix.mapRect(geom);
}

template<>
SkIRect Mapping::map<SkIRect>(const SkIRect& geom, const SkMatrix& matrix) {
    return RoundOut(matrix.mapRect(SkRect::Make(geom)));
}

template<>
SkIPoint Mapping::map<SkIPoint>(const SkIPoint& geom, const SkMatrix& matrix) {
    SkPoint p = SkPoint::Make(SkIntToScalar(geom.fX), SkIntToScalar(geom.fY));
    matrix.mapPoints(&p, 1);
    return SkIPoint::Make(SkScalarRoundToInt(p.fX), SkScalarRoundToInt(p.fY));
}

template<>
SkPoint Mapping::map<SkPoint>(const SkPoint& geom, const SkMatrix& matrix) {
    SkPoint p;
    matrix.mapPoints(&p, &geom, 1);
    return p;
}

template<>
IVector Mapping::map<IVector>(const IVector& geom, const SkMatrix& matrix) {
    return IVector(map_as_vector(geom.fX, geom.fY, matrix));
}

template<>
Vector Mapping::map<Vector>(const Vector& geom, const SkMatrix& matrix) {
    return Vector(map_as_vector(geom.fX, geom.fY, matrix));
}

template<>
SkISize Mapping::map<SkISize>(const SkISize& geom, const SkMatrix& matrix) {
    SkIVector v = map_as_vector(geom.fWidth, geom.fHeight, matrix);
    return SkISize::Make(v.fX, v.fY);
}

template<>
SkSize Mapping::map<SkSize>(const SkSize& geom, const SkMatrix& matrix) {
    SkVector v = map_as_vector(geom.fWidth, geom.fHeight, matrix);
    return SkSize::Make(v.fX, v.fY);
}

template<>
SkMatrix Mapping::map<SkMatrix>(const SkMatrix& m, const SkMatrix& matrix) {
    // If 'matrix' maps from the C1 coord space to the C2 coord space, and 'm' is a transform that
    // operates on, and outputs to, the C1 coord space, we want to return a new matrix that is
    // equivalent to 'm' that operates on and outputs to C2. This is the same as mapping the input
    // from C2 to C1 (matrix^-1), then transforming by 'm', and then mapping from C1 to C2 (matrix).
    SkMatrix inv;
    SkAssertResult(matrix.invert(&inv));
    inv.postConcat(m);
    inv.postConcat(matrix);
    return inv;
}

sk_sp<SkSpecialImage> FilterResult::imageAndOffset(SkIPoint* offset) const {
    if (!fImage) {
        *offset = {0, 0};
        return nullptr;
    }

    LayerSpace<SkIPoint> origin;
    // TODO: When there are other tile modes than kDecal, imageAndOffset needs to apply them even if
    // the transform is representable as an offset.
    if (is_nearly_integer_translation(fTransform, &origin)) {
        // Nothing to resolve
        *offset = SkIPoint(origin);
        return fImage;
    } else {
        // Legacy fallback requires us to produce a new image that matches this FilterResult but
        // has no new properties legacy code can't handle.
        return this->resolve(this->layerBounds()).imageAndOffset(offset);
    }
}

FilterResult FilterResult::applyCrop(const Context& ctx,
                                     const LayerSpace<SkIRect>& crop) const {
    LayerSpace<SkIRect> tightBounds = crop;
    // TODO(michaelludwig): Intersecting to the target output is only valid when the crop has
    // decal tiling (the only current option).
    if (!fImage || !tightBounds.intersect(ctx.desiredOutput())) {
        // The desired output would be filled with transparent black.
        return {};
    }

    if (crop.contains(fLayerBounds)) {
        // The original crop does not affect the image (although the context's desired output might)
        // We can tighten fLayerBounds to tightBounds without resolving the image, regardless of the
        // transform type.
        // TODO(michaelludwig): If the crop would use mirror or repeat, the above isn't true.
        FilterResult restrictedOutput = *this;
        SkAssertResult(restrictedOutput.fLayerBounds.intersect(ctx.desiredOutput()));
        return restrictedOutput;
    } else {
        // The crop modifies the effective image. If the transform is axis-aligned, we can apply the
        // crop to the special image directly w/o resolving the transformation. Otherwise the image
        // is not pixel-aligned with the layer coordinate space, so the crop rect's edges aren't
        // aligned with the image data and we have to resolve it first.

        // // TODO(michaelludwig): Only valid for kDecal, although kClamp would only need 1 extra
        // pixel of padding so some restriction could happen.
        if (!tightBounds.intersect(fLayerBounds)) {
            return {};
        }

        // TODO(michaelludwig): If we get to the point where all filter results track bounds in
        // floating point, then we can extend this case to any S+T transform.
        LayerSpace<SkIPoint> origin;
        if (is_nearly_integer_translation(fTransform, &origin)) {
            LayerSpace<IVector> originShift = tightBounds.topLeft() - origin;
            auto subsetImage = fImage->makeSubset(SkIRect::MakeXYWH(originShift.x(),
                                                                    originShift.y(),
                                                                    tightBounds.width(),
                                                                    tightBounds.height()));
            return {std::move(subsetImage), tightBounds.topLeft()};
        } else {
            return this->resolve(tightBounds);
        }
    }
}

static bool compatible_sampling(const SkSamplingOptions& current,
                                SkSamplingOptions* next) {
    // Both transforms could perform non-trivial sampling, but if they are similar enough we
    // assume performing one non-trivial sampling operation with the concatenated transform will
    // not be visually distinguishable from sampling twice.
    // TODO(michaelludwig): For now ignore mipmap policy, SkSpecialImages are not supposed to be
    // drawn with mipmapping, and the majority of filter steps produce images that are at the
    // proper scale and do not define mip levels. The main exception is the ::Image() filter
    // leaf but that doesn't use this system yet.
    if (current.isAniso() && next->isAniso()) {
        // Assume we can get away with one sampling at the highest anisotropy level
        *next =  SkSamplingOptions::Aniso(std::max(current.maxAniso, next->maxAniso));
        return true;
    } else if (current.useCubic && (next->filter == SkFilterMode::kLinear ||
                                    (next->useCubic && current.cubic.B == next->cubic.B &&
                                                      current.cubic.C == next->cubic.C))) {
        // Assume we can get away with the current bicubic filter, since the next is the same
        // or a bilerp that can be upgraded.
        *next = current;
        return true;
    } else if (next->useCubic && current.filter == SkFilterMode::kLinear) {
        // Mirror of the above, assume we can just get away with next's cubic resampler
        return true;
    } else if (current.filter == SkFilterMode::kLinear && next->filter == SkFilterMode::kLinear) {
        // Assume we can get away with a single bilerp vs. the two
        return true;
    } else {
        // The current or next sampling is nearest neighbor, and will produce visible texels
        // oriented with the current transform; assume this is a desired effect and preserve it.
        return false;
    }
}

FilterResult FilterResult::applyTransform(const Context& ctx,
                                          const LayerSpace<SkMatrix> &transform,
                                          const SkSamplingOptions &sampling) const {
    if (!fImage) {
        // Transformed transparent black remains transparent black.
        return {};
    }

    // Extract the sampling options that matter based on the current and next transforms.
    // We make sure the new sampling is bilerp (default) if the new transform doesn't matter
    // (and assert that the current is bilerp if its transform didn't matter). Bilerp can be
    // maximally combined, so simplifies the logic in compatible_sampling().
    SkASSERT(!is_nearly_integer_translation(fTransform) || fSamplingOptions == kDefaultSampling);
    SkSamplingOptions nextSampling = is_nearly_integer_translation(transform)
            ? kDefaultSampling : sampling;

    FilterResult transformed;
    if (compatible_sampling(fSamplingOptions, &nextSampling)) {
        // We can concat transforms and 'nextSampling' will be either fSamplingOptions,
        // sampling, or a merged combination depending on the two transforms in play.
        transformed = *this;
    } else {
        // We'll have to resolve this FilterResult first before 'transform' and 'sampling' can be
        // correctly evaluated. 'nextSampling' will always be 'sampling'.
        transformed = this->resolve(fLayerBounds);
    }

    transformed.concatTransform(transform, nextSampling, ctx.desiredOutput());
    if (transformed.layerBounds().isEmpty()) {
        return {};
    } else {
        return transformed;
    }
}

void FilterResult::concatTransform(const LayerSpace<SkMatrix>& transform,
                                   const SkSamplingOptions& newSampling,
                                   const LayerSpace<SkIRect>& desiredOutput) {
    SkASSERT(fImage);
    fSamplingOptions = newSampling;
    fTransform.postConcat(transform);
    // Rebuild the layer bounds and then restrict to the current desired output.
    fLayerBounds = fTransform.mapRect(LayerSpace<SkRect>(SkRect::MakeIWH(fImage->width(),
                                                                         fImage->height())))
                             .roundOut();
    if (!fLayerBounds.intersect(desiredOutput)) {
        // The transformed output doesn't touch the desired, so it would just be transparent black.
        // TODO: This intersection only applies when the tile mode is kDecal.
        fLayerBounds = LayerSpace<SkIRect>::Empty();
    }
}

FilterResult FilterResult::resolve(const LayerSpace<SkIRect>& dstBounds) const {
    // This assumes that 'dstBounds' has already been optimized to be as small as necessary for
    // correctness given any further processing and desired output, and assuming kDecal sampling.
    if (!fImage || dstBounds.isEmpty()) {
        return {};
    }

    sk_sp<SkSpecialSurface> surface = fImage->makeSurface(fImage->colorType(),
                                                          fImage->getColorSpace(),
                                                          SkISize(dstBounds.size()),
                                                          kPremul_SkAlphaType, {});
    if (!surface) {
        return {};
    }
    SkCanvas* canvas = surface->getCanvas();
    // skbug.com/5075: GPU-backed special surfaces don't reset their contents.
    canvas->clear(SK_ColorTRANSPARENT);
    canvas->translate(-dstBounds.left(), -dstBounds.top()); // dst's origin adjustment

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrc);

    // TODO: When using a tile mode other than kDecal, we'll need to use SkSpecialImage::asShader()
    // and use drawRect(fLayerBounds).
    if (!fLayerBounds.contains(dstBounds)) {
        // We're resolving to a larger than necessary image, so make sure transparency outside of
        // fLayerBounds is preserved.
        // NOTE: This should only happen when the next layer requires processing transparent black.
        canvas->clipIRect(SkIRect(fLayerBounds));
    }
    canvas->concat(SkMatrix(fTransform)); // src's origin is embedded in fTransform
    fImage->draw(canvas, 0.f, 0.f, fSamplingOptions, &paint);

    return {surface->makeImageSnapshot(), dstBounds.topLeft()};
}

} // end namespace skif
