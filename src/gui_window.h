#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <memory>
#include "recorder_engine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_record_clicked();
    void on_select_region_clicked();
    void on_mode_changed();
    void on_browse_path_clicked();
    void update_stats();
    void on_tray_activated(QSystemTrayIcon::ActivationReason reason);

private:
    void setup_ui();
    std::string generate_output_filename();

    std::unique_ptr<RecorderEngine> engine_;
    QTimer *stats_timer_{nullptr};
    QSystemTrayIcon *tray_icon_{nullptr};

    QLabel *status_label_{nullptr};
    QLabel *stats_label_{nullptr};

    QRadioButton *radio_fullscreen_{nullptr};
    QRadioButton *radio_region_{nullptr};
    QPushButton *btn_select_region_{nullptr};
    QLabel *region_info_label_{nullptr};

    QCheckBox *chk_desktop_audio_{nullptr};
    QCheckBox *chk_microphone_{nullptr};

    QComboBox *combo_fps_{nullptr};
    QComboBox *combo_quality_{nullptr};

    QLineEdit *edit_output_dir_{nullptr};
    QPushButton *btn_browse_path_{nullptr};

    QPushButton *btn_record_{nullptr};

    RegionRect selected_region_{};
};

#endif
