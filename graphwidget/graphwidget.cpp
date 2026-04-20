#include "graphwidget.h"

#include <QMatrix4x4>
#include <QOpenGLShader>
#include <QVector4D>
#include <QtMath>
#include <algorithm>

namespace {
constexpr float kMinZoom = 1e-9f;
constexpr float kPaddingRatioY = 0.02f;
}

GraphData::GraphData()
    : vbo(QOpenGLBuffer::VertexBuffer) {
}

void GraphData::initializeGl() {
    if (!vbo.isCreated()) {
        vbo.create();
    }
    vbo.bind();
    vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    if (m_capacity > 0) {
        vbo.allocate(m_capacity * static_cast<int>(sizeof(QVector2D)));
        uploadFull();
    }
    vbo.release();
}

void GraphData::setCapacity(int newCapacity) {
    m_capacity = std::max(1, newCapacity);
    m_points.resize(m_capacity);
    m_head = 0;
    m_size = 0;

    if (vbo.isCreated()) {
        vbo.bind();
        vbo.allocate(m_capacity * static_cast<int>(sizeof(QVector2D)));
        vbo.release();
    }
}

void GraphData::setPoints(const QVector<QVector2D> &newPoints) {
    if (m_capacity <= 0) {
        return;
    }

    const int keepCount = std::min(static_cast<int>(newPoints.size()), m_capacity);
    m_head = 0;
    m_size = keepCount;

    const int start = newPoints.size() - keepCount;
    for (int i = 0; i < keepCount; ++i) {
        m_points[i] = newPoints[start + i];
    }

    uploadFull();
}

void GraphData::appendPoint(const QVector2D &point) {
    if (m_capacity <= 0 || !vbo.isCreated()) {
        return;
    }

    if (m_size < m_capacity) {
        const int tail = (m_head + m_size) % m_capacity;
        m_points[tail] = point;
        ++m_size;

        vbo.bind();
        vbo.write(tail * static_cast<int>(sizeof(QVector2D)), &point, static_cast<int>(sizeof(QVector2D)));
        vbo.release();
        return;
    }

    m_points[m_head] = point;
    vbo.bind();
    vbo.write(m_head * static_cast<int>(sizeof(QVector2D)), &point, static_cast<int>(sizeof(QVector2D)));
    vbo.release();
    m_head = (m_head + 1) % m_capacity;
}

int GraphData::size() const {
    return m_size;
}

int GraphData::firstChunkSize() const {
    if (m_size == 0) {
        return 0;
    }

    return std::min(m_size, m_capacity - m_head);
}

int GraphData::secondChunkSize() const {
    return m_size - firstChunkSize();
}

int GraphData::firstChunkStartIndex() const {
    return m_head;
}

const QVector<QVector2D> &GraphData::points() const {
    return m_points;
}

void GraphData::clear() {
    m_head = 0;
    m_size = 0;
}

void GraphData::uploadFull() {
    if (!vbo.isCreated()) {
        return;
    }

    vbo.bind();
    if (m_size > 0) {
        vbo.write(0, m_points.constData(), m_size * static_cast<int>(sizeof(QVector2D)));
    }
    vbo.release();
}

GraphWidget::GraphWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , overlayVbo(QOpenGLBuffer::VertexBuffer) {
}

GraphWidget::~GraphWidget() {
    makeCurrent();
    overlayVbo.destroy();
    for (GraphData &graph : graphs) {
        graph.vbo.destroy();
    }
    doneCurrent();
}

int GraphWidget::addGraph(const QVector<QVector2D> &data, const QVector3D &color, float lineWidth, int capacity) {
    makeCurrent();

    GraphData graph;
    graph.color = color;
    graph.lineWidth = lineWidth;
    graph.setCapacity(capacity);
    graph.initializeGl();
    graph.setPoints(data);

    graphs.append(std::move(graph));

    doneCurrent();
    update();
    return graphs.size() - 1;
}

