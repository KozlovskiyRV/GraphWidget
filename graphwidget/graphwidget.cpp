#include "graphwidget.h"

#include <QMatrix4x4>
#include <QOpenGLShader>
#include <QVector4D>
#include <QtMath>

namespace {
constexpr float kMinZoom = 1e-9f;
constexpr float kPaddingRatioY = 0.02f;
}

GraphWidget::GraphWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    grid{10000.0f, 100.0f},
    zoom{1.0f / grid.x(), 1.0f / grid.y()},
    offset{-1.0f, -0.5f} {
}

GraphWidget::~GraphWidget() {
    makeCurrent();
    m_gridVBO.destroy();
    shaderProgram.removeAllShaders();
    for (auto *g : graphs)
        delete g;
    doneCurrent();
}

int GraphWidget::addGraph(const QVector<QVector2D> &data, const QVector3D color, float lineWidth, size_t capacity) {
    makeCurrent();
    auto *gd = new GraphData{data, color, lineWidth, capacity};
    graphs.append(gd);
    doneCurrent();
    return graphs.size() - 1;
}
int GraphWidget::addGraph(const QVector3D color, float lineWidth, size_t capacity) {
    return addGraph({}, color, lineWidth, capacity);
}

void GraphWidget::addPointToGraph(int graphIndex, const QVector2D &point) {
    if (graphIndex < 0 || graphIndex >= graphs.size()) {
        qWarning("Invalid graphIndex");
        return;
    }

    makeCurrent();
    graphs[graphIndex]->appendPoint(point);
    doneCurrent();

    if (!isPointsPresent) {
        chartRect = QRectF(point.x(), point.y(), 0, 0);
        isPointsPresent = true;
    } else {
        chartRect = chartRect.united(QRectF(point.x(), point.y(), 0, 0));
    }
// pointMinX, pointMaxX, pointMinY, pointMaxY - chartRect
// maxX, minX, maxY, minY - widgetRect
    if (isAutoScale) {
        const float paddingY = kPaddingRatioY * qMax(chartRect.height(), 1.0f);
        adjustByMinX(chartRect.left(), false);
        adjustByMaxX(chartRect.right(), false);
        adjustByMinY(chartRect.bottom() - paddingY, false);
        adjustByMaxY(chartRect.top() + paddingY, false);
    } else {
        if (isFollow && lastVisiblePeriod > 0.0f) {
            const float periodMinX = (chartRect.width() > lastVisiblePeriod) ? (chartRect.right() - lastVisiblePeriod) : chartRect.left();
            adjustByMinX(periodMinX, false);
            adjustByMaxX(periodMinX + lastVisiblePeriod, false);
        }
        if (isAutoScaleY) {
            const float paddingY = kPaddingRatioY * qMax(chartRect.height(), 1.0f);
            adjustByMinY(chartRect.left() - paddingY, false);
            adjustByMaxY(chartRect.right() + paddingY, false);
        }
    }

    update();
}

void GraphWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        uniform mat4 transform;
        void main() {
            gl_Position = transform * vec4(position, 0.0, 1.0);
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        uniform vec3 color;
        out vec4 fragColor;
        void main() {
            fragColor = vec4(color, 1.0);
        }
    )";

    if (!shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)
        || !shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)
        || !shaderProgram.link()) {
        qWarning("Error while creating shader program");
    }

    m_gridVBO.create();
    m_gridVBO.bind();
    m_gridVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_gridVBO.release();

    emit initialized();
}

void GraphWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

inline void GraphWidget::evalBoundaries() {
    widgetRect = (QRectF(-1.0f, -1.0f, 2.0f, 2.0f) - offset) / zoom;

    QVector<QVector2D> gridLines;
    // Вертикальные линии сетки
    float startX = std::floor(widgetRect.left() / grid.x()) * grid.x();
    for (float x = startX; x <= widgetRect.right(); x += grid.x()) {
        gridLines.append(QVector2D(x, widgetRect.top()));
        gridLines.append(QVector2D(x, widgetRect.bottom()));
    }
    // Горизонтальные линии сетки
    float startY = std::floor(widgetRect.top() / grid.y()) * grid.y();
    for (float y = startY; y <= widgetRect.bottom(); y += grid.y()) {
        gridLines.append(QVector2D(widgetRect.left(), y));
        gridLines.append(QVector2D(widgetRect.right(), y));
    }

    // Оси
    gridLines.append(QVector2D(widgetRect.left(), 0));
    gridLines.append(QVector2D(widgetRect.right(), 0));
    gridLines.append(QVector2D(0, widgetRect.top()));
    gridLines.append(QVector2D(0, widgetRect.bottom()));

    makeCurrent();
    m_gridVBO.bind();
    m_gridVBO.allocate(gridLines.constData(), gridLines.size() * sizeof(QVector2D));
    m_gridVBO.release();
    doneCurrent();
}

void GraphWidget::markBoundariesChanged() {
    areBoundariesChanged = true;
    evalBoundaries();
}

