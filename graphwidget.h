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
#include <GL/gl.h>
#include <QPen>
#include <QPainter>

struct GraphData {
    QVector<QVector2D> points;  // Точки графика
    QOpenGLBuffer vbo;          // VBO для хранения точек
    QVector3D color;            // Цвет линии (RGB)
    float lineWidth;            // Толщина линии
    int capacity;               // Максимальное число точек, которое может храниться в буфере

    GraphData() : vbo(QOpenGLBuffer::VertexBuffer), lineWidth(1.0f), capacity(0) {}
};

class GraphWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

    /* Добавляет новый график
     * Возвращает индекс графика, по которому уго можно обновлять */
    int addGraph(const QVector<QVector2D> &data, const QVector3D &color, float lineWidth = 1.0f, int capacity = 100000);

    // Добавляет точку к графику
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
    QList<GraphData> graphs;

    float gridX = 10000.0f, gridY = 100.0f;
    float zoomX = 1.0f / gridX, zoomY = 1.0f / gridY;
    float offsetX = -1.0f, offsetY = -0.5f;
    float minX, minY, maxX, maxY;                       // Размеры видимой области графика
    float pointMinX, pointMinY, pointMaxX, pointMaxY;   // Максимальные и минимальные координаты нарисованных точек
    QPoint lastMousePos;
    bool isDragging = false, areBoundariesChanged = true, isAutoScale = true, isPointsPresent = false, isFollow = false, isAutoScaleY = false;

    void evalBoundaries(QMatrix4x4 &transform);
    void evalBoundaries();

    // Обновление VBO для заданного графика (полностью)
    void updateGraphBuffer(int graphIndex);
};

/*
struct Point {
    float x, y;
};

class GraphWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);
    void setData(const QVector<float> &data);
    void addData(float x, float y);
    void setMaxPoints(int maxPoints);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QVector<Point> graphData;
    int maxPoints = 100000;

    QOpenGLBuffer vertexBuffer;
    QOpenGLShaderProgram shaderProgram;

    float zoomX = 1.0f, zoomY = 1.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
    bool isDragging = false;
    QPoint lastMousePos;

    void updateBuffer();
};
*/
#endif // GRAPHWIDGET_H