void GraphWidget::addPointToGraph(int graphIndex, const QVector2D &point) {
    if (graphIndex < 0 || graphIndex >= graphs.size()) {
        qWarning("Invalid graphIndex");
        return;
    }

    GraphData &graph = graphs[graphIndex];
    graph.appendPoint(point);

    const float x = point.x();
    const float y = point.y();

    if (isPointsPresent) {
        pointMaxX = std::max(pointMaxX, x);
        pointMinX = std::min(pointMinX, x);
        pointMaxY = std::max(pointMaxY, y);
        pointMinY = std::min(pointMinY, y);
    } else {
        isPointsPresent = true;
        pointMaxX = x;
        pointMinX = x;
        pointMaxY = y;
        pointMinY = y;
    }

    if (isAutoScale) {
        const float paddingY = kPaddingRatioY * std::max(qAbs(pointMaxY - pointMinY), 1.0f);
        adjustByMinX(pointMinX, false);
        adjustByMaxX(pointMaxX, false);
        adjustByMinY(pointMinY - paddingY, false);
        adjustByMaxY(pointMaxY + paddingY, false);
    } else {
        if (isFollow && lastVisiblePeriod > 0.0f) {
            const float periodMinX = (pointMaxX - pointMinX > lastVisiblePeriod) ? (pointMaxX - lastVisiblePeriod) : pointMinX;
            adjustByMinX(periodMinX, false);
            adjustByMaxX(periodMinX + lastVisiblePeriod, false);
        }
        if (isAutoScaleY) {
            const float paddingY = kPaddingRatioY * std::max(qAbs(pointMaxY - pointMinY), 1.0f);
            adjustByMinY(pointMinY - paddingY, false);
            adjustByMaxY(pointMaxY + paddingY, false);
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

    overlayVbo.create();
    overlayVbo.bind();
    overlayVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    overlayVbo.release();

    for (GraphData &graph : graphs) {
        graph.initializeGl();
    }

    emit initialized();
}

void GraphWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GraphWidget::evalBoundaries() {
    const float safeZoomX = (qAbs(zoomX) < kMinZoom) ? ((zoomX >= 0.0f) ? kMinZoom : -kMinZoom) : zoomX;
    const float safeZoomY = (qAbs(zoomY) < kMinZoom) ? ((zoomY >= 0.0f) ? kMinZoom : -kMinZoom) : zoomY;

    minX = (-1.0f - offsetX) / safeZoomX;
    maxX = (1.0f - offsetX) / safeZoomX;
    minY = (-1.0f - offsetY) / safeZoomY;
    maxY = (1.0f - offsetY) / safeZoomY;

    if (minX > maxX) {
        std::swap(minX, maxX);
    }
    if (minY > maxY) {
        std::swap(minY, maxY);
    }
}

void GraphWidget::markBoundariesChanged() {
    areBoundariesChanged = true;
    evalBoundaries();
}

void GraphWidget::emitBoundariesIfNeeded() {
    if (!areBoundariesChanged) {
        return;
    }

    areBoundariesChanged = false;
    emit boundariesChanged(minX, maxX, minY, maxY);
}

void GraphWidget::drawOverlay(const QMatrix4x4 &transform) {
    QVector<QVector2D> lines;

    const float startX = std::floor(minX / gridX) * gridX;
    for (float x = startX; x <= maxX; x += gridX) {
        lines.append({x, minY});
        lines.append({x, maxY});
    }

    const float startY = std::floor(minY / gridY) * gridY;
    for (float y = startY; y <= maxY; y += gridY) {
        lines.append({minX, y});
        lines.append({maxX, y});
    }

    lines.append({0.0f, minY});
    lines.append({0.0f, maxY});
    lines.append({minX, 0.0f});
    lines.append({maxX, 0.0f});

    overlayVbo.bind();
    overlayVbo.allocate(lines.constData(), lines.size() * static_cast<int>(sizeof(QVector2D)));

    shaderProgram.setUniformValue("transform", transform);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    const int gridVertices = lines.size() - 4;
    shaderProgram.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.7f));
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, gridVertices);

    shaderProgram.setUniformValue("color", QVector3D(1.0f, 1.0f, 1.0f));
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, gridVertices, 4);

    glDisableVertexAttribArray(0);
    overlayVbo.release();
}