void GraphWidget::paintGL() {
    QMatrix4x4 transform;
    transform.ortho(-1, 1, -1, 1, -1, 1);
    transform.translate(offset.x(), offset.y());
    transform.scale(zoom.x(), zoom.y());

    if (areBoundariesChanged) {
        areBoundariesChanged = false;
        emit boundariesChanged(widgetRect.left(), widgetRect.right(), widgetRect.top(), widgetRect.bottom());
    }

    glClear(GL_COLOR_BUFFER_BIT);

    shaderProgram.bind();
    shaderProgram.setUniformValue("transform", transform);

    int positionLocation = shaderProgram.attributeLocation("position");

    // Рисование сетки
    m_gridVBO.bind();
        shaderProgram.enableAttributeArray(positionLocation);
        shaderProgram.setAttributeBuffer(positionLocation, GL_FLOAT, 0, 2, 0);

        glLineWidth(0.7f);
        shaderProgram.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.7f));
        int gridVertexCount = m_gridVBO.size() / sizeof(QVector2D) - 4;
        glDrawArrays(GL_LINES, 0, gridVertexCount);

        glLineWidth(2.0f);
        shaderProgram.setUniformValue("color", QVector3D(1, 1, 1));
        glDrawArrays(GL_LINES, gridVertexCount, 4);
        shaderProgram.disableAttributeArray(positionLocation);
    m_gridVBO.release();

    for (auto *graph : graphs)
        graph->render(shaderProgram, positionLocation);

    shaderProgram.release();
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
    }
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!isDragging) {
        return;
    }

    isAutoScale = false;
    emit autoScaleCleared();

    const QPointF delta = event->position() - lastMousePos;
    if (!isFollow && width() > 0)
        offset.setX(offset.x() + 2.0f * delta.x() / width());
    if (!isAutoScaleY && height() > 0)
        offset.setY(offset.y() - 2.0f * delta.y() / height());

    lastMousePos = event->position();
    markBoundariesChanged();
    update();
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    isAutoScale = false;
    emit autoScaleCleared();

    const float scaleFactor = (event->angleDelta().y() > 0) ? 1.1f : 0.9f;
    const QPointF mouseCenter = event->position();

    const QPointF s{2.0f * mouseCenter.x() / width() - 1.0f,
                    1.0f - 2.0f * mouseCenter.y() / height()};

    const QPointF center = (s - offset) / zoom;

    if (event->modifiers() == Qt::ControlModifier) {
        setZoomX(zoom.x() * scaleFactor);
    } else if (event->modifiers() == Qt::ShiftModifier) {
        setZoomY(zoom.y() * scaleFactor);
    } else {
        setZoom(zoom * scaleFactor);
    }

    QPointF r = s - center * zoom;
    offset.setX(r.x());
    offset.setY(r.y());

    markBoundariesChanged();
    update();
}

void GraphWidget::clear() {
    isAutoScale = false;
    emit autoScaleCleared();
    isPointsPresent = false;

    for (auto *graph : graphs)
        graph->clear();

    update();
}

void GraphWidget::adjustByMinX(float newMinX, bool isToUpdate) {
    if (widgetRect.left() > newMinX) {
        zoom.setX( 2.0f / (widgetRect.right() - newMinX) );
        offset.setX( -1.0f - newMinX * zoom.x() );
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMaxX(float newMaxX, bool isToUpdate) {
    if (newMaxX > widgetRect.left()) {
        zoom.setX( 2.0f / (newMaxX - widgetRect.left()) );
        offset.setX( -1.0f - widgetRect.left() * zoom.x() );
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMinY(float newMinY, bool isToUpdate) {
    if (widgetRect.bottom() > newMinY) {
        zoom.setY( 2.0f / (widgetRect.bottom() - newMinY) );
        offset.setY( -1.0f - newMinY * zoom.y() );
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMaxY(float newMaxY, bool isToUpdate) {
    if (newMaxY > widgetRect.top()) {
        zoom.setY( 2.0f / (newMaxY - widgetRect.top()) );
        offset.setY( -1.0f - widgetRect.top() * zoom.y() );
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::setAutoScale(bool is) {
    isAutoScale = is;
    if (!is || !isPointsPresent) {
        return;
    }

    const float paddingY = kPaddingRatioY * qMax(chartRect.height(), 1.0f);
    adjustByMinX(chartRect.left(), false);
    adjustByMaxX(chartRect.right(), false);
    adjustByMinY(chartRect.bottom() - paddingY, false);
    adjustByMaxY(chartRect.top() + paddingY, false);

    update();
}

void GraphWidget::setZoom(QVector2D newZoom, bool isToUpdate) {
    zoom.setX( qMax(qAbs(newZoom.x()), kMinZoom) );
    zoom.setY( qMax(qAbs(newZoom.y()), kMinZoom) );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}

void GraphWidget::setZoomX(float zoomX, bool isToUpdate) {
    zoom.setX( qMax(qAbs(zoomX), kMinZoom) );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}

void GraphWidget::setZoomY(float zoomY, bool isToUpdate) {
    zoom.setY( qMax(qAbs(zoomY), kMinZoom) );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}

void GraphWidget::setOffset(QVector2D newOffset, bool isToUpdate) {
    offset.setX( newOffset.x() );
    offset.setY( newOffset.y() );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}

void GraphWidget::setOffsetX(float offsetX, bool isToUpdate) {
    offset.setX( offsetX );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}

void GraphWidget::setOffsetY(float offsetY, bool isToUpdate) {
    offset.setY( offsetY );
    markBoundariesChanged();

    if (isToUpdate) {
        isAutoScale = false;
        emit autoScaleCleared();
        update();
    }
}
