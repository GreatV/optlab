#include "app.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	static const char ENV_VAR_QT_DEVICE_PIXEL_RATIO[] = "QT_DEVICE_PIXEL_RATIO";
	if (!qEnvironmentVariableIsSet(ENV_VAR_QT_DEVICE_PIXEL_RATIO)
		&& !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
		&& !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
		&& !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS"))
	{
		QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	}
	QApplication a(argc, argv);
	app w;
	w.show();
	return a.exec();
}
