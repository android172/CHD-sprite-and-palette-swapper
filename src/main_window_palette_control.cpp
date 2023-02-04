#include "main_window.h"
#include "./ui_main_window.h"

#include "image_lwi.h"

#include <fstream>
#include <QColorDialog>
#include <QFileDialog>

// ///////////////////////// //
// MAIN WINDOW PALETTE SLOTS //
// ///////////////////////// //

#define on_click(fun, r_fun, butt)                                             \
    void MainWindow::fun() { on_palette_button_clicked(ui->butt); }            \
    void MainWindow::r_fun() { on_palette_button_right_clicked(ui->butt); }

on_click(on_bt_col_0_clicked, on_bt_col_0_right_clicked, bt_col_0);
on_click(on_bt_col_1_clicked, on_bt_col_1_right_clicked, bt_col_1);
on_click(on_bt_col_2_clicked, on_bt_col_2_right_clicked, bt_col_2);
on_click(on_bt_col_3_clicked, on_bt_col_3_right_clicked, bt_col_3);
on_click(on_bt_col_4_clicked, on_bt_col_4_right_clicked, bt_col_4);
on_click(on_bt_col_5_clicked, on_bt_col_5_right_clicked, bt_col_5);
on_click(on_bt_col_6_clicked, on_bt_col_6_right_clicked, bt_col_6);
on_click(on_bt_col_7_clicked, on_bt_col_7_right_clicked, bt_col_7);
on_click(on_bt_col_8_clicked, on_bt_col_8_right_clicked, bt_col_8);
on_click(on_bt_col_9_clicked, on_bt_col_9_right_clicked, bt_col_9);
on_click(on_bt_col_A_clicked, on_bt_col_A_right_clicked, bt_col_A);
on_click(on_bt_col_B_clicked, on_bt_col_B_right_clicked, bt_col_B);
on_click(on_bt_col_C_clicked, on_bt_col_C_right_clicked, bt_col_C);
on_click(on_bt_col_D_clicked, on_bt_col_D_right_clicked, bt_col_D);
on_click(on_bt_col_E_clicked, on_bt_col_E_right_clicked, bt_col_E);
on_click(on_bt_col_F_clicked, on_bt_col_F_right_clicked, bt_col_F);

// ///////////////////// //
// MAIN WINDOW COL SLOTS //
// ///////////////////// //

void MainWindow::on_bt_import_col_clicked() {
    const auto file_path = QFileDialog::getOpenFileName(
        this, "Import col file", _default_col_import_location, "CHD_col (*.col)"
    );
    if (!file_path.isEmpty()) import_col(file_path);
}

void MainWindow::on_bt_palette_to_col_clicked() {
    // Compute current palette hex
    QString palette_hex {};
    for (const auto& color_pair : _palette_colors) {
        Color c = color_pair.display;
        palette_hex += (char)(c.b / 8);
        palette_hex += (char)(c.g / 8);
        palette_hex += (char)(c.r / 8);
        palette_hex += '\000';
    }
    palette_hex.chop(1);

    // Check selected color set
    int  current_color_set = 0;
    auto selected_text     = ui->cb_color_set->currentText();
    if (selected_text.compare("default") != 0)
        current_color_set = selected_text.toInt();

    // Open col file
    std::ofstream col_file { _current_col_path.toStdString(),
                             std::ios::out | std::ios::binary | std::ios::in };
    col_file.seekp(0x20 + current_color_set * 0x40);

    // Replace color bytes
    col_file.write(palette_hex.toStdString().data(), palette_hex.size());
    col_file.close();
}

// /////////////////////////////////////////// //
// MAIN WINDOW PALETTE CONTROL PRIVATE METHODS //
// /////////////////////////////////////////// //

void MainWindow::import_col(QString file_path) {
    // Save location
    const QString file_name      = file_path.split('/').last();
    const QString file_dir       = file_path.chopped(file_name.size());
    _default_col_import_location = file_dir;
    save_app_state();

    ui->label_col->setText(file_name);

    _current_col_path = file_path;
    ui->bt_palette_to_col->setEnabled(true);
}

