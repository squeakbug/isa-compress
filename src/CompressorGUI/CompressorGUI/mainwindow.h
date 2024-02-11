#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "elfio/elfio.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_chooseFileBtn_clicked();

    void on_compressFileBtn_clicked();

    void on_decompressFileBtn_clicked();

private:
    Ui::MainWindow *ui;
    bool _readerLoaded;
    QString _pathToFile;
    ELFIO::elfio _reader;
};
#endif // MAINWINDOW_H
