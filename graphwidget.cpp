#include "graphwidget.h"
#include <QOpenGLShader>
#include <QMatrix4x4>
#include <QDebug>

GraphWidget::GraphWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
}

int GraphWidget::addGraph(const QVector<QVector2D> &data, const QVector3D &color, float lineWidth, int capacity) {
    makeCurrent();
    GraphData graph;
    graph.points = data;
    graph.color = color;
    graph.lineWidth = lineWidth;
    graph.capacity = capacity;

    graph.vbo.create();
    graph.vbo.bind();
    graph.vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    graph.vbo.allocate(capacity * sizeof(QVector2D));

    if (!data.isEmpty())
        graph.vbo.write(0, data.constData(), data.size() * sizeof(QVector2D));

    graph.vbo.release();
    graphs.append(graph);
    return graphs.size() - 1;
}

void GraphWidget::addPointToGraph(int graphIndex, const QVector2D &point) {
    if (graphIndex < 0 || graphIndex >= graphs.size()) {
        qWarning() << "Invalid graphIndex";
        return;
    }

    GraphData &graph = graphs[graphIndex];

    //Если достигнут передел ёмкости, то сдвинуть данные
    if (graph.points.size() >= graph.capacity) {
        graph.points.pop_front();
        updateGraphBuffer(graphIndex);
    }
    // Записать в VBO только новую точку
    graph.points.append(point);
    int offset = (graph.points.size() - 1) * sizeof(QVector2D);
    graph.vbo.bind();
    graph.vbo.write(offset, &point, sizeof(QVector2D));
    graph.vbo.release();

    float x = point.x();
    float y = point.y();

    if (isPointsPresent) {
        if (x > pointMaxX)
            pointMaxX = x;
        else if (x < pointMinX)
            pointMinX = x;
        if (y > pointMaxY)
            pointMaxY = y;
        else if (y < pointMinY)
            pointMinY = y;
    } else {
        isPointsPresent = true;
        pointMaxX = x;
        pointMinX = x;
        pointMaxY = y;
        pointMinY = y;
    }

    if (isAutoScale) {
        areBoundariesChanged = true;
        if (pointMaxX != maxX)
            adjustByMaxX(pointMaxX, false);
        if (pointMinX != minX)
            adjustByMinX(pointMinX, false);
        float paddingY = 0.02f * qMax(fabs(pointMaxY - pointMinY), 1.0f);
        if (pointMaxY + paddingY != maxY)
            adjustByMaxY(pointMaxY + paddingY, false);
        if (pointMinY - paddingY != minY)
            adjustByMinY(pointMinY - paddingY, false);
    } else {
        if (isFollow) {
            adjustByMinX(pointMaxX-pointMinX>lastVisiblePeriod? pointMaxX-lastVisiblePeriod : pointMinX, false);
            adjustByMaxX(pointMaxX-pointMinX>lastVisiblePeriod? pointMaxX : pointMinX+lastVisiblePeriod, false);
        }
        if (isAutoScaleY) {
            float paddingY = 0.02f * qMax(fabs(pointMaxY - pointMinY), 1.0f);
            if (pointMaxY + paddingY != maxY)
                adjustByMaxY(pointMaxY + paddingY, false);
            if (pointMaxY - paddingY != minY)
                adjustByMaxY(pointMinY - paddingY, false);
        }
    }

    update();
}

void GraphWidget::updateGraphBuffer(int graphIndex) {
    if (graphIndex < 0 || graphIndex >= graphs.size())
        return;

    GraphData &graph = graphs[graphIndex];
    graph.vbo.bind();
    graph.vbo.write(0, graph.points.constData(), graph.points.size() * sizeof(QVector2D));
    graph.vbo.release();
}

void GraphWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    const QString vertexShaderSource = R"(
            #version 120
            attribute vec2 position;
            uniform mat4 transform;
            void main() {
                gl_Position = transform * vec4(position, 0.0, 1.0);
            }
                                        )";
    const QString fragmentShaderSource = R"(
            #version 120
            uniform vec3 color;
            void main() {
                gl_FragColor = vec4(color, 1.0);
            }
                                        )";
    shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);

    if (!shaderProgram.link())
        qWarning() << "Error program linking";

    emit initialized();
}

void GraphWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GraphWidget::evalBoundaries(QMatrix4x4 &transform) {
    QMatrix4x4 invTransform = transform.inverted();
    QVector4D topLeftNDC(-1, 1, 0, 1);
    QVector4D bottomRightNDC(1, -1, 0, 1);
    QVector4D topLeft = invTransform * topLeftNDC;
    QVector4D bottomRight = invTransform * bottomRightNDC;
    minX = topLeft.x();
    maxX = bottomRight.x();
    minY = bottomRight.y();
    maxY = topLeft.y();
}
void GraphWidget::evalBoundaries() {
    QMatrix4x4 transform;
    transform.ortho(-1, 1, -1, 1, -1, 1);
    transform.translate(offsetX, offsetY);
    transform.scale(zoomX, zoomY);

    QMatrix4x4 invTransform = transform.inverted();
    QVector4D topLeftNDC(-1, 1, 0, 1);
    QVector4D bottomRightNDC(1, -1, 0, 1);
    QVector4D topLeft = invTransform * topLeftNDC;
    QVector4D bottomRight = invTransform * bottomRightNDC;
    minX = topLeft.x();
    maxX = bottomRight.x();
    minY = bottomRight.y();
    maxY = topLeft.y();
}

void GraphWidget::paintGL() {
    QMatrix4x4 transform;
    transform.ortho(-1, 1, -1, 1, -1, 1);
    transform.translate(offsetX, offsetY);
    transform.scale(zoomX, zoomY);

    evalBoundaries(transform);

    if (areBoundariesChanged) {
        areBoundariesChanged = false;
        emit boundariesChanged(minX, maxX, minY, maxY);
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(transform.constData());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.7f, 0.7f, 0.7f);
    glLineWidth(0.7f);

    glBegin(GL_LINES);
    for (float x = std::floor(minX / gridX) * gridX; x <= maxX; x += gridX) {
        glVertex2f(x, minY);
        glVertex2f(x, maxY);
    }
    for (float y = std::floor(minY / gridY) * gridY; y <= maxY; y += gridY) {
        glVertex2f(minX, y);
        glVertex2f(maxX, y);
    }
    glEnd();

    glLineWidth(2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, minY);
    glVertex2f(0.0f, maxY);
    glVertex2f(minX, 0.0f);
    glVertex2f(maxX, 0.0f);
    glEnd();

    shaderProgram.bind();
    shaderProgram.setUniformValue("transform", transform);

    int positionLocation = shaderProgram.attributeLocation("position");

    for (int i = 0; i < graphs.size(); i++) {
        GraphData &graph = graphs[i];
        shaderProgram.setUniformValue("color", graph.color);
        glLineWidth(graph.lineWidth);

        graphs[i].vbo.bind();
        shaderProgram.enableAttributeArray(positionLocation);
        shaderProgram.setAttributeArray(positionLocation, GL_FLOAT, 0, 2, 0);

        glDrawArrays(GL_LINE_STRIP, 0, graph.points.size());

        shaderProgram.disableAttributeArray(positionLocation);
        graphs[i].vbo.release();
    }

    shaderProgram.release();

//    QPainter painter(this);
//    painter.setRenderHint(QPainter::Antialiasing);
//    QPen gridPen(Qt::lightGray);
//    gridPen.setStyle(Qt::DashLine);
//    gridPen.setWidth(1);
//    painter.setPen(gridPen);

//    auto worldToWidget = [this, &transform](float wx, float wy) -> QPointF {
//        QVector4D v = transform * QVector4D(wx, wy, 0, 1);
//        float sx = (v.x() + 1.0f) * 0.5f * width();
//        float sy = (1.0f - (v.y() + 1.0f) * 0.5f) * height();
//        return QPointF(sx, sy);
//    };

//    for (float x = std::floor(minX / stepX) * stepX; x <= maxX; x += stepX)
//        painter.drawLine(worldToWidget(x, minY), worldToWidget(x, maxY));

//    for (float y = std::floor(minY / stepY) * stepY; y <= maxY; y += stepY)
//        painter.drawLine(worldToWidget(minX, y), worldToWidget(maxX, y));
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
    QPointF mouseCenter = event->pos();
//    float sx = 2.0f * mouseCenter.x() / float(width()) - 1.0f;
//    float sy = 1.0f - 2.0f * mouseCenter.y() / float(height());
//    float centerX = (sx - offsetX) / zoomX;
//    float centerY = (sy - offsetY) / zoomY;
//    qDebug() << centerX << centerY;
    if (isDragging) {
        isAutoScale = false;
        emit autoScaleCleared();
        QPointF delta = mouseCenter - lastMousePos;
        if (!isFollow)
            offsetX += 2.0f * delta.x() / float(width());
        if (!isAutoScaleY)
            offsetY -= 2.0f * delta.y() / float(height());
        lastMousePos = event->pos();
        areBoundariesChanged = true;
        update();
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    isAutoScale = false;
    emit autoScaleCleared();

    float scaleFactor = event->delta() > 0? 1.1f : 0.9f;
    QPointF mouseCenter = event->pos();
    float sx = 2.0f * mouseCenter.x() / float(width()) - 1.0f;
    float sy = 1.0f - 2.0f * mouseCenter.y() / float(height());
    float centerX = (sx - offsetX) / zoomX;
    float centerY = (sy - offsetY) / zoomY;

    switch (event->modifiers()) {
    case Qt::ControlModifier:
        zoomX *= scaleFactor;
        break;
    case Qt::ShiftModifier:
        zoomY *= scaleFactor;
        break;
    default:
        zoomX *= scaleFactor;
        zoomY *= scaleFactor;
        break;
    }

    offsetX = sx - centerX * zoomX;
    offsetY = sy - centerY * zoomY;
    areBoundariesChanged = true;
    update();
}

void GraphWidget::clear() {
    isAutoScale = false;
    emit autoScaleCleared();
    isPointsPresent = false;
    for (GraphData &graph : graphs) {
        graph.points.clear();
        graph.vbo.bind();
        graph.vbo.write(0, graph.points.constData(), graph.points.size() * sizeof(QVector2D));
        graph.vbo.release();
    }
    update();
}

void GraphWidget::adjustByMinX(float newMinX, bool isToUpdate) {
    if (maxX > newMinX) {
        zoomX = 2.0f / (maxX - newMinX);
        offsetX = -1.0f - newMinX * zoomX;
        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        } else
            evalBoundaries();
    }
    areBoundariesChanged = true;
}
void GraphWidget::adjustByMaxX(float newMaxX, bool isToUpdate) {
    if (newMaxX > minX) {
        zoomX = 2.0f / (newMaxX - minX);
        offsetX = -1.0f - minX * zoomX;
        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        } else
            evalBoundaries();
    }
    areBoundariesChanged = true;
}

