#ifndef OPTLABAPP_H
#define OPTLABAPP_H

#include <QDialog>
#include <QMainWindow>

#include "./ui_binarization.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OptLabApp; class binarization_operation_dialog;}
QT_END_NAMESPACE

class binarization_operation : public QDialog
{
	Q_OBJECT

public:
	explicit binarization_operation(QImage image, QWidget *parent = Q_NULLPTR);
	~binarization_operation() override;

private:
	QImage image_;
	Ui::binarization_operation_dialog* ui_;

private slots:
	void on_local_binarization_push_button_clicked();

signals:
	void export_image(QImage);
};


class OptLabApp final : public QMainWindow
{
Q_OBJECT

public:
	explicit OptLabApp(QWidget* parent = Q_NULLPTR);
	~OptLabApp() override;

private:
	void setup_app_ui();
	bool load_file(const QString& file_name);
	void set_image(const QImage& new_image);
	bool save_file(const QString& file_name);

	Ui::OptLabApp* ui_;
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

	void receive_image(QImage image);
};

#endif // OPTLABAPP_H
