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

    rd::usd::loader loader = rd::usd::loader(settings.usd_file);
    if(settings.error > 0) return 1;

    render render(&settings, &loader);
    if(settings.show_ui){
        QApplication app(argc, argv);
        RenderWindow window(&settings, &loader);
        // connect the render window to the render object
        QObject::connect(&render, &render::progressUpdated, &window, &RenderWindow::updateProgress);
        QObject::connect(&render, &render::bucketFinished, &window, &RenderWindow::updateBucket);
        QObject::connect(&window, &RenderWindow::render_requested, &render, &render::render_scene_slot);
        QObject::connect(&window, &RenderWindow::spectrum_sampling_changed, &render, &render::spectrum_sampling_changed);
        QObject::connect(&window, &RenderWindow::samples_changed, &render, &render::samples_changed);
        QObject::connect(&window, &RenderWindow::resolution_changed, &render, &render::resolution_changed);
        QObject::connect(&window, &RenderWindow::lightsource_changed, &render, &render::lightsource_override);
        QObject::connect(&window, &RenderWindow::render_mode_changed, &render, &render::render_mode_changed);
        QObject::connect(&render, &render::samples_changed_internal, &window, &RenderWindow::updateSamples);
        window.show();
        return app.exec();
    } else {
        return render.render_scene();
    }
}