void GraphWidget::adjustByMinY(float newMinY, bool isToUpdate) {
    if (maxY > newMinY) {
        zoomY = 2.0f / (maxY - newMinY);
        offsetY = -1.0f - newMinY * zoomY;
        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        } else
            evalBoundaries();
    }
    areBoundariesChanged = true;
}
void GraphWidget::adjustByMaxY(float newMaxY, bool isToUpdate) {
    if (newMaxY > minY) {
        zoomY = 2.0f / (newMaxY - minY);
        offsetY = -1.0f - minY * zoomY;
        if (isToUpdate) {
            isAutoScale = false;
            emit autoScaleCleared();
            update();
        } else
            evalBoundaries();
    }
    areBoundariesChanged = true;
}

void GraphWidget::setAutoScale(bool is) {
    isAutoScale = is;
    if (!isPointsPresent || !is)
        return;
    float paddingY = 0.02f * fabs(pointMaxY - pointMinY);
    adjustByMaxY(pointMaxY + paddingY, false);
    adjustByMinY(pointMinY - paddingY, false);
    adjustByMaxX(pointMaxX, false);
    adjustByMinX(pointMinX, false);
    update();
}

void GraphWidget::setZoom(QVector2D zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomX = zoom.x();
    zoomY = zoom.y();
    update();
}
void GraphWidget::setZoomX(float zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomX = zoom;
    update();
}
void GraphWidget::setZoomY(float zoom) {
    isAutoScale = false;
    emit autoScaleCleared();
    zoomY = zoom;
    update();
}
void GraphWidget::setOffset(QVector2D offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetX = offset.x();
    offsetY = offset.y();
    update();
}
void GraphWidget::setOffsetX(float offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetX = offset;
    update();
}
void GraphWidget::setOffsetY(float offset) {
    isAutoScale = false;
    emit autoScaleCleared();
    offsetY = offset;
    update();
}
/*
GraphWidget::GraphWidget(QWidget *parent)
    : QOpenGLWidget(parent), vertexBuffer(QOpenGLBuffer::VertexBuffer) {}

void GraphWidget::setMaxPoints(int maxPoints) {
    this->maxPoints = maxPoints;
}

void GraphWidget::addData(float x, float y) {
    if (graphData.size() >= maxPoints)
        graphData.pop_front();
    graphData.append({x, y});
    updateBuffer();
    update();
}

void GraphWidget::setData(const QVector<float> &data) {
    //graphData = data;
    update();
}

void GraphWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(1, 1, 1, 1);

    shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                          R"(
                                            #version 330 core
                                            layout(location = 0) in vec2 position;
                                            uniform vec2 zoom;
                                            uniform vec2 offset;
                                            out vec2 fragColor;
                                            void main() {
                                                gl_Position = vec4((position.x + offset.x) * zoom.x, (position.y + offset.y) * zoom.y, 0.0, 1.0);
                                            }
                                          )");
    shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          R"(
                                            #version 330 core
                                            out vec4 color;
                                            void main() {
                                                color = vec4(0.0, 0.0, 1.0, 1.0);
                                            }
                                          )");

    shaderProgram.link();
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

void GraphWidget::updateBuffer() {
    if (graphData.isEmpty())
        return;

    vertexBuffer.bind();
    vertexBuffer.allocate(graphData.constData(), graphData.size() * sizeof(Point));
}

void GraphWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (graphData.isEmpty())
        return;

    shaderProgram.bind();
    vertexBuffer.bind();

    shaderProgram.setUniformValue("zoom", QVector2D(zoomX, zoomY));
    shaderProgram.setUniformValue("offset", QVector2D(offsetX, offsetY));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);

    glDrawArrays(GL_LINE_STRIP, 0, graphData.size());
    glDisableVertexAttribArray(0);
}

void GraphWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GraphWidget::wheelEvent(QWheelEvent *event) {
    float scaleFactor = 1.1f;
    switch (event->modifiers()) {
    case Qt::ControlModifier:
        zoomX *= (event->angleDelta().y() > 0)? scaleFactor : (1.0f / scaleFactor);
        break;
    case Qt::ShiftModifier:
        zoomY *= (event->angleDelta().y() > 0)? scaleFactor : (1.0f / scaleFactor);
        break;
    default:
        zoomX *= (event->angleDelta().y() > 0)? scaleFactor : (1.0f / scaleFactor);
        zoomY *= (event->angleDelta().y() > 0)? scaleFactor : (1.0f / scaleFactor);
        break;
    }
    update();
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
    }
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging) {
        float dx = (event->pos().x() - lastMousePos.x()) / float(width()) * 2.0f / zoomX;
        float dy = (event->pos().y() - lastMousePos.y()) / float(height()) * 2.0f / zoomY;
        offsetX += dx;
        offsetY -= dy;
        lastMousePos = event->pos();
        update();
    }
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
}
*/
