# GraphWidget

Qt-виджет на C++ с аппаратным ускорением OpenGL для визуализации графиков с большим числом точек. Легковесный, не требует зависимостей вне Qt и легко встраивается в любые приложения.

## ✨ Особенности

- **Аппаратная отрисовка:** рендеринг через OpenGL, минимальная нагрузка на процессор.
- **Отрисовка в реальном времени:** точки добавляются на лету; виджет автоматически обновляется.
- **Автоматическое масштабирование:** ось графика подстраивается под новые данные (включая режим следования за последними значениями).
- **Интерактивность:**
    - Перетаскивание области просмотра.
    - Масштабирование колесиком мыши (по осям `X`, `Y` или обеим с модификаторами `Ctrl` / `Shift`).
- **Настройка сетки:** фоновая координатная сетка с выделенными осями.
- **Мультисерийность:** одновременное отображение нескольких графиков с индивидуальным цветом и толщиной линии.
- **Программный API:** полный контроль границ просмотра, зума, смещения, очистки и режимов масштабирования.
- **Интеграция в Qt Designer:** входящий в комплект `graphwidgetplugin` позволяет добавлять виджет через дизайнер.

## 🖥️ Демонстрация

<img width="1875" height="880" alt="image" src="https://github.com/user-attachments/assets/4de8da8d-634f-4930-9f29-1dfe758e15be" />


## 💡 Быстрый старт

```cpp
#include "graphwidget.h"

// Создайте виджет
auto *gw = ui->graphWidget;

// Добавьте новый график (вернёт индекс)
int graphIdx = gw->addGraph({1.0f, 0.5f, 0.0f}); // цвет по желанию

// Добавляйте точки по одной
gw->addPointToGraph(graphIdx, QVector2D(0.0f, 1.2f));
gw->addPointToGraph(graphIdx, QVector2D(1.0f, 2.4f));
gw->addPointToGraph(graphIdx, QVector2D(2.0f, 3.1f));

// Или сразу набором точек
// gw->addGraph(dataVector);  // предварительно подготовленный QVector<QVector2D>
```

Если хотите управлять областью просмотра вручную:

```cpp
gw->setAutoScale(false);           // отключить авто-масштаб
gw->adjustByMinX(-5.0f);
gw->adjustByMaxX(10.0f);
gw->adjustByMinY(-2.0f);
gw->adjustByMaxY(5.0f);
```

## 🔌 Интеграция в ваш проект

### QMake (файл .pro)

Добавьте путь к исходникам виджета и необходимые зависимости:

```qmake
QT       += core gui widgets opengl openglwidgets

IMPORTED_WIDGETS_PATH = $$PWD/external_widgets

INCLUDEPATH += IMPORTED_WIDGETS_PATH

# Обязательно — линковка с OpenGL
win32:LIBS += -L$$IMPORTED_WIDGETS_PATH -lgraphwidget -lopengl32
unix:!macx:LIBS += -L$$IMPORTED_WIDGETS_PATH -lgraphwidget -lGL
macx:LIBS += -L$$IMPORTED_WIDGETS_PATH -lgraphwidget -framework OpenGL
```

### CMake

```cmake
find_package(Qt5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(OpenGL REQUIRED)

function(add_external_lib NAME PATH)
    find_library(LIB_PATH ${NAME} PATHS ${PATH} NO_DEFAULT_PATH)
    add_library(${NAME} STATIC IMPORTED)
    set_target_properties(${NAME} PROPERTIES
        IMPORTED_LOCATION "${LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${PATH}"
    )
    unset(LIB_PATH CACHE)
endfunction()

set(IMPORTED_WIDGETS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external_widgets")
add_external_lib(GraphWidget  ${IMPORTED_WIDGETS_PATH})

target_include_directories(MyApp PRIVATE IMPORTED_WIDGETS_PATH)

target_link_libraries(MyApp PRIVATE
    GraphWidget        # ПЕРЕД остальными
    OpenGL::GL
    Qt::Core
    Qt::Widgets
    Qt6::OpenGLWidgets
)

```

> **Примечание:** Библиотека виджета выделена в отдельную статическую сборку, поэтому в ваш проект включаются двоичные файлы. Зависимость `OpenGL::GL` (или `-lopengl32`) обязательна, так как виджет использует нативный OpenGL для рисования.

## 📖 API

### Создание виджета

`explicit GraphWidget(QWidget *parent = nullptr);`

### График

* `int addGraph(const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 100000);`  
* `int addGraph(const QVector<QVector2D> &data, const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 100000);`  
  * `capacity` — начальная ёмкость буфера.
* `void addPointToGraph(int graphIndex, const QVector2D &point);`
* `void clear();` — очищает все серии.

### Масштаб и границы

* `void setAutoScale(bool is);`
* `void adjustByMinX(float minX, bool isToUpdate = true);`
* `void adjustByMaxX(float maxX, bool isToUpdate = true);`
* `void adjustByMinY(float minY, bool isToUpdate = true);`
* `void adjustByMaxY(float maxY, bool isToUpdate = true);`
* `void setZoom(QVector2D zoom, bool isToUpdate = true);`
* `void setZoomX(float zoom, bool isToUpdate = true);`
* `void setZoomY(float zoom, bool isToUpdate = true);`
* `void setOffset(QVector2D offset, bool isToUpdate = true);`
* `void setOffsetX(float offset, bool isToUpdate = true);`
* `void setOffsetY(float offset, bool isToUpdate = true);`

### Сигналы

* `void initialized();` — виджет готов к работе.
* `void boundariesChanged(float minX, float maxX, float minY, float maxY);`
* `void autoScaleCleared();` — авто-масштаб отключён действием пользователя.

### Режимы

* **Авто-следование (`isFollow`)** — при добавлении точек удерживает видимым последний временной интервал; активируется вручную при изменении границ, когда `isAutoScale` выключен.
* **Авто-масштабирование по Y (`isAutoScaleY`)** — подстраивает вертикальные границы по данным, когда ручное масштабирование отключено.

## 📁 Структура проекта

```
GraphWidget/
├── graphwidget/            # Исходный код виджета
│   ├── graphdata.h         # Внутреннее представление одной серии
│   ├── graphdata.cpp
│   ├── graphwidget.h       # Основной класс виджета
│   ├── graphwidget.cpp
│   └── graphwidget.pro     # Проектный файл
├── graphwidgetplugin/      # Плагин для Qt Designer
│   └── ...
├── dest.pri                # Общие настройки сборки
├── GraphWidget.pro         # Корневой проект (сборка виджета + плагин)
└── README.md
```
