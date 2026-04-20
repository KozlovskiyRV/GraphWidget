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
#include <QVector2D>
#include <QVector3D>

class GraphData {
public:
    GraphData();

    void initializeGl();
    void clear();

    void setCapacity(int newCapacity);
    void setPoints(const QVector<QVector2D> &newPoints);

    void appendPoint(const QVector2D &point);

    int size() const;
    int firstChunkSize() const;
    int secondChunkSize() const;
    int firstChunkStartIndex() const;

    const QVector<QVector2D> &points() const;

    QVector3D color = {0.1f, 0.8f, 0.2f};
    float lineWidth = 1.0f;

    QOpenGLBuffer vbo;

private:
    QVector<QVector2D> m_points;
    int m_capacity = 0;
    int m_head = 0;
    int m_size = 0;

    void uploadFull();
};

class GraphWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);
    ~GraphWidget() override;

    int addGraph(const QVector<QVector2D> &data, const QVector3D &color, float lineWidth = 1.0f, int capacity = 100000);
    void addPointToGraph(int graphIndex, const QVector2D &point);

    void setAutoScale(bool is);

    void adjustByMinX(float minX, bool isToUpdate = true);
    void adjustByMinY(float minY, bool isToUpdate = true);
    void adjustByMaxX(float maxX, bool isToUpdate = true);
    void adjustByMaxY(float maxY, bool isToUpdate = true);

    void setZoom(QVector2D zoom);
    void setZoomX(float zoom);
    void setZoomY(float zoom);

    void setOffset(QVector2D offset);
    void setOffsetX(float offset);
    void setOffsetY(float offset);

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
    QOpenGLBuffer overlayVbo;
    QVector<GraphData> graphs;

    float gridX = 10000.0f;
    float gridY = 100.0f;
    float zoomX = 1.0f / gridX;
    float zoomY = 1.0f / gridY;
    float offsetX = -1.0f;
    float offsetY = -0.5f;

    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;

    float pointMinX = 0.0f;
    float pointMinY = 0.0f;
    float pointMaxX = 0.0f;
    float pointMaxY = 0.0f;

    QPoint lastMousePos;
    bool isDragging = false;
    bool areBoundariesChanged = true;
    bool isAutoScale = true;
    bool isPointsPresent = false;
    bool isFollow = false;
    bool isAutoScaleY = false;
    float lastVisiblePeriod = 0.0f;

    void evalBoundaries();
    void markBoundariesChanged();
    void drawOverlay(const QMatrix4x4 &transform);
    void emitBoundariesIfNeeded();
};

#endif // GRAPHWIDGET_H
