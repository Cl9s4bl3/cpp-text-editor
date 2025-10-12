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
#include <FL/Fl_Widget.H>

#define HEADER_HEIGHT 25

void select_file_cb(Fl_Widget *w, void* data);
void save_file_cb(Fl_Widget *w, void* data);

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

int global_handler(int event) {
    if (event == FL_MOUSEWHEEL && Fl::event_state(FL_CTRL)) {
        Fl_Widget *w = Fl::belowmouse();
        if (w == textedit) {
            int dy = Fl::event_dy();
            int sz = textedit->textsize();
            if (dy < 0)
                textedit->textsize(sz + 1);
            else if (dy > 0 && sz > 6)
                textedit->textsize(sz - 1);

            textedit->redraw();
            return 1;
        }
    }

    return 0;
}


class Editor {
    public:
        Fl_Window *window;
        Fl_Box *container;
        Fl_Box *header;

        FixedButton *header_open_button;
        FixedButton *header_save_button;
        FixedButton *header_settings_button;

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

            header_settings_button = new FixedButton(140, 0, 60, HEADER_HEIGHT, "Settings");
            header_settings_button->box(FL_NO_BOX);

            textbuf = new Fl_Text_Buffer();
            textedit = new Fl_Text_Editor(0, HEADER_HEIGHT, window->w(), window->h());

            textedit->buffer(textbuf);
            textbuf->text("");
            textedit->textsize(40);

            textedit->color(FL_GRAY);

            window->resizable(container);
            window->size_range(400, 300);
            window->end();
            window->show();

            Fl::add_handler(global_handler);
        }

        void select_file(){
            Fl_Native_File_Chooser browser;

            browser.title("Open a file");
            browser.type(Fl_Native_File_Chooser::BROWSE_FILE);
            browser.directory(".");

            switch (browser.show()){
                case -1: std::cout << "Error: " << browser.errmsg() << std::endl; break;
                case 1: std::cout << "Action cancelled" << std::endl; break;
                default: loadContent(browser.filename()); break;
            }
        
        }

        std::string open_file = "";

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
            } catch (const std::exception& e){
                std::cerr << "An error occurred while trying to load content for '" << filepath << "'. Error: " << e.what() << std::endl;
                return;
            }
        }

        void save_file(){
            try {
                const std::string data = textbuf->text();

                if (open_file.empty()){
                    return;
                }

                std::ofstream outFile(open_file);
                if (!outFile){
                    std::cerr << "Failed to open '" << open_file << "' for writing." << std::endl;
                    return;
                }

                outFile << data;
                outFile.close();
            } catch (const std::exception& e){
                std::cerr << "An error occurred while trying to save content for '" << open_file << "'. Error: " << e.what() << std::endl;
                return;
            }
        }
};

void select_file_cb(Fl_Widget *w, void* data){
    auto *app = static_cast<Editor*>(data);
    app->select_file();
}

void save_file_cb(Fl_Widget *w, void* data){
    auto *app = static_cast<Editor*>(data);
    app->save_file();
}

int main(){
    Editor editor;
    return Fl::run();
}