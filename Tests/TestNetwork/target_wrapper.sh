#!/bin/sh
PATH=/C/Qt/Qt5.12.8/5.12.8/mingw73_32/bin:$PATH
export PATH
QT_PLUGIN_PATH=/C/Qt/Qt5.12.8/5.12.8/mingw73_32/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
export QT_PLUGIN_PATH
exec "$@"
