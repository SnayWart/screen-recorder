#include "gui_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QStyle>
#include <QMenu>
#include <QMessageBox>
#include <cstdio>
#include <array>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), engine_(std::make_unique<RecorderEngine>()) {
    setup_ui();

    stats_timer_ = new QTimer(this);
    connect(stats_timer_, &QTimer::timeout, this, &MainWindow::update_stats);
}

MainWindow::~MainWindow() {
    if (engine_ && engine_->is_recording()) {
        engine_->stop();
    }
}

void MainWindow::setup_ui() {
    setWindowTitle("Экранный рекордер");
    setMinimumWidth(440);

    auto *central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    auto *main_layout = new QVBoxLayout(central_widget);
    main_layout->setSpacing(10);
    main_layout->setContentsMargins(12, 12, 12, 12);

    status_label_ = new QLabel("Готов к работе", this);
    status_label_->setAlignment(Qt::AlignCenter);
    QFont status_font = status_label_->font();
    status_font.setBold(true);
    status_font.setPointSize(11);
    status_label_->setFont(status_font);
    main_layout->addWidget(status_label_);

    stats_label_ = new QLabel("Время: 00:00:00 | Размер: 0.0 МБ | FPS: 0", this);
    stats_label_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(stats_label_);

    auto *group_capture = new QGroupBox("Область захвата", this);
    auto *vbox_capture = new QVBoxLayout(group_capture);

    radio_fullscreen_ = new QRadioButton("Весь экран", group_capture);
    radio_fullscreen_->setChecked(true);
    vbox_capture->addWidget(radio_fullscreen_);

    auto *hbox_region = new QHBoxLayout();
    radio_region_ = new QRadioButton("Выбранная область", group_capture);
    btn_select_region_ = new QPushButton("Выбрать рамкой...", group_capture);
    hbox_region->addWidget(radio_region_);
    hbox_region->addWidget(btn_select_region_);
    vbox_capture->addLayout(hbox_region);

    region_info_label_ = new QLabel("Область не выбрана", group_capture);
    region_info_label_->setStyleSheet("color: gray;");
    vbox_capture->addWidget(region_info_label_);

    main_layout->addWidget(group_capture);

    auto *group_audio = new QGroupBox("Звук", this);
    auto *vbox_audio = new QVBoxLayout(group_audio);
    chk_desktop_audio_ = new QCheckBox("Звук системы (динамики)", group_audio);
    chk_desktop_audio_->setChecked(true);
    chk_microphone_ = new QCheckBox("Микрофон", group_audio);
    chk_microphone_->setChecked(false);
    vbox_audio->addWidget(chk_desktop_audio_);
    vbox_audio->addWidget(chk_microphone_);
    main_layout->addWidget(group_audio);

    auto *group_video = new QGroupBox("Настройки видео", this);
    auto *hbox_video = new QHBoxLayout(group_video);

    auto *lbl_fps = new QLabel("Частота:", group_video);
    combo_fps_ = new QComboBox(group_video);
    combo_fps_->addItem("75 FPS", 75);
    combo_fps_->addItem("60 FPS", 60);
    combo_fps_->addItem("30 FPS", 30);
    combo_fps_->setCurrentIndex(0);

    auto *lbl_quality = new QLabel("Качество:", group_video);
    combo_quality_ = new QComboBox(group_video);
    combo_quality_->addItem("Сверхчеткое (CRF 16)", 16);
    combo_quality_->addItem("Высокое (CRF 18)", 18);
    combo_quality_->addItem("Стандартное (CRF 22)", 22);
    combo_quality_->addItem("Сжатое (CRF 28)", 28);
    combo_quality_->setCurrentIndex(1);

    hbox_video->addWidget(lbl_fps);
    hbox_video->addWidget(combo_fps_);
    hbox_video->addSpacing(15);
    hbox_video->addWidget(lbl_quality);
    hbox_video->addWidget(combo_quality_);
    main_layout->addWidget(group_video);

    auto *group_output = new QGroupBox("Папка сохранения", this);
    auto *hbox_output = new QHBoxLayout(group_output);
    QString default_dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (default_dir.isEmpty()) {
        default_dir = QDir::homePath() + "/Videos";
    }
    edit_output_dir_ = new QLineEdit(default_dir, group_output);
    btn_browse_path_ = new QPushButton("Обзор...", group_output);
    hbox_output->addWidget(edit_output_dir_);
    hbox_output->addWidget(btn_browse_path_);
    main_layout->addWidget(group_output);

    btn_record_ = new QPushButton("Начать запись", this);
    btn_record_->setMinimumHeight(44);
    QFont btn_font = btn_record_->font();
    btn_font.setBold(true);
    btn_font.setPointSize(12);
    btn_record_->setFont(btn_font);
    main_layout->addWidget(btn_record_);

    connect(btn_record_, &QPushButton::clicked, this, &MainWindow::on_record_clicked);
    connect(btn_select_region_, &QPushButton::clicked, this, &MainWindow::on_select_region_clicked);
    connect(btn_browse_path_, &QPushButton::clicked, this, &MainWindow::on_browse_path_clicked);
    connect(radio_fullscreen_, &QRadioButton::toggled, this, &MainWindow::on_mode_changed);
    connect(radio_region_, &QRadioButton::toggled, this, &MainWindow::on_mode_changed);

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        tray_icon_ = new QSystemTrayIcon(this);
        tray_icon_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        auto *tray_menu = new QMenu(this);
        auto *act_toggle = tray_menu->addAction("Показать / Скрыть");
        auto *act_stop = tray_menu->addAction("Остановить запись");
        tray_menu->addSeparator();
        auto *act_quit = tray_menu->addAction("Выход");

        connect(act_toggle, &QAction::triggered, this, [this] {
            if (isVisible()) hide();
            else { show(); raise(); activateWindow(); }
        });
        connect(act_stop, &QAction::triggered, this, [this] {
            if (engine_->is_recording()) on_record_clicked();
        });
        connect(act_quit, &QAction::triggered, this, &MainWindow::close);

        tray_icon_->setContextMenu(tray_menu);
        connect(tray_icon_, &QSystemTrayIcon::activated, this, &MainWindow::on_tray_activated);
        tray_icon_->show();
    }
}

