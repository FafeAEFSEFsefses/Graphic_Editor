#include <gtkmm.h>
#include <iostream>

class DrawingArea : public Gtk::DrawingArea{
private:
    struct Point{
        double x;
        double y;
    };

    struct Line{
        std::vector<Point> points; //линия состоит из точек
        double width = 1.0;// ширина линии
        Gdk::RGBA color = Gdk::RGBA("#000000");// цвет линии
    };
    bool m_button_is_draw_pressed = false;// проверка нанажатие кнопки Draw
    bool m_is_drawing = false; // пользователь в данный момент рисует
    std::vector<Line> m_lines; // для построения линий
    double m_line_width = 1.0; // установка текущей ширины линии
    Gdk::RGBA m_line_color = Gdk::RGBA("#000000"); // установка текущего цвета линии

protected:
    virtual void draw_lines(const Cairo::RefPtr<Cairo::Context>& cr){//функция для отрисовки линий
        cr->save();
        cr->set_source_rgb(1.0, 1.0, 1.0);//цвет холста
        cr->rectangle(0, 0, get_width(), get_height());//размер холста
        cr->fill();

        for (const auto& line : m_lines) {
            cr->set_line_width(line.width); //установка текущей ширины линии
            cr->set_source_rgba(line.color.get_red(), line.color.get_green(), line.color.get_blue(), line.color.get_alpha()); // установка текущего цвета линии

            if (line.points.size() < 2) // если линия не полная
                continue;

            cr->move_to(line.points[0].x, line.points[0].y); // передвигаем точку линии
            for (const auto& p : line.points) {
                cr->line_to(p.x, p.y); //ведём линию к этой точке
            }
            cr->stroke();
        }
        cr->restore();
    }

public:
    DrawingArea(){
        signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& cr) { // сингнал рисования
            draw_lines(cr);
            return true;
        });
        add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK); // события мышки, 1 - на мышку нажали, 2 - мышку отпустили, 3 - мышку двигают
        signal_button_press_event().connect([this](GdkEventButton *event) -> bool {// на мышку нажали
            if(!m_button_is_draw_pressed)//пока кпока Draw не нажата не рисуем
                return true;
            if (event->button == GDK_BUTTON_PRIMARY){//мышку передвигают
                m_is_drawing = true;
                m_lines.push_back(Line());
                m_lines.back().points.push_back({ event->x, event->y });
                m_lines.back().width = m_line_width;
                m_lines.back().color = m_line_color;
            }
            return true;
        });
        signal_button_release_event().connect([this](GdkEventButton *event) -> bool { // мышку отжали
            if (event->button == GDK_BUTTON_PRIMARY){
                m_is_drawing = false;
            }
            return true;
        });
        signal_motion_notify_event().connect([this](GdkEventMotion *event) -> bool { //мышку двигают
            if (m_is_drawing){
                m_lines.back().points.push_back({event->x, event->y});
                queue_draw();
            }
            return true;
        });
    }

    virtual void clear_lines(){ //очищение экрана
        m_lines.clear();
        queue_draw();
    }

    virtual void remove_last_line() { // удаление последней линии
        if (!m_lines.empty()){
            m_lines.pop_back();
            queue_draw();
        }
    }

    virtual void set_line_width(double width) { // установка ширины линии
        m_line_width = width;
    }

    virtual void set_line_color(Gdk::RGBA color) { // установка цвета линии
        m_line_color = color;
    }

    virtual void start_drawing(bool pressed) { // нажатие кнопки Draw
        m_button_is_draw_pressed = pressed;
    }

    virtual void redraw() {
        auto cr = get_window()->create_cairo_context();
        draw_lines(cr);
    }

    virtual void save_image() { // сохранение изображения
    // создание диалогового окна выбора файла для сохранения
        Gtk::FileChooserDialog dialog("Save file", Gtk::FILE_CHOOSER_ACTION_SAVE);
        dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
        dialog.add_button("_Save", Gtk::RESPONSE_OK);

    // создание фильтра для файлов PNG
        auto png_filter = Gtk::FileFilter::create();
        png_filter->set_name("PNG images");
        png_filter->add_mime_type("image/png");

    // добавление созданного фильтра к диалоговому окну
        dialog.add_filter(png_filter);

    // установка имени файла по умолчанию и запрос подтверждения перезаписи, если файл уже существует
        dialog.set_current_name("untitled.png");
        dialog.set_do_overwrite_confirmation(true);

    // запуск диалогового окна и проверка результата
        if (dialog.run() == Gtk::RESPONSE_OK) {
        // получение имени выбранного файла и расширения
            std::string filename = dialog.get_filename();
            auto ext_pos = filename.find_last_of(".");
            std::string extension;
            if (ext_pos != std::string::npos) {
                extension = filename.substr(ext_pos + 1, filename.length() - ext_pos - 1);
            }
            else {
                extension = "";
            }

        // если расширение не указано явно, используется фильтр по умолчанию
            if (extension.empty()) {
                auto filter = dialog.get_filter();
                if (filter && filter->get_name() == "PNG images")
                extension = "png";
            }

        // если расширение - это PNG, создаем поверхность Cairo и сохраняем ее в файл
            if (extension == "png") {
                int width = get_allocated_width();
                int height = get_allocated_height();
                auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
                auto cr = Cairo::Context::create(surface);
                draw_lines(cr);
                surface->write_to_png(filename);
            }
        }
    }
    virtual ~DrawingArea() {}
};

