#include "valveworkbench.h"

#include <QApplication>

#ifdef _WIN32
#include <windows.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // Compute a global UI scale factor based on the screen work-area height.
    // This must run before QApplication is constructed, otherwise Qt has already
    // locked in DPI / scaling metrics.
    //
    // Mapping (continuous):
    // - workAreaHeight ~= 1080 -> scale ~ 0.93
    // - workAreaHeight ~= 1440 -> scale ~ 1.00
    // Clamp to [0.90, 1.00] to avoid extreme shrinking on smaller screens.
    //
    // If the user manually set QT_SCALE_FACTOR, respect that and do nothing.
    if (std::getenv("QT_SCALE_FACTOR") == nullptr) {
        double scale = 1.0;

        RECT r{};
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0)) {
            const int workAreaHeight = (r.bottom - r.top);
            const double a = (1.0 - 0.93) / (1440.0 - 1080.0);
            const double b = 1.0 - a * 1440.0;
            scale = a * static_cast<double>(workAreaHeight) + b;

            if (scale < 0.90) scale = 0.90;
            if (scale > 1.0)  scale = 1.0;
        }

        std::ostringstream ss;
        ss.setf(std::ios::fixed);
        ss << std::setprecision(3) << scale;
        _putenv_s("QT_SCALE_FACTOR", ss.str().c_str());
    }
#endif

    QApplication a(argc, argv);
    ValveWorkbench w;
    w.show();
    return a.exec();
}
