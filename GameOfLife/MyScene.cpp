#include "MyScene.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>

MyScene::MyScene(QObject *parent): QGraphicsScene(parent),
item_(nullptr)
{
}

void MyScene::set_image(const QImage& image) 
{
	//sets screen using mouse clicks and next state 

	if (!item_) {
		item_ = addPixmap(QPixmap::fromImage(image));
	}
	else {
		item_->setPixmap(QPixmap::fromImage(image));
	}
}

void MyScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	// Use temporary image for screen

	const auto button_type = event->button();
	const auto is_left = button_type == Qt::LeftButton;
	if (is_left || button_type == Qt::RightButton){
		emit clicked_on_pixel(event->scenePos(), is_left);
	}
		
	QGraphicsScene::mousePressEvent(event);
}
