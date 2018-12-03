#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractItemModel>
#include <QMainWindow>
#include <mysqldemolib.h>
#include <string>

namespace Ui {
class MainWindow;
}

struct sqlServerInfo {
    std::wstring _strServerUrl;
    std::wstring _dbName;
    std::wstring _tbName;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void init();

    bool checkLink() {return m_pDemo != NULL;}
    void message(const QString& text);
    void setTableView(std::vector<std::wstring>& headers, std::vector<TableItem>& contents);
    void showDatabases();
    void showTables(std::wstring& dbName);
    void showTable(const std::wstring &dbName, const std::wstring &tbName, const int perPageNum = 5, const int page = 0);
    void updatePath();
    void getFileNames(std::wstring dir_path, std::vector<std::wstring>& files);

private slots:
    void onBackPath();
    void onPrevPage();
    void onNextPage();
    void dataChanged(const QModelIndex &topLeft,const QModelIndex &bottomRight,const QVector<int> &roles);
    void onLinkServer();
    void onCreateDatabase();
    void onDeleteDatabase();
    void onOpenDatabase();
    void onCreateTable();
    void onDeleteTable();
    void onOpenTable();
    void onInsertToTable();
    void onDeleteFromTable();
    void onSearchInTable();
    void onModifyInTable();
    void onOpenImageDemo();

    void onSpeedCheckInsert();
    void onSpeedCheckImage();

private:
    Ui::MainWindow *ui;
    MysqlDemoLib* m_pDemo;
    sqlServerInfo m_server;
    int m_curPage;
    int m_perPageNum;
    int m_tolPages;
    bool m_bChange;
};

#endif // MAINWINDOW_H
