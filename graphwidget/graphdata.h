#ifndef GRAPHDATA_H
#define GRAPHDATA_H

#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class GraphData {
public:
    explicit GraphData(const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 0);
    explicit GraphData(const QVector<QVector2D> data, const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 0);
    explicit GraphData(QVector<QVector2D> &data, const QVector3D color = {1.0f, 0.0f, 1.0f}, float lineWidth = 1.0f, size_t capacity = 0);

    ~GraphData();

    void setCapacity(int newCapacity);
    void setPoints(const QVector<QVector2D> &newPoints);

    void clear(void);
    void appendPoint(const QVector2D &point);

    int size() const;

    const QVector<QVector2D> &points() const;

    void render(QOpenGLShaderProgram &p, int positionLoc);

    void updateVBO();

private:
    QVector3D m_color;
    float m_lineWidth;
    QOpenGLBuffer m_vbo;
    size_t m_capacity;
    QVector<QVector2D> m_points;
};

#endif // GRAPHDATA_H