int main(int argc, char *argv[]){
    auto app = Gtk::Application::create(argc, argv);
    Gtk::Window window;
    window.set_title("ГР_ЗТ");
    window.set_default_size(600, 600);

    DrawingArea draw_area;
    Gtk::Button draw_button("Draw");
    draw_button.signal_clicked().connect([&]() {
        draw_area.start_drawing(true);
    });

    Gtk::Button remove_last_line_button("Remove last line");
    remove_last_line_button.signal_clicked().connect([&]() {
        draw_area.remove_last_line();
    });
    Gtk::Button clear_button("Clear");
    clear_button.signal_clicked().connect([&]() {
        draw_area.clear_lines();
    });


    Gtk::Scale line_width_scale(Gtk::Adjustment::create(1.0, 1.0, 10.0, 1.0, 1.0, 0.0), Gtk::ORIENTATION_HORIZONTAL);
    line_width_scale.signal_value_changed().connect([&]() {
        draw_area.set_line_width(line_width_scale.get_value());
    });

    Gtk::ColorButton color_button;
    color_button.set_rgba(Gdk::RGBA("#000000"));
    color_button.signal_color_set().connect([&]() {
        draw_area.set_line_color(color_button.get_rgba());
    });

    Gtk::Button quit_button("Quit");
    quit_button.signal_clicked().connect([&]() {
        app->quit();
    });

    Gtk::Button save_image_button("Save Image");
    save_image_button.signal_clicked().connect([&]() {
        draw_area.save_image();
    });

    Gtk::Box box(Gtk::ORIENTATION_VERTICAL);
    box.pack_start(draw_button, Gtk::PACK_SHRINK);
    box.pack_start(remove_last_line_button, Gtk::PACK_SHRINK);
    box.pack_start(clear_button, Gtk::PACK_SHRINK);
    box.pack_start(save_image_button, Gtk::PACK_SHRINK);
    box.pack_start(line_width_scale, Gtk::PACK_SHRINK);
    box.pack_start(color_button, Gtk::PACK_SHRINK);
    box.pack_start(quit_button, Gtk::PACK_SHRINK);


    Gtk::Box main_box(Gtk::ORIENTATION_HORIZONTAL);
    main_box.pack_start(draw_area, Gtk::PACK_EXPAND_WIDGET);
    main_box.pack_start(box, Gtk::PACK_SHRINK);

    window.add(main_box);
    window.show_all();

    return app->run(window);
}