void MainWindow::on_mode_changed() {
    if (radio_fullscreen_->isChecked()) {
        region_info_label_->setText("Область не используется (полный экран)");
    } else {
        if (selected_region_.width > 0 && selected_region_.height > 0) {
            region_info_label_->setText(QString("Область: %1x%2 (+%3,+%4)")
                .arg(selected_region_.width).arg(selected_region_.height)
                .arg(selected_region_.x).arg(selected_region_.y));
        } else {
            region_info_label_->setText("Область не выбрана. Нажмите «Выбрать рамкой...»");
        }
    }
}

void MainWindow::on_select_region_clicked() {
    hide();
    FILE *pipe = popen("slurp -f \"%x %y %w %h\" 2>/dev/null", "r");
    std::array<char, 128> buf;
    std::string output;
    if (pipe) {
        if (fgets(buf.data(), buf.size(), pipe) != nullptr) {
            output = buf.data();
        }
        pclose(pipe);
    }
    show();
    raise();
    activateWindow();

    int x = 0, y = 0, w = 0, h = 0;
    if (sscanf(output.c_str(), "%d %d %d %d", &x, &y, &w, &h) == 4 && w > 10 && h > 10) {
        selected_region_.x = x;
        selected_region_.y = y;
        selected_region_.width = (w / 2) * 2;
        selected_region_.height = (h / 2) * 2;
        radio_region_->setChecked(true);
        region_info_label_->setText(QString("Выбрано: %1x%2 (+%3,+%4)")
            .arg(selected_region_.width).arg(selected_region_.height)
            .arg(selected_region_.x).arg(selected_region_.y));
    }
}