void MainWindow::on_palette_button_clicked(ColorButton* button) {
    if (_palette_colors.size() <= button->color_index) return;

    auto   c = _palette_colors[button->color_index].display;
    QColor initial_color { c.r, c.g, c.b };

    QColor new_color = QColorDialog::getColor(initial_color);
    if (new_color.isValid() == false) return;
    Color nc { (unsigned char) new_color.red(),
               (unsigned char) new_color.green(),
               (unsigned char) new_color.blue() };
    _palette_colors[button->color_index].display = nc;

    // Setup button colors
    button->set_color();

    // Reload drawn image
    on_list_images_currentItemChanged(ui->list_images->currentItem(), nullptr);
}

void MainWindow::on_palette_button_right_clicked(ColorButton* button) {
    if (button->color_index == 0 ||
        _palette_colors.size() <= button->color_index)
        return;

    // Rearrange palette
    const auto temp                      = _palette_colors[button->color_index];
    _palette_colors[button->color_index] = _palette_colors[0];
    _palette_colors[0]                   = temp;

    // Sort & recompute hash
    std::sort(_palette_colors.begin() + 1, _palette_colors.end());
    _color_index.clear();
    for (int i = 0; i < _palette_colors.size(); ++i)
        _color_index[_palette_colors[i].original] = i;

    // Inform images of changed palette
    for (int i = 0; i < ui->list_images->count(); i++) {
        auto image = dynamic_cast<ImageLWI*>(ui->list_images->item(i));
        image->update_color();
    }

    // Setup button colors
    bt_col_ALL(set_color());
}

void MainWindow::setup_palette(QImage image) {
    // Compute palette
    _palette_colors.clear();
    for (int i = 0; i < image.width(); ++i) {
        for (int j = 0; j < image.height(); ++j) {
            QColor pixel { image.pixel(j, i) };
            Color  color { (unsigned char) pixel.red(),
                          (unsigned char) pixel.green(),
                          (unsigned char) pixel.blue() };

            if (_palette_colors.contains({ color, color }) == false)
                _palette_colors.push_back({ color, color });
        }
    }
    if (_palette_colors.size() > 16) _palette_colors.resize(16);
    std::sort(_palette_colors.begin() + 1, _palette_colors.end());
    // Compute util palette structures
    _color_index.clear();
    for (int i = 0; i < _palette_colors.size(); ++i)
        _color_index[_palette_colors[i].original] = i;

    // Setup button colors
    bt_col_ALL(set_color());
}

void MainWindow::update_palette(QImage image) {
    // Compute palette
    auto initial_size = _palette_colors.size();
    for (int i = 0; i < image.width(); ++i) {
        for (int j = 0; j < image.height(); ++j) {
            QColor pixel { image.pixel(j, i) };
            Color  color { (unsigned char) pixel.red(),
                          (unsigned char) pixel.green(),
                          (unsigned char) pixel.blue() };

            if (_palette_colors.contains({ color, color }) == false)
                _palette_colors.push_back({ color, color });
        }
    }
    if (_palette_colors.size() > 16) _palette_colors.resize(16);
    if (initial_size < _palette_colors.size()) {
        std::sort(_palette_colors.begin() + 1, _palette_colors.end());
        _color_index.clear();
        for (int i = 0; i < _palette_colors.size(); ++i)
            _color_index[_palette_colors[i].original] = i;
    }

    // Inform images of changed palette
    for (int i = 0; i < ui->list_images->count(); i++) {
        auto image = dynamic_cast<ImageLWI*>(ui->list_images->item(i));
        image->update_color();
    }

    // Setup button colors
    bt_col_ALL(set_color());
}

// // Clipboard copy (for reference)
// QClipboard* clipboard = QGuiApplication::clipboard();
// clipboard->setText(palette_hex);