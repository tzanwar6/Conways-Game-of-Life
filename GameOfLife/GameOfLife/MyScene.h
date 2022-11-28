#pragma once
#ifndef MY_SCENE_H
#define MY_SCENE_H

#include <QGraphicsScene>

class MyScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	explicit MyScene(QObject* parent = Q_NULLPTR);

public slots:
	void set_image(const QImage& image);

signals:
	void clicked_on_pixel(const QPointF& point, bool is_left);
		
private:

	QGraphicsPixmapItem* item_;
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
};

#endif
