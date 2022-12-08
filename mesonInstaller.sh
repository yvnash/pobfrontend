#!/bin/bash

cp -rf ${MESON_SOURCE_ROOT}/PathOfBuildingBuild/* ${MESON_INSTALL_PREFIX}/Contents/MacOS/
cp -f ${MESON_SOURCE_ROOT}/fonts/*.ttf ${MESON_INSTALL_PREFIX}/Contents/MacOS/

mkdir -p ${MESON_INSTALL_PREFIX}/Contents/Frameworks

# This is an attempted fix to the error on old Mac versions:
# "Symbol not found: _curl_easy_option_by_id" since that didn't exist in older
# cURL libraries.
#
# Copy over the library 
cp -f "$(brew --prefix)/opt/curl/lib/libcurl.4.dylib" ${MESON_INSTALL_PREFIX}/Contents/Frameworks
# Change the reference to refer to the new library path
install_name_tool -change /usr/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ${MESON_INSTALL_PREFIX}/Contents/MacOS/lcurl.so