void MainWindow::on_browse_path_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для сохранения видео",
                                                    edit_output_dir_->text());
    if (!dir.isEmpty()) {
        edit_output_dir_->setText(dir);
    }
}

std::string MainWindow::generate_output_filename() {
    QString dir = edit_output_dir_->text().trimmed();
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/Videos";
    }
    QDir().mkpath(dir);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString full_path = dir + "/Record_" + timestamp + ".mp4";
    return full_path.toStdString();
}

void MainWindow::on_record_clicked() {
    if (!engine_->is_recording()) {
        RecorderConfig cfg;
        cfg.mode = radio_fullscreen_->isChecked() ? CaptureMode::FULL_SCREEN : CaptureMode::REGION;
        if (cfg.mode == CaptureMode::REGION) {
            if (selected_region_.width <= 0 || selected_region_.height <= 0) {
                QMessageBox::warning(this, "Внимание", "Пожалуйста, сначала выберите область экрана рамкой.");
                return;
            }
            cfg.region = selected_region_;
        }

        cfg.video.fps = combo_fps_->currentData().toInt();
        cfg.video.crf = combo_quality_->currentData().toInt();
        cfg.video.preset = "ultrafast";

        cfg.audio.capture_desktop = chk_desktop_audio_->isChecked();
        cfg.audio.capture_mic = chk_microphone_->isChecked();

        cfg.output_path = generate_output_filename();

        if (engine_->start(cfg)) {
            btn_record_->setText("Остановить запись");
            status_label_->setText("● Идет запись...");
            status_label_->setStyleSheet("color: #e53935;");
            if (tray_icon_) {
                tray_icon_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
            }

            radio_fullscreen_->setEnabled(false);
            radio_region_->setEnabled(false);
            btn_select_region_->setEnabled(false);
            chk_desktop_audio_->setEnabled(false);
            chk_microphone_->setEnabled(false);
            combo_fps_->setEnabled(false);
            combo_quality_->setEnabled(false);
            edit_output_dir_->setEnabled(false);
            btn_browse_path_->setEnabled(false);

            stats_timer_->start(250);
        } else {
            status_label_->setText("Ошибка запуска записи");
            status_label_->setStyleSheet("color: red;");
            QMessageBox::critical(this, "Ошибка", "Не удалось инициализировать захват экрана или энкодер.");
        }
    } else {
        stats_timer_->stop();
        engine_->stop();

        btn_record_->setText("Начать запись");
        status_label_->setText("Запись сохранена");
        status_label_->setStyleSheet("");
        if (tray_icon_) {
            tray_icon_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        }

        radio_fullscreen_->setEnabled(true);
        radio_region_->setEnabled(true);
        btn_select_region_->setEnabled(true);
        chk_desktop_audio_->setEnabled(true);
        chk_microphone_->setEnabled(true);
        combo_fps_->setEnabled(true);
        combo_quality_->setEnabled(true);
        edit_output_dir_->setEnabled(true);
        btn_browse_path_->setEnabled(true);
    }
}

void MainWindow::update_stats() {
    if (!engine_->is_recording()) {
        return;
    }
    auto stats = engine_->get_stats();
    int total_sec = static_cast<int>(stats.elapsed_seconds);
    int hours = total_sec / 3600;
    int mins = (total_sec % 3600) / 60;
    int secs = total_sec % 60;

    double mb = static_cast<double>(stats.bytes_written) / (1024.0 * 1024.0);

    stats_label_->setText(QString("Время: %1:%2:%3 | Размер: %4 МБ | FPS: %5")
        .arg(hours, 2, 10, QChar('0'))
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'))
        .arg(mb, 0, 'f', 1)
        .arg(stats.current_fps, 0, 'f', 1));
}

void MainWindow::on_tray_activated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (engine_->is_recording()) {
        auto res = QMessageBox::question(this, "Запись активна",
            "Запись видео еще продолжается. Остановить и выйти?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (res == QMessageBox::Yes) {
            engine_->stop();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