void GraphWidget::paintGL() {
    const float safeZoomX = std::max(qAbs(zoomX), kMinZoom);
    const float safeZoomY = std::max(qAbs(zoomY), kMinZoom);
    zoomX = (zoomX >= 0.0f) ? safeZoomX : -safeZoomX;
    zoomY = (zoomY >= 0.0f) ? safeZoomY : -safeZoomY;

    QMatrix4x4 transform;
    transform.ortho(-1, 1, -1, 1, -1, 1);
    transform.translate(offsetX, offsetY);
    transform.scale(zoomX, zoomY);

    evalBoundaries();
    emitBoundariesIfNeeded();

    glClear(GL_COLOR_BUFFER_BIT);

    shaderProgram.bind();
    drawOverlay(transform);

    shaderProgram.setUniformValue("transform", transform);

    for (GraphData &graph : graphs) {
        if (graph.size() == 0) {
            continue;
        }

        shaderProgram.setUniformValue("color", graph.color);
        glLineWidth(graph.lineWidth);

        graph.vbo.bind();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        const int firstSize = graph.firstChunkSize();
        const int secondSize = graph.secondChunkSize();
        const int firstStart = graph.firstChunkStartIndex();

        if (firstSize > 1) {
            glDrawArrays(GL_LINE_STRIP, firstStart, firstSize);
        }
        if (secondSize > 1) {
            glDrawArrays(GL_LINE_STRIP, 0, secondSize);
        }

        glDisableVertexAttribArray(0);
        graph.vbo.release();
    }

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

    const QPointF delta = event->position() - QPointF(lastMousePos);
    if (!isFollow && width() > 0) {
        offsetX += 2.0f * static_cast<float>(delta.x()) / static_cast<float>(width());
    }
    if (!isAutoScaleY && height() > 0) {
        offsetY -= 2.0f * static_cast<float>(delta.y()) / static_cast<float>(height());
    }

    lastMousePos = event->position().toPoint();
    markBoundariesChanged();
    update();
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    isAutoScale = false;
    emit autoScaleCleared();

    const float scaleFactor = (event->angleDelta().y() > 0) ? 1.1f : 0.9f;
    const QPointF mouseCenter = event->position();

    const float sx = 2.0f * static_cast<float>(mouseCenter.x()) / std::max(1, width()) - 1.0f;
    const float sy = 1.0f - 2.0f * static_cast<float>(mouseCenter.y()) / std::max(1, height());

    const float centerX = (sx - offsetX) / std::max(qAbs(zoomX), kMinZoom);
    const float centerY = (sy - offsetY) / std::max(qAbs(zoomY), kMinZoom);

    if (event->modifiers() == Qt::ControlModifier) {
        zoomX *= scaleFactor;
    } else if (event->modifiers() == Qt::ShiftModifier) {
        zoomY *= scaleFactor;
    } else {
        zoomX *= scaleFactor;
        zoomY *= scaleFactor;
    }

    offsetX = sx - centerX * zoomX;
    offsetY = sy - centerY * zoomY;

    markBoundariesChanged();
    update();
}

void GraphWidget::clear() {
    isAutoScale = false;
    emit autoScaleCleared();
    isPointsPresent = false;

    for (GraphData &graph : graphs) {
        graph.clear();
    }

    update();
}

void GraphWidget::adjustByMinX(float newMinX, bool isToUpdate) {
    if (maxX > newMinX) {
        zoomX = 2.0f / (maxX - newMinX);
        offsetX = -1.0f - newMinX * zoomX;
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMaxX(float newMaxX, bool isToUpdate) {
    if (newMaxX > minX) {
        zoomX = 2.0f / (newMaxX - minX);
        offsetX = -1.0f - minX * zoomX;
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMinY(float newMinY, bool isToUpdate) {
    if (maxY > newMinY) {
        zoomY = 2.0f / (maxY - newMinY);
        offsetY = -1.0f - newMinY * zoomY;
        markBoundariesChanged();

        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        }
    }
}

void GraphWidget::adjustByMaxY(float newMaxY, bool isToUpdate) {
    if (newMaxY > minY) {
        zoomY = 2.0f / (newMaxY - minY);
        offsetY = -1.0f - minY * zoomY;
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

    const float paddingY = kPaddingRatioY * std::max(qAbs(pointMaxY - pointMinY), 1.0f);
    adjustByMinX(pointMinX, false);
    adjustByMaxX(pointMaxX, false);
    adjustByMinY(pointMinY - paddingY, false);
    adjustByMaxY(pointMaxY + paddingY, false);

    update();
}

void GraphWidget::setZoom(QVector2D zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomX = zoom.x();
    zoomY = zoom.y();
    markBoundariesChanged();
    update();
}

void GraphWidget::setZoomX(float zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomX = zoom;
    markBoundariesChanged();
    update();
}

void GraphWidget::setZoomY(float zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomY = zoom;
    markBoundariesChanged();
    update();
}

void GraphWidget::setOffset(QVector2D offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetX = offset.x();
    offsetY = offset.y();
    markBoundariesChanged();
    update();
}

void GraphWidget::setOffsetX(float offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetX = offset;
    markBoundariesChanged();
    update();
}

void GraphWidget::setOffsetY(float offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetY = offset;
    markBoundariesChanged();
    update();
}
