
cc -o skia-c-example -I ../../.. ../skia-c-example.cpp -L../../../out/macos-x64-debug/ -lskia -framework CoreFoundation -framework CoreGraphics \
      \
     -Wl,-rpath -Wl, ../../../out/macos-x64-debug/
