#include "frame.h"

#include <qpainter.h>
#include <qbitmap.h>
#include <qstyle.h>

#include "dialogs.h"


FieldFrame::FieldFrame(QWidget *parent)
    : QFrame(parent, "field"), _button(0)
{
    setFrameStyle( QFrame::Box | QFrame::Raised );
	setLineWidth(2);
	setMidLineWidth(2);
}

void FieldFrame::readSettings()
{
    _cp = AppearanceConfig::readCaseProperties();
    _button.resize(_cp.size, _cp.size);

    QBitmap mask;

    for (uint i=0; i<Nb_Pixmap_Types; i++) {
        drawPixmap(mask, (PixmapType)i, true);
        drawPixmap(_pixmaps[i], (PixmapType)i, false);
        _pixmaps[i].setMask(mask);
    }
    for (uint i=0; i<Nb_Advised; i++) {
        drawAdvised(mask, i, true);
        drawAdvised(_advised[i], i, false);
        _advised[i].setMask(mask);
    }

    QFont f = font();
	f.setPointSize(_cp.size-6);
	f.setBold(true);
	setFont(f);
}

void FieldFrame::initPixmap(QPixmap &pix, bool mask) const
{
    pix.resize(_cp.size, _cp.size);
    if (mask) pix.fill(color0);
}

void FieldFrame::drawPixmap(QPixmap &pix, PixmapType type, bool mask) const
{
    initPixmap(pix, mask);
    QPainter p(&pix);

    if ( type==FlagPixmap ) {
        p.setWindow(0, 0, 16, 16);
        p.setPen( (mask ? color1 : black) );
        p.drawLine(6, 13, 14, 13);
        p.drawLine(8, 12, 12, 12);
        p.drawLine(9, 11, 11, 11);
        p.drawLine(10, 2, 10, 10);
        if (!mask) p.setPen(black);
        p.setBrush( (mask ? color1 : _cp.colors[FlagColor]) );
        p.drawRect(4, 3, 6, 5);
        return;
    }

    p.setWindow(0, 0, 20, 20);
	if ( type==ExplodedPixmap )
		p.fillRect(2, 2, 16, 16, (mask ? color1 : _cp.colors[ExplosionColor]));
	QPen pen(mask ? color1 : black, 1);
	p.setPen(pen);
	p.setBrush(mask ? color1 : black);
	p.drawLine(10,3,10,18);
	p.drawLine(3,10,18,10);
	p.drawLine(5, 5, 16, 16);
	p.drawLine(5, 15, 15, 5);
	p.drawEllipse(5, 5, 11, 11);
	p.fillRect(8, 8, 2, 2, (mask ? color1 : white));
	if ( type==ErrorPixmap ) {
		if (!mask) {
			pen.setColor(_cp.colors[ErrorColor]);
			p.setPen(pen);
		}
		p.drawLine(3, 3, 17, 17);
		p.drawLine(4, 3, 17, 16);
		p.drawLine(3, 4, 16, 17);
		p.drawLine(3, 17, 17, 3);
		p.drawLine(3, 16, 16, 3);
		p.drawLine(4, 17, 17, 4);
	}
}

void FieldFrame::drawAdvised(QPixmap &pix, uint i, bool mask) const
{
    initPixmap(pix, mask);
    QPainter p(&pix);
    p.setWindow(0, 0, 16, 16);
    p.setPen( QPen((mask ? color1 : _cp.numberColors[i]), 2) );
    p.drawRect(3, 3, 11, 11);
}

void FieldFrame::drawBox(QPainter &painter, const QPoint &p,
                      bool pressed, PixmapType type, const QString &text,
                      uint nbMines, int advised,
                      bool hasFocus) const
{
	painter.translate(p.x(), p.y());

    QStyle::SFlags flags = QStyle::Style_Enabled;
    if (hasFocus) flags |= QStyle::Style_HasFocus;
    if (pressed) {
        flags |= QStyle::Style_Sunken;
        flags |= QStyle::Style_Down;
    } else {
        flags |= QStyle::Style_Raised;
        flags |= QStyle::Style_Up;
    }

    style().drawPrimitive(QStyle::PE_ButtonCommand, &painter, _button.rect(),
                        colorGroup(), flags);
    if (hasFocus) {
        QRect fbr = style().subRect(QStyle::SR_PushButtonFocusRect, &_button);
        style().drawPrimitive(QStyle::PE_FocusRect, &painter, fbr,
                            colorGroup(), flags);
    }

	painter.resetXForm();
    QRect r(p, _button.size());
    const QPixmap *pixmap = (type==NoPixmap ? 0 : &_pixmaps[type]);
    QColor color = (nbMines==0 ? black : _cp.numberColors[nbMines-1]);
    style().drawItem(&painter, r, AlignCenter, colorGroup(), true, pixmap,
                     text, -1, &color);
    if ( advised!=-1 )
        style().drawItem(&painter, r, AlignCenter, colorGroup(), true,
                         &_advised[advised], QString::null);
}