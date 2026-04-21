# Определяем корень проекта (путь к этой папке)
TOP_PWD = $$PWD

# 1. КУДА КЛАСТЬ ГОТОВОЕ (.lib, .dll, .exe)
DESTDIR = $$TOP_PWD/bin

# 2. КУДА КЛАСТЬ МУСОР (объектные файлы, moc, ui, qrc)
CONFIG(debug, debug|release) {
    BUILD_SUBDIR = debug
} else {
    BUILD_SUBDIR = release
}

OBJECTS_DIR = $$TOP_PWD/build/$$TARGET/$$BUILD_SUBDIR/obj
MOC_DIR     = $$TOP_PWD/build/$$TARGET/$$BUILD_SUBDIR/moc
RCC_DIR     = $$TOP_PWD/build/$$TARGET/$$BUILD_SUBDIR/rcc
UI_DIR      = $$TOP_PWD/build/$$TARGET/$$BUILD_SUBDIR/ui
