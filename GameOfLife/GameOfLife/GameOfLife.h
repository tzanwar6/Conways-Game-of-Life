#pragma once

#ifndef GAME_OF_LIFE_H
#define GAME_OF_LIFE_H

#include <QtWidgets/QMainWindow>
#include "ui_GameOfLife.h"
#include <QGraphicsItem>
#include <QMainWindow>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>

struct pattern_setting
{
	explicit pattern_setting(const QString& name) : name(name) {}
	QString name;
	enum class pattern { glider, exploder, ten_cell_row, small_exploder, diagonal, spaceship };
	const static std::unordered_map<pattern, const pattern_setting> settings;
};

Q_DECLARE_METATYPE(pattern_setting::pattern)

namespace  Ui
{
	class GameOfLifeClass;
}

class MyScene;
class QTimer;

class GameOfLife final : public QMainWindow
{
	Q_OBJECT

public:
	explicit GameOfLife(QWidget* parent = Q_NULLPTR);
	~GameOfLife();
	void update_next();										//updates next cell state
	void game_play();										//controls game screen
	void fill_pattern(QImage& img, pattern_setting::pattern option);
	void compute_next_state();

public slots:
	void pause_game() const;
	void on_pattern_change();					//intializes screen with different items depending on choice from combo box 
	void game_history(int value);		//Displays history of your game as you move slider
	void set_mouse_click(const QPointF& point, bool is_left);
	void update_image();

private:
	void showEvent(QShowEvent* event) override;       //Scales pix map to graphicView
	void resizeEvent(QResizeEvent* event) override;   //Scales pix map to graphicView when resizing of window occurs 
	bool kill_thread_;

	std::unique_ptr<Ui::GameOfLifeClass> ui_;

	MyScene* scene_ptr_;
	QTimer* timer_;
	std::vector<QImage> image_vec_;

	std::mutex mutex_;
	std::condition_variable cond_;

	bool ready_to_push_;
	std::vector<QImage> output_queue_;
	std::thread thread_;

	const int width_;
	const int height_;
};

#endif
