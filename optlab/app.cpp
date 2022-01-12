#include "app.h"

#include <QtCore/QDebug>
#include <QtGui/QImageReader>
#include <QtGui/QImageWriter>
#include <QtGui/QPixmap>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <leptonica/allheaders.h>
#include <avir.h>


static PIX* QImage2PIX(QImage& src_image)
{
	src_image = src_image.rgbSwapped();
	const int width = src_image.width();
	const int height = src_image.height();
	const int depth = src_image.depth();
	const int wpl = src_image.bytesPerLine() / 4;

	PIX* dst_image = pixCreate(width, height, depth);
	pixSetWpl(dst_image, wpl);
	pixSetColormap(dst_image, nullptr);
	l_uint32* raw_data = dst_image->data;

	for (int y = 0; y < height; y++)
	{
		l_uint32* lines = raw_data + y * wpl;
		QByteArray arr(reinterpret_cast<const char*>(src_image.scanLine(y)), src_image.bytesPerLine());
		for (int j = 0; j < arr.size(); ++j)
		{
			*(reinterpret_cast<l_uint8*>(lines) + j) = arr[j];
		}
	}

	return pixEndianByteSwapNew(dst_image);
}


static QImage PIX2QImage(PIX* src_image)
{
	const int width = pixGetWidth(src_image);
	const int height = pixGetHeight(src_image);
	const int depth = pixGetDepth(src_image);
	const int bytesPerLine = pixGetWpl(src_image) * 4;
	l_uint32* s_data = pixGetData(pixEndianByteSwapNew(src_image));

	QImage::Format format;
	if (depth == 1)
		format = QImage::Format_Mono;
	else if (depth == 0)
		format = QImage::Format_Indexed8;
	else
		format = QImage::Format_RGB32;

	QImage result(reinterpret_cast<uchar*>(s_data), width, height, bytesPerLine, format);

	// Handle pallete
	QVector<QRgb> _bwCT;
	_bwCT.append(qRgb(255, 255, 255));
	_bwCT.append(qRgb(0, 0, 0));

	QVector<QRgb> _grayscaleCT(256);
	for (int i = 0; i < 256; ++i)
	{
		_grayscaleCT.append(qRgb(i, i, i));
	}

	if (depth == 1)
	{
		result.setColorTable(_bwCT);
	}
	else if (depth == 8)
	{
		result.setColorTable(_grayscaleCT);
	}
	else
	{
		result.setColorTable(_grayscaleCT);
	}

	if (result.isNull())
	{
		QImage none(0, 0, QImage::Format_Invalid);
		return none;
	}

	return result.rgbSwapped();
}


app::app(QWidget* parent)
	: QMainWindow(parent),
	  ui_(new Ui::app),
	  current_image_file_name_(""),
	  scale_factor_(1.0)
{
	ui_->setupUi(this);
}

app::~app()
{
	delete ui_;
}

void app::setup_app_ui()
{
	ui_->image_label->setScaledContents(true);
	this->resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

bool app::load_file(const QString& file_name)
{
	QImageReader reader(file_name);
	reader.setAutoTransform(true);
	const QImage new_image = reader.read();
	if (new_image.isNull())
	{
		QMessageBox::information(this,
		                         QGuiApplication::applicationDisplayName(),
		                         tr(u8"无法载入图片 %1: %2")
		                         .arg(QDir::toNativeSeparators(file_name), reader.errorString()));
		return false;
	}

	set_image(new_image);

	this->setWindowFilePath(file_name);

	const QString message = tr(u8"打开文件 \"%1\"\t分辨率 %2x%3\t位深度 %4")
	                        .arg(QDir::toNativeSeparators(file_name))
	                        .arg(image_.width()).arg(image_.height()).arg(image_.depth());
	ui_->status_bar->showMessage(message);
	return true;
}

void app::set_image(const QImage& new_image)
{
	image_ = new_image;
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));
	Q_ASSERT(ui_->image_label->pixmap());
	scale_factor_ = 1.0;
}

bool app::save_file(const QString& file_name)
{
	QImageWriter writer(file_name);

	if (!writer.write(image_))
	{
		QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
		                         tr(u8"无法保存 %1: %2")
		                         .arg(QDir::toNativeSeparators(file_name)), writer.errorString());
		return false;
	}
	const QString message = tr(u8"已保存 \"%1\"").arg(QDir::toNativeSeparators(file_name));
	ui_->status_bar->showMessage(message);
	return true;
}

