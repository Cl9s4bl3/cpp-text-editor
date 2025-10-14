#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#define HEADER_HEIGHT 25
#define CONFIG_FILE "editor.conf"
#define FALLBACK_FONT_SIZE 20
#define DEFAULT_FONT_SIZE 20

void select_file_cb(Fl_Widget *w, void* data);
void save_file_cb(Fl_Widget *w, void* data);
void save_as_cb(Fl_Widget *w, void* data);

class FixedButton : public Fl_Button {
    int fixed_x, fixed_w;
    public:
        FixedButton(int X, int Y, int W, int H, const char* L = 0)
            : Fl_Button(X, Y, W, H, L), fixed_x(X), fixed_w(W) {}

        void resize(int X, int Y, int W, int H) override {
            Fl_Widget::resize(fixed_x, Y, fixed_w, H);
        }
};

Fl_Text_Editor *textedit;

void saveConfig(const std::string& data){
    try {
        std::ofstream outFile(CONFIG_FILE);

        if (!outFile){
            std::cerr << "Failed to open '" << CONFIG_FILE << "' for writing." << std::endl;
            return;
        }

        outFile << data;
        outFile.close();
    } catch (const std::exception& e){
        std::cerr << "An error occurred while trying to save config file. Error: " << e.what() << std::endl;
    }
}

int loadFontSize(){
    try {
        std::ifstream inFile(CONFIG_FILE);
        if (!inFile){
            std::cerr << "Failed to open '" << CONFIG_FILE << "' for reading. Setting " << FALLBACK_FONT_SIZE << " as font size." << std::endl;
            return FALLBACK_FONT_SIZE;
        }

        std::string line;
        std::getline(inFile, line);

        int font_size = std::stoi(line);

        return font_size;
    } catch (const std::invalid_argument&){
        std::cerr << "Invalid integer in config file. Setting " << FALLBACK_FONT_SIZE << " as font size." << std::endl;
        return FALLBACK_FONT_SIZE;
    } catch (const std::out_of_range&){
        std::cerr << "Font size is out of integer range. Setting " << FALLBACK_FONT_SIZE << " as font size." << std::endl;
        return FALLBACK_FONT_SIZE;
    } catch (const std::exception& e){
        std::cerr << "An error occurred while trying to load font size. Setting " << FALLBACK_FONT_SIZE << " as font size. Error: " << e.what() << std::endl;
        return FALLBACK_FONT_SIZE;
    }
}

int global_handler(int event) {
    if (!textedit) return 0;
    if (event == FL_SHORTCUT) {
        int key = Fl::event_key();
        if (key == 'i'){
            int sz = textedit->textsize();
            if (sz >= __INT32_MAX__){
                return 0;
            }
            saveConfig(std::to_string(sz + 5));
            textedit->textsize(sz + 5);
            textedit->redisplay_range(0, textedit->buffer()->length());
            textedit->resize(textedit->x(), textedit->y(), textedit->w(), textedit->h());
            textedit->redraw();
            return 1;
        } else if (key == 'o'){
            int sz = textedit->textsize();
            if (sz <= 5){
                return 0;
            }
            saveConfig(std::to_string(sz - 5));
            textedit->textsize(sz - 5);
            textedit->redisplay_range(0, textedit->buffer()->length());
            textedit->resize(textedit->x(), textedit->y(), textedit->w(), textedit->h());
            textedit->redraw();
            return 1;
        }
    }

    return 0;
}
class Notification : public Fl_Box {
    public:
        std::string current_message;
        Notification(int X, int Y, int W, int H)
            : Fl_Box(X, Y, W, H, "") {
            box(FL_FLAT_BOX);
            color(fl_rgb_color(255, 220, 120));
            labelsize(10);
            labelfont(FL_HELVETICA_BOLD);
            hide();
        }

        void show_message(const std::string& msg, int duration_ms, Fl_Color notif_color = FL_YELLOW) {
            current_message = msg;
            label(current_message.c_str());
            color(notif_color);
            show();
            redraw();

            Fl::remove_timeout(notification_timeout_cb, this);
            
            Fl::add_timeout(duration_ms / 1000.0, notification_timeout_cb, this);
        }

        static void notification_timeout_cb(void* v) {
            auto* self = static_cast<Notification*>(v);
            self->hide();
            self->redraw();
        }
};

inline void notify(Notification* notif, const std::string& msg, int duration, Fl_Color notif_color = FL_YELLOW) {
    notif->show_message(msg, duration, notif_color);
}

class Editor {
    public:
        Fl_Window *window;
        Fl_Box *container;
        Fl_Box *header;

        FixedButton *header_open_button;
        FixedButton *header_save_button;
        FixedButton *header_save_as_button;
        FixedButton *header_settings_button;

        Notification *notif;

        Fl_Text_Buffer *textbuf;

