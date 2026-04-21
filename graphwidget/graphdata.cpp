#include "graphdata.h"

GraphData::GraphData(const QVector<QVector2D> data, const QVector3D color, float lineWidth, size_t capacity)
    : m_color{color}, m_lineWidth{lineWidth}, m_vbo{QOpenGLBuffer::VertexBuffer}, m_capacity{capacity}, m_points{data} {

    if (!m_vbo.isCreated())
        m_vbo.create();

    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    if (m_capacity > 0) {
        m_vbo.allocate(m_capacity * sizeof(QVector2D));
        if (!m_points.isEmpty()) {
            m_vbo.write(0, m_points.constData(), m_points.size() * sizeof(QVector2D));
        }
    }

    m_vbo.release();
}

GraphData::GraphData(const QVector3D color, float lineWidth, size_t capacity)
    : GraphData(QVector<QVector2D>{}, color, lineWidth, capacity) {}

GraphData::~GraphData() {
    if (m_vbo.isCreated())
        m_vbo.destroy();
}

void GraphData::setCapacity(int newCapacity) {
    m_capacity = std::max(1, newCapacity);
    m_points.resize(m_capacity);

    if (m_vbo.isCreated()) {
        m_vbo.bind();
        m_vbo.allocate(m_capacity * sizeof(QVector2D));
        m_vbo.release();
    }
}

void GraphData::setPoints(const QVector<QVector2D> &newPoints) {
    m_points = newPoints;
    updateVBO();
}

void GraphData::appendPoint(const QVector2D &point) {
    size_t m_size = static_cast<size_t>(m_points.size());

    if (m_capacity <= 0 || !m_vbo.isCreated())
        return;

    m_vbo.bind();
    if (m_size < m_capacity)
        m_vbo.write(m_size * sizeof(QVector2D), &point, sizeof(QVector2D));
    m_vbo.release();

    m_points.append(point);
}

int GraphData::size() const {
    return m_points.size();
}

void GraphData::updateVBO() {
    if (!m_vbo.isCreated())
        return;

    m_vbo.bind();

    size_t m_size = static_cast<size_t>(m_points.size());
    if (m_size > 0) {
        size_t initPointPos = (m_size > m_capacity) ? (m_size - m_capacity) : 0;
        m_vbo.write(0, m_points.constData() + initPointPos,
                    std::min(m_size, m_capacity) * sizeof(QVector2D) );
    }

    m_vbo.release();
}

void GraphData::render(QOpenGLShaderProgram &p, int positionLoc) {
    p.setUniformValue("color", m_color);
    glLineWidth(m_lineWidth);

    m_vbo.bind();
        p.enableAttributeArray(positionLoc);
        p.setAttributeBuffer(positionLoc, GL_FLOAT, 0, 2, 0);

        glDrawArrays(GL_LINE_STRIP, 0, m_points.size());

        p.disableAttributeArray(positionLoc);
    m_vbo.release();
}

void GraphData::clear(void) {
    m_points.clear();
    m_vbo.bind();
    m_vbo.write(0, nullptr, 0);
    m_vbo.release();
}
