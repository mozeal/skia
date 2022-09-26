/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "HelloWorld.h"
#include <stdio.h>
extern "C" {
    #include "include/c/sk_canvas.h"
    #include "include/c/sk_data.h"
    #include "include/c/sk_image.h"
    #include "include/c/sk_imageinfo.h"
    #include "include/c/sk_paint.h"
    #include "include/c/sk_path.h"
    #include "include/c/sk_surface.h"
}

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkShader.h"
#include "include/core/SkString.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkGradientShader.h"
#include "tools/sk_app/DisplayParams.h"

#include <string.h>

using namespace sk_app;

static sk_surface_t* make_surface(int32_t w, int32_t h) {
    sk_imageinfo_t* info = sk_imageinfo_new(w, h, RGBA_8888_SK_COLORTYPE,
                                            PREMUL_SK_ALPHATYPE, NULL);
    sk_surface_t* result = sk_surface_new_raster(info, NULL);
    sk_imageinfo_delete(info);
    return result;
}

void draw(sk_canvas_t* canvas) {
    sk_paint_t* fill = sk_paint_new();
    sk_paint_set_color(fill, sk_color_set_argb(0xFF, 0x00, 0x00, 0xFF));
    sk_canvas_draw_paint(canvas, fill);

    sk_paint_set_color(fill, sk_color_set_argb(0xFF, 0x00, 0xFF, 0xFF));
    sk_rect_t rect;
    rect.left = 100.0f;
    rect.top = 100.0f;
    rect.right = 540.0f;
    rect.bottom = 380.0f;
    sk_canvas_draw_rect(canvas, &rect, fill);

    sk_paint_t* stroke = sk_paint_new();
    sk_paint_set_color(stroke, sk_color_set_argb(0xFF, 0xFF, 0x00, 0x00));
    sk_paint_set_antialias(stroke, true);
    sk_paint_set_stroke(stroke, true);
    sk_paint_set_stroke_width(stroke, 5.0f);

    sk_pathbuilder_t* path_builder = sk_pathbuilder_new();
    sk_pathbuilder_move_to(path_builder, 50.0f, 50.0f);
    sk_pathbuilder_line_to(path_builder, 590.0f, 50.0f);
    sk_pathbuilder_cubic_to(path_builder, -490.0f, 50.0f, 1130.0f, 430.0f, 50.0f, 430.0f);
    sk_pathbuilder_line_to(path_builder, 590.0f, 430.0f);

    sk_path_t* path = sk_pathbuilder_detach_path(path_builder);
    sk_canvas_draw_path(canvas, path, stroke);

    sk_paint_set_color(fill, sk_color_set_argb(0x80, 0x00, 0xFF, 0x00));
    sk_rect_t rect2;
    rect2.left = 120.0f;
    rect2.top = 120.0f;
    rect2.right = 520.0f;
    rect2.bottom = 360.0f;
    sk_canvas_draw_oval(canvas, &rect2, fill);

    sk_pathbuilder_delete(path_builder);
    sk_path_delete(path);
    sk_paint_delete(stroke);
    sk_paint_delete(fill);
}

static void emit_png(const char* path, sk_surface_t* surface) {
    sk_image_t* image = sk_surface_new_image_snapshot(surface);
    sk_data_t* data = sk_image_encode(image);
    sk_image_unref(image);
    FILE* f = fopen(path, "wb");
    fwrite(sk_data_get_data(data), sk_data_get_size(data), 1, f);
    fclose(f);
    sk_data_unref(data);
}

int main() {
    printf( "Hello world\n" );

    sk_surface_t* surface = make_surface(640, 480);
    sk_canvas_t* canvas = sk_surface_get_canvas(surface);

    draw(canvas);

    emit_png("skia-c-example.png", surface);
    sk_surface_unref(surface);    

}



/*
Application* Application::Create(int argc, char** argv, void* platformData) {
    return new HelloWorld(argc, argv, platformData);
}

HelloWorld::HelloWorld(int argc, char** argv, void* platformData)
#if defined(SK_GL)
        : fBackendType(Window::kNativeGL_BackendType),
#elif defined(SK_VULKAN)
        : fBackendType(Window::kVulkan_BackendType),
#elif defined(SK_DAWN)
        : fBackendType(Window::kDawn_BackendType),
#else
        : fBackendType(Window::kRaster_BackendType),
#endif
        fRotationAngle(0) {
    SkGraphics::Init();

    fWindow = Window::CreateNativeWindow(platformData);
    fWindow->setRequestedDisplayParams(DisplayParams());

    // register callbacks
    fWindow->pushLayer(this);

    fWindow->attach(fBackendType);
}

HelloWorld::~HelloWorld() {
    fWindow->detach();
    delete fWindow;
}

void HelloWorld::updateTitle() {
    if (!fWindow) {
        return;
    }

    SkString title("Hello World ");
    if (Window::kRaster_BackendType == fBackendType) {
        title.append("Raster");
    } else {
#if defined(SK_GL)
        title.append("GL");
#elif defined(SK_VULKAN)
        title.append("Vulkan");
#elif defined(SK_DAWN)
        title.append("Dawn");
#else
        title.append("Unknown GPU backend");
#endif
    }

    fWindow->setTitle(title.c_str());
}

void HelloWorld::onBackendCreated() {
    this->updateTitle();
    fWindow->show();
    fWindow->inval();
}

void HelloWorld::onPaint(SkSurface* surface) {
    auto canvas = surface->getCanvas();

    // Clear background
    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SK_ColorRED);

    // Draw a rectangle with red paint
    SkRect rect = SkRect::MakeXYWH(10, 10, 128, 128);
    canvas->drawRect(rect, paint);

    // Set up a linear gradient and draw a circle
    {
        SkPoint linearPoints[] = { { 0, 0 }, { 300, 300 } };
        SkColor linearColors[] = { SK_ColorGREEN, SK_ColorBLACK };
        paint.setShader(SkGradientShader::MakeLinear(linearPoints, linearColors, nullptr, 2,
                                                     SkTileMode::kMirror));
        paint.setAntiAlias(true);

        canvas->drawCircle(200, 200, 64, paint);

        // Detach shader
        paint.setShader(nullptr);
    }

    // Draw a message with a nice black paint
    SkFont font;
    font.setSubpixel(true);
    font.setSize(20);
    paint.setColor(SK_ColorBLACK);

    canvas->save();
    static const char message[] = "Hello World ";

    // Translate and rotate
    canvas->translate(300, 300);
    fRotationAngle += 0.2f;
    if (fRotationAngle > 360) {
        fRotationAngle -= 360;
    }
    canvas->rotate(fRotationAngle);

    // Draw the text
    canvas->drawSimpleText(message, strlen(message), SkTextEncoding::kUTF8, 0, 0, font, paint);

    canvas->restore();
}

void HelloWorld::onIdle() {
    // Just re-paint continuously
    fWindow->inval();
}

bool HelloWorld::onChar(SkUnichar c, skui::ModifierKey modifiers) {
    if (' ' == c) {
        if (Window::kRaster_BackendType == fBackendType) {
#if defined(SK_GL)
            fBackendType = Window::kNativeGL_BackendType;
#elif defined(SK_VULKAN)
            fBackendType = Window::kVulkan_BackendType;
#elif defined(SK_DAWN)
            fBackendType = Window::kDawn_BackendType;
#else
            SkDebugf("No GPU backend configured\n");
            return true;
#endif
        } else {
            fBackendType = Window::kRaster_BackendType;
        }
        fWindow->detach();
        fWindow->attach(fBackendType);
    }
    return true;
}
*/