void app::on_action_open_triggered()
{
	current_image_file_name_ = QFileDialog::getOpenFileName(this,
	                                                        tr(u8"打开文件"),
	                                                        "",
	                                                        tr(u8"图像文件 (*.png;*.jpg;*.jpeg)"));
	qDebug() << u8"正在打开文件" << current_image_file_name_;
	load_file(current_image_file_name_);
}

void app::on_action_save_triggered()
{
	const QString image_file_name = current_image_file_name_;
	qDebug() << u8"正在保存文件" << image_file_name;

	if (!image_file_name.isNull() || !image_file_name.isEmpty())
		save_file(image_file_name);
}

void app::on_action_save_as_triggered()
{
	const QString image_file_name = QFileDialog::getSaveFileName(this,
	                                                             tr(u8"保存文件"),
	                                                             "",
	                                                             tr(u8"图像文件 (*.png;*.jpg;*.jpeg)"));
	qDebug() << u8"正在保存文件" << image_file_name;

	if (!image_file_name.isNull() || !image_file_name.isEmpty())
		save_file(image_file_name);
}

void app::on_inverting_push_button_clicked()
{
	qDebug() << "Inverting Image";
	if (image_.isNull())
	{
		return;
	}
	auto src_image = QImage2PIX(image_);
	const auto w = pixGetWidth(src_image);
	const auto h = pixGetHeight(src_image);
	const auto d = pixGetDepth(src_image);
	PIX* dst_image = pixCreate(w, h, d);
	pixInvert(dst_image, src_image);
	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

void app::on_rotate_it_push_button_clicked()
{
	const float deg2rad = 3.1415926535f / 180.f;
	float angle = ui_->set_angle_line_edit->text().toFloat();
	qDebug() << "Rotate Image";
	if (image_.isNull())
	{
		return;
	}
	auto src_image = QImage2PIX(image_);

	auto format = pixGetInputFormat(src_image);
	if (format == IFF_UNKNOWN) format = IFF_PNG;

	if (angle == 90.0f || angle == 180.0f || angle == 270.0f)
	{
		const auto quad = static_cast<l_int32>((angle + 0.5f) / 90.0f);
		auto dst_image = pixRotateOrth(src_image, quad);
		image_ = PIX2QImage(dst_image);
		ui_->image_label->setPixmap(QPixmap::fromImage(image_));

		pixDestroy(&src_image);
		pixDestroy(&dst_image);
		return;
	}

	const float angle_rad = deg2rad * angle;
	const auto i_color = L_BRING_IN_WHITE;
	const auto i_type = L_ROTATE_AREA_MAP;

	auto dst_image = pixRotate(src_image, angle_rad, i_type, i_color, 0, 0);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

void app::on_rescaling_push_button_clicked()
{
	qDebug() << "Rescaling Image";
	if (image_.isNull())
	{
		return;
	}

	float x_factor = ui_->set_width_factor_line_edit->text().toFloat();
	float y_factor = ui_->set_height_factor_line_edit->text().toFloat();

	const int width = image_.width();
	const int height = image_.height();
	const int scan_line_size = image_.bytesPerLine();
	const int depth = image_.depth();

	const int new_width = x_factor * width;
	const int new_height = y_factor * height;


	QImage dst_image(new_width, new_height, image_.format());
	dst_image.fill(0);

	avir::CImageResizer<> image_resizer(depth, depth);
	image_resizer.resizeImage(image_.bits(), width, height, scan_line_size,
	                          dst_image.bits(), new_width, new_height,
	                          depth / 8, 0);

	image_ = dst_image;
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));
}