        Editor(){
            window = new Fl_Window(400, 300, "Editor");

            header = new Fl_Box(0, 0, window->w(), HEADER_HEIGHT);
            header->box(FL_FLAT_BOX);
            header->color(FL_DARK1);

            container = new Fl_Box(0, HEADER_HEIGHT, window->w(), window->h() - HEADER_HEIGHT);
            container->box(FL_FLAT_BOX);

            header_open_button = new FixedButton(0, 0, 60, HEADER_HEIGHT, "Open");
            header_open_button->callback(select_file_cb, this);
            header_open_button->box(FL_NO_BOX);

            header_save_button = new FixedButton(70, 0, 60, HEADER_HEIGHT, "Save");
            header_save_button->callback(save_file_cb, this);
            header_save_button->shortcut(FL_CTRL | 's');
            header_save_button->box(FL_NO_BOX);

            header_save_as_button = new FixedButton(140, 0, 60, HEADER_HEIGHT, "Save as");
            header_save_as_button->box(FL_NO_BOX);
            header_save_as_button->callback(save_as_cb ,this);

            header_settings_button = new FixedButton(210, 0, 60, HEADER_HEIGHT, "Settings");
            header_settings_button->box(FL_NO_BOX);

            notif = new Notification(0, 0, window->w(), HEADER_HEIGHT);

            textbuf = new Fl_Text_Buffer();
            textedit = new Fl_Text_Editor(0, HEADER_HEIGHT, window->w(), window->h());

            textedit->buffer(textbuf);
            textbuf->text("");
            textedit->textsize(loadFontSize());

            textedit->color(FL_GRAY);

            window->resizable(container);
            window->size_range(400, 300);
            window->end();
            window->show();

            Fl::add_handler(global_handler);
        }

        std::string open_file = "";

        void select_file(int type){ // Type 1: File selection, Type 2: Save as
            Fl_Native_File_Chooser browser;

            if (type == 1){
                browser.title("Open a file");
                browser.type(Fl_Native_File_Chooser::BROWSE_FILE);
                browser.directory(".");

                switch (browser.show()){
                    case -1: std::cout << "Error: " << browser.errmsg() << std::endl; break;
                    case 1: std::cout << "Action cancelled" << std::endl; break;
                    default: loadContent(browser.filename()); break;
                }
            } else if (type == 2){
                browser.title("Save file as");
                browser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
                browser.directory(".");

                switch (browser.show()){
                    case -1: std::cout << "Error: " << browser.errmsg() << std::endl; break;
                    case 1: std::cout << "Action cancelled" << std::endl; break;
                    default: save_as(browser.filename()); break;
                }
            }
        }

        void loadContent(const char * filepath){
            try {
                std::ifstream inFile(filepath);
                if (!inFile){
                    std::cerr << "Failed to open '" << filepath << "' for reading." << std::endl;
                    return;
                }

                std::string line;
                std::vector<std::string> data;
                while (std::getline(inFile, line)){
                    std::string formatted_line = line + "\n";
                    data.push_back(formatted_line);
                }

                textbuf->text("");

                for (const std::string& l : data){
                    textbuf->append(l.c_str());
                }
                inFile.close();
                open_file = filepath;

                notify(notif, ("Successfully loaded file " + open_file + "."), 2000, FL_YELLOW);
            } catch (const std::exception& e){
                std::cerr << "An error occurred while trying to load content for '" << filepath << "'. Error: " << e.what() << std::endl;
                return;
            }
        }

        void save_file(){
            try {
                const std::string data = textbuf->text();

                if (open_file.empty()){
                    notify(notif, "No file is open: cannot save", 2000, FL_RED);
                    return;
                }

                std::ofstream outFile(open_file);
                if (!outFile){
                    std::cerr << "Failed to open '" << open_file << "' for writing." << std::endl;
                    return;
                }

                outFile << data;
                outFile.close();

                notify(notif, ("Successfully saved to " + open_file + "."), 2000, FL_GREEN);
            } catch (const std::exception& e){
                std::cerr << "An error occurred while trying to save content for '" << open_file << "'. Error: " << e.what() << std::endl;
                return;
            }
        }

        void save_as(const char * filepath){
            try {
                const std::string data = textbuf->text();

                std::ofstream outFile(filepath);
                if (!outFile){
                    std::cerr << "Failed to open '" << filepath << "' for writing." << std::endl;
                    return;
                }

                outFile << data;
                outFile.close();

                open_file = filepath;
                notify(notif, ("Successfully created and saved to " + open_file + "."), 2000, FL_GREEN);
            } catch (const std::exception& e){
                std::cerr << "An error occurred while trying to save content for '" << open_file << "'. Error: " << e.what() << std::endl;
                return;
            }
        }
};

void select_file_cb(Fl_Widget *w, void* data){
    auto *app = static_cast<Editor*>(data);
    app->select_file(1);
}

void save_file_cb(Fl_Widget *w, void* data){
    auto *app = static_cast<Editor*>(data);
    app->save_file();
}

void save_as_cb(Fl_Widget *w, void* data){
    auto *app = static_cast<Editor*>(data);
    app->select_file(2);
}

void generateConfigFile(){
    try {

        if (std::filesystem::exists(CONFIG_FILE)){
            return;
        }

        std::ofstream outFile(CONFIG_FILE);
        if (!outFile){
            std::cerr << "Failed to open '" << CONFIG_FILE << "' for writing." << std::endl;
            return;
        }

        outFile << DEFAULT_FONT_SIZE;
        outFile.close();
    } catch (const std::exception& e){
        std::cerr << "An error occurred while trying to generate config file. Error: " << e.what() << std::endl;
        return;
    }
}

int main(){
    generateConfigFile();
    Editor editor;
    return Fl::run();
}