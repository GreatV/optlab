#pragma once

#include <QtWidgets/QMainWindow>
#include <QtGui/QImage>

#include "ui_app.h"


class app final : public QMainWindow
{
Q_OBJECT

public:
	explicit app(QWidget* parent = Q_NULLPTR);
	~app() override;

private:
	void setup_app_ui();
	bool load_file(const QString& file_name);
	void set_image(const QImage& new_image);
	bool save_file(const QString& file_name);

	Ui::app* ui_;
	QImage image_;
	QString current_image_file_name_;
	double scale_factor_;

private slots:
	void on_action_open_triggered();
	void on_action_save_triggered();
	void on_action_save_as_triggered();
	void on_inverting_push_button_clicked();
	void on_rotate_it_push_button_clicked();
	void on_rescaling_push_button_clicked();
	void on_binarisation_push_button_clicked();
	void on_deskew_push_button_clicked();
	void on_despeckle_push_button_clicked();
	void on_dilation_push_button_clicked();
	void on_erosion_push_button_clicked();
	void on_scanning_border_removal_push_button_clicked();
};
