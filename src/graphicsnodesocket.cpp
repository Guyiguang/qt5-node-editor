/* See LICENSE file for copyright and license details. */

#include "graphicsnodesocket.hpp"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QFont>
#include <QFontMetrics>

#include <algorithm>

#include "graphicsbezieredge.hpp"

#include "graphicsnode.hpp"

#include "graphicsnodesocket_p.h"

#include "qnodeeditorsocketmodel.h"

#define BRUSH_COLOR_SINK      QColor("#FF0077FF")
#define BRUSH_COLOR_SOURCE    QColor("#FFFF7700")
#define TEXT_ALIGNMENT_SINK   Qt::AlignLeft
#define TEXT_ALIGNMENT_SOURCE Qt::AlignRight

#include "graphicsbezieredge_p.h"

GraphicsNodeSocket::
GraphicsNodeSocket(QNodeEditorSocketModel* model, SocketType type, GraphicsNode *parent)
: GraphicsNodeSocket(model, type, QString(), parent)
{ }


GraphicsNodeSocket::
GraphicsNodeSocket(QNodeEditorSocketModel* model, SocketType socket_type, const QString &text, GraphicsNode *parent, QObject *data,int index)
: QObject(), d_ptr(new GraphicsNodeSocketPrivate(this))
{
    d_ptr->m_data  = data; //FIXME dead code 
    d_ptr->m_index = index; //FIXME user QPersistentModelIndex

    d_ptr->_socket_type = socket_type;

    d_ptr->_brush_circle = (socket_type == SocketType::SINK) ?
        BRUSH_COLOR_SINK : BRUSH_COLOR_SOURCE;

    d_ptr->_text = text;
    d_ptr->_node = parent;

    d_ptr->_pen_circle.setWidth(0);

    d_ptr->m_pGraphicsItem = new SocketGraphicsItem(parent->graphicsItem(), d_ptr);

    d_ptr->m_pGraphicsItem->setAcceptDrops(true);
}

QGraphicsItem *GraphicsNodeSocket::
graphicsItem() const
{
    return d_ptr->m_pGraphicsItem;
}

GraphicsNodeSocket::SocketType GraphicsNodeSocket::
socketType() const
{
    return d_ptr->_socket_type;
}

int SocketGraphicsItem::
type() const
{
    return GraphicsNodeItemTypes::TypeSocket;
}

bool GraphicsNodeSocket::
isSink() const
{
    return d_ptr->_socket_type == SocketType::SINK;
}

bool GraphicsNodeSocket::
isSource() const
{
    return d_ptr->_socket_type == SocketType::SOURCE;
}


QRectF SocketGraphicsItem::
boundingRect() const
{
    const QSizeF s = d_ptr->q_ptr->size();
    const qreal  x = -d_ptr->_circle_radius - d_ptr->_pen_width/2;
    const qreal  y = -s.height()/2.0 - d_ptr->_pen_width/2;

    return QRectF(
        d_ptr->_socket_type == GraphicsNodeSocket::SocketType::SINK ?
            x : -s.width()-x,
        y,
        s.width(),
        s.height()
    ).normalized();
}


QSizeF GraphicsNodeSocket::
minimalSize() const {
    // Assumes the theme doesn't change
    static QFontMetrics fm({});
    static const qreal  text_height = static_cast<qreal>(fm.height());
    const int           text_width  = fm.width(d_ptr->_text);

    return {
        std::max(
            d_ptr->_min_width,
            d_ptr->_circle_radius*2 + d_ptr->_text_offset + text_width + d_ptr->_pen_width
        ),
        std::max(d_ptr->_min_height, text_height + d_ptr->_pen_width)
    };
}


QSizeF GraphicsNodeSocket::
size() const
{
    return minimalSize();
}


void GraphicsNodeSocketPrivate::
drawAlignedText(QPainter *painter)
{
    int flags = Qt::AlignVCenter;

    static const qreal s = 32767.0;

    QPointF corner;

    switch(_socket_type) {
        case GraphicsNodeSocket::SocketType::SINK:
            corner = {
                _circle_radius + _text_offset,
                -s
            };
            corner.ry() += s/2.0;
            flags |= Qt::AlignLeft;
            break;
        case GraphicsNodeSocket::SocketType::SOURCE:
            corner = {
                -_circle_radius - _text_offset,
                -s
            };
            corner.ry() += s/2.0; //TODO find out why it was done this way
            corner.rx() -= s;
            flags |= Qt::AlignRight;
            break;
    }

    const QRectF rect(corner, QSizeF(s, s));
    painter->setPen(_pen_text);
    painter->drawText(rect, flags, _text, 0);
}


QPointF GraphicsNodeSocketPrivate::
sceneAnchorPos() const
{
    return m_pGraphicsItem->mapToScene(0,0);
}


void SocketGraphicsItem::
paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->setPen(d_ptr->_pen_circle);
    painter->setBrush(d_ptr->_brush_circle);
    painter->drawEllipse(-d_ptr->_circle_radius, -d_ptr->_circle_radius, d_ptr->_circle_radius*2, d_ptr->_circle_radius*2);
    d_ptr->drawAlignedText(painter);

    // debug painting the bounding box
#if 0
    QPen debugPen = QPen(QColor(Qt::red));
    debugPen.setWidth(0);
    auto r = boundingRect();
    painter->setPen(debugPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);

    painter->drawPoint(0,0);
    painter->drawLine(0,0, r.width(), 0);
#endif
}


void SocketGraphicsItem::
mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
}


void SocketGraphicsItem::
mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}


bool GraphicsNodeSocketPrivate::
isInSocketCircle(const QPointF &p) const
{
    return p.x() >= -_circle_radius
        && p.x() <=  _circle_radius
        && p.y() >= -_circle_radius
        && p.y() <=  _circle_radius;
}


void SocketGraphicsItem::
mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
}

/**
* set the edge for this socket
*/
void GraphicsNodeSocket::
setEdge(GraphicsDirectedEdge *edge)
{
    // TODO: handle edge conflict
    d_ptr->_edge = edge;

    if (d_ptr->_edge) {
        switch (d_ptr->_socket_type) {
        case SocketType::SINK:
            Q_EMIT connectedTo(d_ptr->_edge->source());
            break;
        case SocketType::SOURCE:
            Q_EMIT connectedTo(d_ptr->_edge->sink());
            break;
        }
    }

    d_ptr->notifyPositionChange();
}

void GraphicsNodeSocketPrivate::
notifyPositionChange()
{
    if (!_edge) return;

    switch (_socket_type) {
    case GraphicsNodeSocket::SocketType::SINK:
        _edge->d_ptr->setStop(m_pGraphicsItem->mapToScene(0,0));
        break;
    case GraphicsNodeSocket::SocketType::SOURCE:
        _edge->d_ptr->setStart(m_pGraphicsItem->mapToScene(0,0));
        break;
    }
}

GraphicsDirectedEdge* GraphicsNodeSocket::
edge() const
{
    return d_ptr->_edge;
}


QString GraphicsNodeSocket::
text() const
{
    return d_ptr->_text;
}

void GraphicsNodeSocket::
setText(const QString& t)
{
    d_ptr->_text = t;
}

GraphicsNode *GraphicsNodeSocket::
source() const
{
    if ((!d_ptr->_edge) || (!d_ptr->_edge->source())) return nullptr;

    return d_ptr->_edge->source()->d_ptr->_node;
}

GraphicsNode *GraphicsNodeSocket::
sink() const
{
    if ((!d_ptr->_edge) || (!d_ptr->_edge->sink())) return nullptr;

    return d_ptr->_edge->sink()->d_ptr->_node;
}

