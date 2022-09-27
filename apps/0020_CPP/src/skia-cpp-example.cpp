/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/
#include <stdio.h>
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
#include "include/core/SkStream.h"

SkSurface * createSurface( int w, int h );
SkData * makeSnapshot( SkSurface *srf );

void draw( SkCanvas *canvas ) {
    SkPaint paintFill;
    paintFill.setColor( SkColorSetARGB(0xFF, 0x00, 0x00, 0xFF) );
    canvas->drawPaint( paintFill );

    paintFill.setColor( SkColorSetARGB(0xFF, 0x00, 0xFF, 0xFF) );
    SkRect rect = SkRect::MakeLTRB( 100.0, 100.0, 540.0, 380.0 );
    canvas->drawRect( rect, paintFill );

    SkPaint paintStroke;
    paintStroke.setColor( SkColorSetARGB(0xFF, 0xFF, 0x00, 0x00) );
    paintStroke.setAntiAlias( true );
    paintStroke.setStyle( SkPaint::Style::kStroke_Style );
    paintStroke.setStrokeWidth( 5.0 );

    SkPathBuilder pathBuilder;
    pathBuilder.moveTo( 50.0f, 50.0f );
    pathBuilder.lineTo( 590.0f, 50.0f );
    pathBuilder.cubicTo( -490.0f, 50.0f, 1130.0f, 430.0f, 50.0f, 430.0f );
    pathBuilder.lineTo( 590.0f, 430.0f );

    SkPath path( pathBuilder.detach() );
    canvas->drawPath( path, paintStroke );

    paintFill.setColor( SkColorSetARGB(0x80, 0x00, 0xFF, 0x00) );
    SkRect rect2 = SkRect::MakeLTRB( 120.0f, 120.0f, 520.0f, 360.0f );
    canvas->drawOval( rect2, paintFill );    
}

int main() {
    printf( "Hello world\n" );

    SkSurface * surface = createSurface( 640, 480 );
    SkCanvas * canvas = surface->getCanvas();

    draw( canvas );

    SkData *png = makeSnapshot( surface );
    SkFILEWStream out("skia-cpp-example.png");
    out.write(png->data(), png->size());

    SkSafeUnref( png );
    SkSafeUnref( surface );
}