void app::on_binarisation_push_button_clicked()
{
	qDebug() << "Binarisation Image";
	if (image_.isNull())
	{
		return;
	}
	QImage img = image_.convertToFormat(QImage::Format_Grayscale8);
	auto src_image = QImage2PIX(img);
	l_int32 w, h;
	PIX* dst_image = nullptr;
	pixGetDimensions(src_image, &w, &h, nullptr);
	pixSauvolaBinarize(src_image, 7, 0.34f, 1, nullptr, nullptr, nullptr, &dst_image);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

void app::on_deskew_push_button_clicked()
{
	static const l_float32  DEFAULT_SWEEP_RANGE = 90.;
	qDebug() << "Deskew Image";
	if (image_.isNull())
	{
		return;
	}

	l_float32 angle, conf;
	auto src_image = QImage2PIX(image_);
	auto dst_image = pixDeskewGeneral(src_image, 0, DEFAULT_SWEEP_RANGE, 0.0, 0, 0, &angle, &conf);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

void app::on_despeckle_push_button_clicked()
{
	qDebug() << "Despeckle Image";
	if (image_.isNull())
	{
		return;
	}
	/* HMT (with just misses) for speckle up to 2x2 */
	static auto selstr2 = "oooo"
		"oC o"
		"o  o"
		"oooo";

	/*  Normalize for rapidly varying background */
	auto pixa_1 = pixaCreate(0);
	auto img = image_.convertToFormat(QImage::Format_Grayscale8);
	auto src_image = QImage2PIX(img);
	pixaAddPix(pixa_1, src_image, L_INSERT);
	auto pix_1 = pixBackgroundNormFlex(src_image, 7, 7, 1, 1, 10);
	pixaAddPix(pixa_1, pix_1, L_INSERT);

	/* Remove the background */
	auto pix_2 = pixGammaTRCMasked(nullptr, pix_1, nullptr, 1.0, 100, 175);

	/* Binarize */
	auto pix_3 = pixThresholdToBinary(pix_2, 180);
	pixaAddPix(pixa_1, pix_3, L_INSERT);

	/* Remove the speckle noise up to 2x2 */
	auto sel_1 = selCreateFromString(selstr2, 4, 4, "speckle2");
	auto pix_4 = pixHMT(nullptr, pix_3, sel_1);
	pixaAddPix(pixa_1, pix_4, L_INSERT);
	auto sel_2 = selCreateBrick(2, 2, 0, 0, SEL_HIT);
	auto pix_5 = pixDilate(nullptr, pix_4, sel_2);
	pixaAddPix(pixa_1, pix_5, L_INSERT);

	auto dst_image = pixSubtract(nullptr, pix_3, pix_5);
	pixaAddPix(pixa_1, dst_image, L_INSERT);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	selDestroy(&sel_1);
	selDestroy(&sel_2);

	pixDestroy(&pix_2);
	pixaDestroy(&pixa_1);

}

void app::on_dilation_push_button_clicked()
{
	qDebug() << "Dilation";
	if (image_.isNull())
	{
		return;
	}

	auto img = image_.convertToFormat(QImage::Format_Mono);
	auto src_image = QImage2PIX(img);

	auto sela = selaAddBasic(nullptr);
	auto nsels = selaGetCount(sela);
	l_int32 idx = 1;
	auto sel = selaGetSel(sela, idx);
	auto selname = selGetName(sel);

	auto dst_image = pixDilate(nullptr, src_image, sel);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

void app::on_erosion_push_button_clicked()
{
	qDebug() << "Erosion";
	if (image_.isNull())
	{
		return;
	}

	auto img = image_.convertToFormat(QImage::Format_Mono);
	auto src_image = QImage2PIX(img);

	auto sela = selaAddBasic(nullptr);
	auto nsels = selaGetCount(sela);
	l_int32 idx = 0;
	auto sel = selaGetSel(sela, idx);
	auto selname = selGetName(sel);

	auto dst_image = pixErode(nullptr, src_image, sel);

	image_ = PIX2QImage(dst_image);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	pixDestroy(&src_image);
	pixDestroy(&dst_image);
}

/*
 * Use these variable abbreviations:
 *
 * pap1: distance from left edge to the page
 * txt1: distance from left edge to the text
 * Identify pap1 by (a) 1st downward transition in intensity (nait).
 *                  (b) start of 1st lowpass interval (nail)
 * Identify txt1 by (a) end of 1st lowpass interval (nail)
 *                  (b) first upward transition in reversals (nart)
 *
 * pap2: distance from right edge to beginning of last upward transition,
 *       plus some extra for safety.
 * txt1: distance from right edge to the text
 * Identify pap2 by 1st downward transition in intensity.
 * Identify txt2 by (a) beginning of 1st lowpass interval from bottom
 *                  (b) last downward transition in reversals from bottom
 */
static l_int32
GetLeftCut(NUMA* narl,
           NUMA* nart,
           NUMA* nait,
           l_int32 w,
           l_int32* pleft)
{
	l_int32 nrl, nrt, nit, start, end, sign, pap1, txt1, del;

	nrl = numaGetCount(narl);
	nrt = numaGetCount(nart);
	nit = numaGetCount(nait);

	/* Check for small max number of reversals or no edge */
	numaGetSpanValues(narl, 0, nullptr, &end);
	if (end < 20 || nrl <= 1)
	{
		*pleft = 0;
		return 0;
	}

	/* Where is text and page, scanning from the left? */
	pap1 = 0;
	txt1 = 0;
	if (nrt >= 4)
	{
		/* beginning of first upward transition */
		numaGetEdgeValues(nart, 0, &start, nullptr, nullptr);
		txt1 = start;
	}
	if (nit >= 4)
	{
		/* end of first downward trans in (inverse) intensity */
		numaGetEdgeValues(nait, 0, nullptr, &end, &sign);
		if (end < txt1 && sign == -1)
			pap1 = end;
		else
			pap1 = 0.5 * txt1;
	}
	del = txt1 - pap1;
	if (del > 20)
	{
		txt1 -= L_MIN(20, 0.5 * del);
		pap1 += L_MIN(20, 0.5 * del);
	}
	lept_stderr("txt1 = %d, pap1 = %d\n", txt1, pap1);
	*pleft = pap1;
	return 0;
}


static l_int32
GetRightCut(NUMA* narl,
            NUMA* nart,
            NUMA* nait,
            l_int32 w,
            l_int32* pright)
{
	l_int32 nrt, ntrans, start, end, sign, txt2, pap2, found, trans;

	nrt = numaGetCount(nart);

	/* Check for small max number of reversals or no edge */
	/* Where is text and page, scanning from the right?  */
	ntrans = nrt / 3;
	if (ntrans > 1)
	{
		found = FALSE;
		for (trans = ntrans - 1; trans > 0; --trans)
		{
			numaGetEdgeValues(nart, trans, &start, &end, &sign);
			if (sign == -1)
			{
				/* end of textblock */
				txt2 = end;
				found = TRUE;
			}
		}
		if (!found)
		{
			txt2 = w - 1; /* take the whole thing! */
			pap2 = w - 1;
		}
		else
		{
			/* found textblock; now find right side of page */
			found = FALSE;
			for (trans = ntrans - 1; trans > 0; --trans)
			{
				numaGetEdgeValues(nart, trans, &start, &end, &sign);
				if (sign == 1 && start > txt2)
				{
					pap2 = start; /* start of textblock on other page */
					found = TRUE;
				}
			}
			if (!found)
			{
				/* no text from other page */
				pap2 = w - 1; /* refine later */
			}
		}
	}
	else
	{
		txt2 = w - 1;
		pap2 = w - 1;
	}
	lept_stderr("txt2 = %d, pap2 = %d\n", txt2, pap2);
	*pright = pap2;
	return 0;
}

void app::on_scanning_border_removal_push_button_clicked()
{
	static const l_int32  mindif = 60;

	qDebug() << "Scanning Border Removal";
	if (image_.isNull())
	{
		return;
	}
	l_int32 w, h;
	auto src_image = QImage2PIX(image_);
	auto gray_image = pixConvertTo8(src_image, 0);
	pixGetDimensions(gray_image, &w, &h, nullptr);

	/* Get info on vertical reversal profile */
	auto nar = pixReversalProfile(gray_image, 0.8, L_VERTICAL_LINE,
	                         0, h - 1, mindif, 1, 1);
	auto naro = numaOpen(nar, 11);
	auto pix1 = gplotSimplePix1(naro, "Reversals Opened");
	auto narl = numaLowPassIntervals(naro, 0.1, 0.0);
	auto nart = numaThresholdEdges(naro, 0.1, 0.5, 0.0);

	numaDestroy(&nar);
	numaDestroy(&naro);

	/* Get info on vertical intensity profile */
	auto pixgi = pixInvert(nullptr, gray_image);
	auto nai = pixAverageIntensityProfile(pixgi, 0.8, L_VERTICAL_LINE,
	                                 0, h - 1, 1, 1);
	auto naio = numaOpen(nai, 11);
	auto pix2 = gplotSimplePix1(naio, "Intensities Opened");
	auto nait = numaThresholdEdges(naio, 0.4, 0.6, 0.0);
	numaDestroy(&nai);
	numaDestroy(&naio);

	/* Analyze profiles for left/right edges  */
	l_int32 left, right;
	GetLeftCut(narl, nart, nait, w, &left);
	GetRightCut(narl, nart, nait, w, &right);

	/* Output visuals */
	auto pixa2 = pixaCreate(3);
	pixaAddPix(pixa2, src_image, L_INSERT);
	pixaAddPix(pixa2, pix1, L_INSERT);
	pixaAddPix(pixa2, pix2, L_INSERT);
	auto pixd = pixaDisplayTiledInColumns(pixa2, 2, 1.0, 25, 0);
	pixaDestroy(&pixa2);
	image_ = PIX2QImage(pixd);
	ui_->image_label->setPixmap(QPixmap::fromImage(image_));

	//pixDestroy(&src_image);
	//pixDestroy(&gray_image);
}
