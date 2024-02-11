#include <iostream>
#include <sstream>

#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "elfio/elfio_dump.hpp"
#include "utils.h"

#define DUMP_DEC_FORMAT( width ) \
    std::setw( width ) << std::setfill( ' ' ) << std::dec << std::right
#define DUMP_HEX0x_FORMAT( width ) \
    "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_HEX_FORMAT( width ) \
    std::setw( width ) << std::setfill( '0' ) << std::hex << std::right
#define DUMP_STR_FORMAT( width ) \
    std::setw( width ) << std::setfill( ' ' ) << std::hex << std::left

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _readerLoaded = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void section_data(std::ostream &out, const ELFIO::section *sec)
{
    const char* pdata = sec->get_data();
    if ( pdata ) {
        ELFIO::Elf_Xword i;
        for ( i = 0; i < sec->get_size(); ++i ) {
            if ( i % 16 == 0 ) {
                out << "[" << DUMP_HEX0x_FORMAT( 8 ) << i << "]";
            }

            out << " " << DUMP_HEX0x_FORMAT( 2 )
                << ( pdata[i] & 0x000000FF );

            if ( i % 16 == 15 ) {
                out << std::endl;
            }
        }
        if ( i % 16 != 0 ) {
            out << std::endl;
        }
    }
}


void MainWindow::on_chooseFileBtn_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setWindowTitle("Выберите файл");
    dialog.setNameFilter("All Files (*);");

    QString selectedFile = "";
    if (dialog.exec())
    {
        selectedFile = dialog.selectedFiles().at(0);
    }
    else
    {
        return;
    }

    if (!_reader.load(selectedFile.toStdString()) ) {
        QMessageBox msg;
        msg.setText("Невозможно открыть выбранный файл");
        msg.exec();
        return;
    }

    _readerLoaded = true;
    _pathToFile = selectedFile;

    std::stringstream osHeader;
    ELFIO::dump::header( osHeader, _reader);
    ui->unputFileInfoTextEdit->setText(QString::fromStdString(osHeader.str()));

    std::stringstream osText;
    const ELFIO::section* sec = utils::get_section_with_name(&_reader, ".text");
    if (sec == nullptr)
    {
        QMessageBox msg;
        msg.setText("Файл не содержит секции .text");
        msg.exec();
        return;
    }

    section_data(osText, sec);
    ui->inputTextContentTextEdit->setText(QString::fromStdString(osText.str()));

    ui->inputCodeSectionSizeLineEdit->setText(QString::number(sec->get_size()));
}


void MainWindow::on_compressFileBtn_clicked()
{
    if (!_readerLoaded)
    {
        QMessageBox msg;
        msg.setText("Исходный файл не выбран");
        msg.exec();
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save File"),
                                                    "/home/user/Documents",
                                                    tr("Text Files (*.txt);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    utils::config_builder cfg_builder;
    switch (ui->maskFormatComboBox->currentIndex())
    {
        case 0:
            cfg_builder.set_etype(encode_type::DICT);
            break;
        case 1:
            cfg_builder.set_etype(encode_type::MASK_SINGLE);
            break;
        case 2:
            cfg_builder.set_etype(encode_type::MASK_DUO);
            break;
        case 3:
            cfg_builder.set_etype(encode_type::MASK_OPERANDS_OPCODE);
            break;
        case 4:
            cfg_builder.set_etype(encode_type::MASK_DUO_QUAD);
            break;
        case 5:
            cfg_builder.set_etype(encode_type::MASK_QUAD);
            break;
    }

    utils::size_stat sz_stat;
    std::vector<std::string> dict_infos;
    utils::config cfg = cfg_builder.build();

    ELFIO::elfio writer;
    if (!writer.load(_pathToFile.toStdString()) ) {
        QMessageBox msg;
        msg.setText("Выбранный файл недоступен");
        msg.exec();
        return;
    }
    utils::compress_executable(sz_stat, dict_infos, &writer, cfg);

    bool v = writer.save(fileName.toStdString());
    if (!v) {
        QMessageBox msg;
        msg.setText("Ошибка при сохранении файла: \n" + QString::fromStdString(writer.validate()));
        msg.exec();
        return;
    }

    ui->startTextSizeLineEdit->setText(QString::number(sz_stat.initial_code_size));
    ui->cmprTextSizeLineEdit->setText(QString::number(sz_stat.final_code_size));
    ui->dictSizeLineEdit->setText(QString::number(sz_stat.dict_32_bit_size));
    ui->totalSizeLineEdit->setText(QString::number(sz_stat.final_code_size + sz_stat.dict_32_bit_size));
    ui->cmprRateLineEdit->setText(QString::number(double(sz_stat.initial_code_size) / (sz_stat.final_code_size + sz_stat.dict_32_bit_size)));

    ui->dict1Name->setText("");
    ui->dict2Name->setText("");
    ui->dict3Name->setText("");
    ui->dict4Name->setText("");
    ui->dict1TextEdit->setText("");
    ui->dict2TextEdit->setText("");
    ui->dict3TextEdit->setText("");
    ui->dict4TextEdit->setText("");
    switch (ui->maskFormatComboBox->currentIndex())
    {
        case 0:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict1Name->setText(".dict");
            break;
        case 1:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict1Name->setText(".dict");
            break;
        case 2:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict2TextEdit->setText(QString::fromStdString(dict_infos[1]));
            ui->dict1Name->setText(".dict1");
            ui->dict2Name->setText(".dict2");
            break;
        case 3:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict2TextEdit->setText(QString::fromStdString(dict_infos[1]));
            ui->dict1Name->setText(".dict_operands");
            ui->dict2Name->setText(".dict_opcode");
            break;
        case 4:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict2TextEdit->setText(QString::fromStdString(dict_infos[1]));
            ui->dict3TextEdit->setText(QString::fromStdString(dict_infos[2]));
            ui->dict1Name->setText(".dict1");
            ui->dict2Name->setText(".dict21");
            ui->dict3Name->setText(".dict22");
            break;
        case 5:
            ui->dict1TextEdit->setText(QString::fromStdString(dict_infos[0]));
            ui->dict2TextEdit->setText(QString::fromStdString(dict_infos[1]));
            ui->dict3TextEdit->setText(QString::fromStdString(dict_infos[2]));
            ui->dict4TextEdit->setText(QString::fromStdString(dict_infos[3]));
            ui->dict1Name->setText(".dict11");
            ui->dict2Name->setText(".dict12");
            ui->dict3Name->setText(".dict21");
            ui->dict4Name->setText(".dict22");
            break;
    }
}


void MainWindow::on_decompressFileBtn_clicked()
{
    if (!_readerLoaded)
    {
        QMessageBox msg;
        msg.setText("Исходный файл не выбран");
        msg.exec();
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save File"),
                                                    "/home/user/Documents",
                                                    tr("All Files (*);;Text Files (*.txt)"));
    if (fileName.isEmpty())
        return;

    ELFIO::elfio writer;
    if (!writer.load(_pathToFile.toStdString()) ) {
        QMessageBox msg;
        msg.setText("Выбранный файл недоступен");
        msg.exec();
        return;
    }

    try {
        utils::decompress_executable(&writer);
    }  catch (std::runtime_error &ex) {
        QMessageBox msg;
        msg.setText("Невозможно выполнить декомпрессию (убедитесь, что выбран сжатый файл)");
        msg.exec();
        return;
    }

    bool v = writer.save(fileName.toStdString());
    if (!v) {
        QMessageBox msg;
        msg.setText("Ошибка при сохранении файла: \n" + QString::fromStdString(writer.validate()));
        msg.exec();
        return;
    }
}

