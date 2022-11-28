#include "GameOfLife.h"
#include <QDebug>
#include <thread>
#include <mutex>
#include <functional>
#include <QThread>
#include "ui_GameOfLife.h"
#include <QTimer>
#include "MyScene.h"

const std::unordered_map<pattern_setting::pattern, const pattern_setting> pattern_setting::settings =
{ 
	{pattern_setting::pattern::glider, pattern_setting("Glider")}, 
	{pattern_setting::pattern::exploder, pattern_setting("Exploder")}, 
	{pattern_setting::pattern::ten_cell_row, pattern_setting("10 cell row")},
	{pattern_setting::pattern::small_exploder, pattern_setting("Small Exploder")}, 
	{pattern_setting::pattern::diagonal, pattern_setting("Diagonal")}, 
	{pattern_setting::pattern::spaceship, pattern_setting("Spaceship")}
};

const int black = 0;
const int white = 255;

GameOfLife::GameOfLife(QWidget* parent)
	: QMainWindow(parent),
	width_(100),
	height_(100)
{
	ui_ = std::make_unique<Ui::GameOfLifeClass>();
	ui_->setupUi(this);

	//define a timer / Set speed of each frame
	timer_ = new QTimer(this);
	timer_->setInterval(800);

	// Add different modes for the combo box
	for (const auto& item : pattern_setting::settings) {
		ui_->comboBox->addItem(item.second.name, QVariant::fromValue(item.first));
	}

	// Create a Graphics Scene and set it in the Graphics view
	scene_ptr_ = new MyScene(this);
	ui_->graphicsView->setScene(scene_ptr_);

	ui_->history_slider->setMaximum(100); // Keep a history of 100 stages

	//connect signals to slot functions
	connect(timer_, &QTimer::timeout, ui_->next_, &QPushButton::click);
	connect(ui_->start_, &QPushButton::clicked, [&] {timer_->start(); });
	connect(ui_->stop_, &QPushButton::clicked, this, &GameOfLife::pause_game);
	connect(ui_->next_, &QPushButton::clicked, this, &GameOfLife::game_play);
	connect(ui_->history_slider, &QSlider::valueChanged, this, &GameOfLife::game_history);
	connect(ui_->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &GameOfLife::on_pattern_change);
	connect(scene_ptr_, &MyScene::clicked_on_pixel, this, &GameOfLife::set_mouse_click);

	on_pattern_change(); // Initailize and Display starting Image/pattern

	// Initialize my thread
	kill_thread_ = false;
	ready_to_push_ = false;
	thread_ = std::thread(&GameOfLife::update_next, this);
}

GameOfLife::~GameOfLife()
{
	kill_thread_ = true;
	cond_.notify_one();
	thread_.join();
}

// Resize/Refit Image with these 2 functions
void GameOfLife::showEvent(QShowEvent* event)
{
	ui_->graphicsView->fitInView(scene_ptr_->sceneRect(), Qt::KeepAspectRatioByExpanding);
	QWidget::showEvent(event);
}
void GameOfLife::resizeEvent(QResizeEvent* event)
{
	ui_->graphicsView->fitInView(scene_ptr_->sceneRect(), Qt::KeepAspectRatioByExpanding);
	QWidget::resizeEvent(event);
}

// Updates/Calculates next state cell
void GameOfLife::update_next()
{
	while (true) {
		{
			std::unique_lock<std::mutex> locker(mutex_);
			cond_.wait(locker, [&](){ return ready_to_push_ || kill_thread_; });

			if (kill_thread_){
				return;
			}
		}

		image_vec_.emplace_back(width_, height_, QImage::Format_Indexed8);
		compute_next_state();

		QMetaObject::invokeMethod(this, "update_image", Qt::QueuedConnection);
		ready_to_push_ = false;
	}
}

void GameOfLife::compute_next_state()
{
	// This is where the game logic is implemented:
	// 1. If a cell is alive -> it stays alive if it has either 2 or 3 live neighbors
	// 2. If a cell is dead -> it comes to life only if it has 3 live neighbors

	if (image_vec_.size() < 2) return; // check incase


	QImage& next_state_img = image_vec_.back();
	const QImage& prev_image = image_vec_[image_vec_.size() - 2];

	const auto is_alive = [&](const auto i, const auto j) {
		return qGray(prev_image.pixel(i, j)) == white;
	};

	next_state_img.fill(black);
	next_state_img.setColor(black, qRgb(black, black, black));
	next_state_img.setColor(white, qRgb(white, white, white));

	//Loop through each cell to update
	for (auto i = 0; i < width_; ++i) {
		for (auto j = 0; j < height_; ++j) {

			auto neighbor_count = 0;

			// Check cell on the right.
			if (i != width_ - 1)
				if (is_alive(i + 1, j))
					neighbor_count++;
			// Check cell on the bottom right.
			if (i != width_ - 1 && j != height_ - 1)
				if (is_alive(i + 1, j + 1))
					neighbor_count++;
			// Check cell on the bottom.
			if (j != height_ - 1)
				if (is_alive(i, j + 1))
					neighbor_count++;
			// Check cell on the bottom left.
			if (i != 0 && j != height_ - 1)
				if (is_alive(i - 1, j + 1))
					neighbor_count++;
			// Check cell on the left.
			if (i != 0)
				if (is_alive(i - 1, j))
					neighbor_count++;
			// Check cell on the top left.
			if (i != 0 && j != 0)
				if (is_alive(i - 1, j - 1))
					neighbor_count++;
			// Check cell on the top.
			if (j != 0)
				if (is_alive(i, j - 1))
					neighbor_count++;
			// Check cell on the top right.
			if (i != width_ - 1 && j != 0)
				if (is_alive(i + 1, j - 1))
					neighbor_count++;

			if (is_alive(i, j) && neighbor_count < 2) { next_state_img.setPixel(i, j, black); }
			if (is_alive(i, j) && (neighbor_count == 2 || neighbor_count == 3)) { next_state_img.setPixel(i, j, white); }
			if (is_alive(i, j) && neighbor_count > 3) { next_state_img.setPixel(i, j, black); }
			if (!is_alive(i, j) && neighbor_count == 3) { next_state_img.setPixel(i, j, white); }
		}
	}
}

