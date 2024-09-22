#include <cstdio>
#include <png.h>
#include <vector>


#include "render.h"
#include "ui/render_window.h"
#include <QApplication>
#include <thread>
#include <atomic>

std::atomic<bool> should_continue_rendering(true);

int main(int argc, char *argv[]) {
    settings settings(argc, argv);
    if(settings.error > 0) return 1;

    render render(settings);
    if(settings.show_ui){
        QApplication app(argc, argv);
        RenderWindow window(settings.image_width, settings.image_height);
        
        std::thread render_thread([&]() {
            render.render_scene(settings);
        });

        QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
            should_continue_rendering = false;
            if (render_thread.joinable()) {
                render_thread.join();
            }
        });

        window.show();
        return app.exec();
    } else {
        return render.render_scene(settings);
    }
}
