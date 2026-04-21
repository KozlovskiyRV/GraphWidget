#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector>
#include <QPointF>
#include <QVector2D>
#include <QVector3D>
#include <QRectF>
#include "graphdata.h"

inline QRectF operator/(const QRectF &rect, const QVector2D &zoom) {
    return QRectF(rect.left() / zoom.x(),
                  rect.top() / zoom.y(),
                  rect.width() / zoom.x(),
                  rect.height() / zoom.y());
}

inline QRectF operator-(const QRectF &rect, const QVector2D &zoom) {
    return QRectF(rect.left() - zoom.x(),
                  rect.top() - zoom.y(),
                  rect.width(),
                  rect.height());
}

inline QPointF operator-(const QPointF &point, const QVector2D &zoom) {
    return QPointF(point.x() - zoom.x(),
                   point.y() - zoom.y());
}

inline QPointF operator/(const QPointF &point, const QVector2D &zoom) {
    return QPointF(point.x() / zoom.x(),
                   point.y() / zoom.y());
}

inline QPointF operator*(const QPointF &point, const QVector2D &zoom) {
    return QPointF(point.x() * zoom.x(),
                   point.y() * zoom.y());
}

inline QVector2D operator/(const QVector2D &point, const QVector2D &zoom) {
    return QVector2D(point.x() / zoom.x(),
                     point.y() / zoom.y());
}

inline QVector2D operator*(const QVector2D &point, const QVector2D &zoom) {
    return QVector2D(point.x() * zoom.x(),
                     point.y() * zoom.y());
}

class GraphWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);
    ~GraphWidget() override;

    int addGraph(const QVector<QVector2D> &data, const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 100000);
    int addGraph(const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 100000);

    void addPointToGraph(int graphIndex, const QVector2D &point);

    void setAutoScale(bool is);

    void adjustByMinX(float minX, bool isToUpdate = true);
    void adjustByMinY(float minY, bool isToUpdate = true);
    void adjustByMaxX(float maxX, bool isToUpdate = true);
    void adjustByMaxY(float maxY, bool isToUpdate = true);

    void setZoom(QVector2D zoom, bool isToUpdate = true);
    void setZoomX(float zoom, bool isToUpdate = true);
    void setZoomY(float zoom, bool isToUpdate = true);

    void setOffset(QVector2D offset, bool isToUpdate = true);
    void setOffsetX(float offset, bool isToUpdate = true);
    void setOffsetY(float offset, bool isToUpdate = true);

    void clear();

signals:
    void initialized();
    void boundariesChanged(float minX, float maxX, float minY, float maxY);
    void autoScaleCleared();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QOpenGLShaderProgram shaderProgram;
    QVector<GraphData*> graphs;
    QOpenGLBuffer m_gridVBO;

    QVector2D grid;
    QVector2D zoom;
    QVector2D offset;

    QRectF widgetRect;
    QRectF chartRect;

    QPointF lastMousePos;
    bool isDragging = false;
    bool areBoundariesChanged = true;
    bool isAutoScale = true;
    bool isPointsPresent = false;
    bool isFollow = false;
    bool isAutoScaleY = false;
    float lastVisiblePeriod = 0.0f;

    void evalBoundaries();
    void markBoundariesChanged();
};

#endif // GRAPHWIDGET_H