void GameOfLife::update_image()
{
	scene_ptr_->set_image(image_vec_.back());
}


// Function connected with the TIMER, Continuously updates screen
void GameOfLife::game_play()
{
	{
		std::unique_lock<std::mutex> locker(mutex_);
		ready_to_push_ = true;
	}

	cond_.notify_one();
}

// Function connected to slider
// Displays history of your game as you move slider
void GameOfLife::game_history(const int value)
{
	// Value range black-99

	const auto max_value = ui_->history_slider->maximum();
	pause_game();

	const auto images = image_vec_.size();

	if (value < images) {

		if (images > max_value){
			const auto factor = images / max_value;			//Ex: if vec size = 200  //factor = 200/100 = 2
			const auto index = value * factor;				//i = 2 * (0-100) = (0 - 200)
			scene_ptr_->set_image(image_vec_[index]);
		}
		else{
			scene_ptr_->set_image(image_vec_[value]);
		}
	}

}

void GameOfLife::set_mouse_click(const QPointF& point, const bool is_left)
{
	const auto old_index = image_vec_.size() - 1;
	auto& mouse_clk_img = image_vec_.emplace_back(width_, height_, QImage::Format_Indexed8);

	// Build current/new image
	mouse_clk_img.fill(black);
	mouse_clk_img.setColor(black, qRgb(black, black, black));
	mouse_clk_img.setColor(white, qRgb(white, white, white));

	std::copy(image_vec_[old_index].constBits(), image_vec_[old_index].constBits() + image_vec_[old_index].sizeInBytes(), mouse_clk_img.bits());

	mouse_clk_img.setPixel(point.x(), point.y(), is_left ? white : black); // Left click makes cell alive, right click kills cell

	update_image();
}

void GameOfLife::pause_game() const
{
	timer_->stop();
}

// Intializes screen with different items depending on choice from combo box
void GameOfLife::fill_pattern(QImage& img, const pattern_setting::pattern option)
{
	const auto center_x = width_ / 2;
	const auto center_y = height_ / 2;

	switch (option)
	{
	case pattern_setting::pattern::glider:
		for (auto i = 0; i < 3; ++i) { img.setPixel(center_x + i, center_y, white); }
		img.setPixel(center_x + 1, center_y - 2, white);
		img.setPixel(center_x + 2, center_y - 1, white);
		break;

	case pattern_setting::pattern::exploder:
		for (auto j = 0; j < 5; ++j) { img.setPixel(center_x, center_y + j, white); }
		for (auto j = 0; j < 5; ++j) { img.setPixel(center_x + 4, center_y + j, white); }
		img.setPixel(center_x + 2, center_y, white);
		img.setPixel(center_x + 2, center_y + 4, white);
		break;

	case pattern_setting::pattern::small_exploder:
		img.setPixel(center_x, center_y, white);
		img.setPixel(center_x - 1, center_y, white);
		img.setPixel(center_x + 1, center_y, white);
		img.setPixel(center_x, center_y - 1, white);
		img.setPixel(center_x, center_y + 2, white);
		img.setPixel(center_x - 1, center_y + 1, white);
		img.setPixel(center_x + 1, center_y + 1, white);
		break;


	case pattern_setting::pattern::spaceship:
		img.setPixel((center_x / 2), center_y + 1, white);
		img.setPixel((center_x / 2), center_y + 3, white);
		img.setPixel((center_x / 2) + 3, center_y + 3, white);
		for (auto i = 1; i < 5; ++i) { img.setPixel((center_x / 2) + i, center_y, white); }
		for (auto j = 0; j < 3; ++j) { img.setPixel((center_x / 2) + 4, center_y + j, white); }
		break;


	case pattern_setting::pattern::ten_cell_row:
		for (auto i = -5; i < 5; ++i) { img.setPixel(center_x + i, center_y, white); }
		break;

	case pattern_setting::pattern::diagonal:
		for (auto i = 0; i < height_; ++i) { img.setPixel(i, i, white); }
		break;

	default:
		throw std::runtime_error("Made an error");
	}
}

void GameOfLife::on_pattern_change()
{
	auto& img = image_vec_.emplace_back(width_, height_, QImage::Format_Indexed8);
	img.fill(black);
	img.setColor(white, qRgb(white, white, white)); // only need white/alive for when creating a pattern manually

	const auto option = ui_->comboBox->currentData().value<pattern_setting::pattern>();
	fill_pattern(img, option);

	update_image();
}
