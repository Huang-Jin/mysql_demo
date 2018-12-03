#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>

#include <QMessageBox>
#include <QStandardItemModel>
#include <QTime>
#include <QDebug>
#include <QInputDialog>
#include <QDir>

void findFilesFromDir(std::wstring& path, std::vector<std::wstring>& files) {
    //判断路径是否存在
    QDir dir(QString::fromStdWString(path));
    if(!dir.exists())
        return;

    //查看路径中后缀为.tiff格式的文件
    QStringList filters;
    filters<<QString("*.tiff");
    dir.setFilter(QDir::Files | QDir::NoSymLinks); //设置类型过滤器，只为文件格式
    dir.setNameFilters(filters);  //设置文件名称过滤器，只为filters格式

    //统计cfg格式的文件个数
    int dir_count = dir.count();
    if(dir_count <= 0)
        return;

    //存储文件名称
    for(int i=0; i<dir_count; i++)
    {
        QString file_name = dir[i];  //文件名称
        files.push_back(file_name.toStdWString());
    }
}

void saveBytes2Image(const std::wstring& filename, Bytes& data) {
    QFile file(QString::fromStdWString(filename));
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

//    for(int i=0; i<data._length; ++i) {
//        out << data._data[i];
//    }

    out.writeRawData(data._data, data._length);
    file.close();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    if(m_pDemo != NULL) {
        m_pDemo->closeClient();
        m_pDemo=NULL;
    }
}

void MainWindow::init()
{
    m_pDemo = new MysqlDemoLib();
    m_bChange = false;
    m_perPageNum = 5;
    m_curPage = 0;
    m_tolPages = -1;

    ui->btnPrevPage->setVisible(false);
    ui->btnNextPage->setVisible(false);
    ui->labPages->setVisible(false);

    connect(ui->btnBack, SIGNAL(released()), this, SLOT(onBackPath()));
    connect(ui->btnPrevPage, SIGNAL(released()), this, SLOT(onPrevPage()));
    connect(ui->btnNextPage, SIGNAL(released()), this, SLOT(onNextPage()));
    connect(ui->linkServer, SIGNAL(triggered(bool)), this, SLOT(onLinkServer()));

    connect(ui->createDatabase, SIGNAL(triggered(bool)), this, SLOT(onCreateDatabase()));
    connect(ui->deleteDatabase, SIGNAL(triggered(bool)), this, SLOT(onDeleteDatabase()));
    connect(ui->openDatabase, SIGNAL(triggered(bool)), this, SLOT(onOpenDatabase()));

    connect(ui->createTable, SIGNAL(triggered(bool)), this, SLOT(onCreateTable()));
    connect(ui->deleteTable, SIGNAL(triggered(bool)), this, SLOT(onDeleteTable()));
    connect(ui->openTable, SIGNAL(triggered(bool)), this, SLOT(onOpenTable()));
    connect(ui->insertToTable, SIGNAL(triggered(bool)), this, SLOT(onInsertToTable()));
    connect(ui->deleteRow, SIGNAL(triggered(bool)), this, SLOT(onDeleteFromTable()));
    connect(ui->searchInTable, SIGNAL(triggered(bool)), this, SLOT(onSearchInTable()));
    connect(ui->modifyInTable, SIGNAL(triggered(bool)), this, SLOT(onModifyInTable()));
    connect(ui->speedcheck_insert, SIGNAL(triggered(bool)), this, SLOT(onSpeedCheckInsert()));
    connect(ui->speedcheck_image, SIGNAL(triggered(bool)), this, SLOT(onSpeedCheckImage()));
    connect(ui->open_image_from_table, SIGNAL(triggered(bool)), this, SLOT(onOpenImageDemo()));
}

void MainWindow::message(const QString& text)
{
    QMessageBox::information(this, "info", text);
}

void MainWindow::setTableView(std::vector<std::wstring>& headers, std::vector<TableItem>& contents)
{
    m_bChange = false;
    QStandardItemModel  *model;
    if(ui->tableView->model() != NULL) {
        model = qobject_cast<QStandardItemModel*>(ui->tableView->model());
        model->clear();
    }
    else {
       model = new QStandardItemModel();
       connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this,
               SLOT(dataChanged(QModelIndex,QModelIndex,QVector<int>)));
    }

    model->setColumnCount(headers.size());
    for (int i=0;i<headers.size();i++) {
        model->setHeaderData(i,Qt::Horizontal,QString::fromStdWString(headers[i]));
    }

    for (int i=0;i<contents.size();i++) {
        for (int j=0;j<contents[i]._str.size();j++) {
            model->setItem(i, j, new QStandardItem(QString::fromStdWString(contents[i]._str[j])));
        }

        if(!m_server._tbName.empty() && contents[i]._str.size() > 0) {
            QStandardItem *item = model->item(i, 0);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }

    ui->tableView->setModel(model);
    m_bChange = true;
}

void MainWindow::showDatabases()
{
    if(!checkLink()) {
        return;
    }

    std::vector<std::wstring> dbNames = m_pDemo->getAllDatabasesNames();
    std::vector<std::wstring> headers = {L"databases"};
    std::vector<TableItem> rows;
    for (int i=0;i<dbNames.size();i++) {
        TableItem temp;
        temp._str.push_back(dbNames[i]);
        rows.push_back(temp);
    }

    ui->btnNextPage->setVisible(false);
    ui->btnPrevPage->setVisible(false);
    ui->labPages->setVisible(false);

    setTableView(headers, rows);
}

void MainWindow::showTables(std::wstring &dbName)
{
    if(!checkLink()) {
        return;
    }

    ui->btnNextPage->setVisible(false);
    ui->btnPrevPage->setVisible(false);
    ui->labPages->setVisible(false);

    std::vector<std::wstring> tbNames = m_pDemo->getTableNames(dbName);
    std::vector<std::wstring> headers = {L"tables"};
    std::vector<TableItem> rows;
    for (int i=0;i<tbNames.size();i++) {
        TableItem temp;
        temp._str.push_back(tbNames[i]);
        rows.push_back(temp);
    }
    setTableView(headers, rows);
}

void MainWindow::showTable(const std::wstring &dbName, const std::wstring &tbName, const int perPageNum, const int page)
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    ui->btnNextPage->setVisible(true);
    ui->btnPrevPage->setVisible(true);
    ui->labPages->setVisible(true);

    Condition cond;
    TableItems tbItems;
    m_pDemo->searchInTable(dbName, tbName, cond, tbItems, perPageNum, page);
    m_tolPages = tbItems._tolItems / m_perPageNum;
    ui->labPages->setText(QString::asprintf("%1/%2").arg(m_curPage+1).arg(m_tolPages));
    setTableView(tbItems._headers, tbItems._items);
}

void MainWindow::updatePath()
{
    QString text = "/";
    if(!m_server._dbName.empty()) {
        text += QString::fromStdWString(m_server._dbName);
        if(!m_server._tbName.empty()) {
            text += QString::fromStdWString(L"/" + m_server._tbName);
        }
    }

    ui->ed_path->setText(text);
}

void MainWindow::onBackPath()
{
    if(!m_server._dbName.empty()) {
        if(!m_server._tbName.empty()) {
            m_server._tbName.clear();
            showTables(m_server._dbName);
        }
        else {
            m_server._dbName.clear();
            showDatabases();
        }
    }

    updatePath();
}

void MainWindow::onPrevPage()
{
    if(m_curPage > 0) {
        m_curPage--;
    }

    showTable(m_server._dbName, m_server._tbName, m_perPageNum, m_curPage);
}

void MainWindow::onNextPage()
{
    if(m_curPage < m_tolPages - 1) {
        m_curPage++;
    }

    showTable(m_server._dbName, m_server._tbName, m_perPageNum, m_curPage);
}

void MainWindow::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if(!m_server._dbName.empty() && !m_server._tbName.empty() && m_bChange) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->tableView->model());

        Condition cond;
        cond._colName = model->horizontalHeaderItem(0)->text().toStdWString();
        cond._ctLow = CT_E;
        cond._low = model->item(topLeft.row())->text().toLong();
        cond._type = DT_INT;

        std::wstring strCol = model->horizontalHeaderItem(topLeft.column())->text().toStdWString();
        std::wstring strValue = model->item(topLeft.row(), topLeft.column())->text().toStdWString();
        m_pDemo->modifyInTable(m_server._dbName, m_server._tbName, cond, strCol, strValue);
//        m_curPage = 0;
//        showTable(m_server._dbName, m_server._tbName);
    }
}

void MainWindow::onLinkServer()
{
    m_server._strServerUrl = L"mysqlx://hj:644152350@localhost:33060"; // This url could be got by login window or something like it
    m_server._dbName.clear();
    m_server._tbName.clear();

    if(!m_pDemo->initClient(m_server._strServerUrl)) {
        m_pDemo = NULL;
        message("Unable to link with server! Please check network!");
    }
    else {
//        message("Success to link with server!");
        updatePath();
        showDatabases();
    }
}

void MainWindow::onCreateDatabase()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"demo";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(ok && !dbName.empty()) {
        if(m_pDemo->checkExist(dbName)) {
            message(QString::fromStdWString(L"Database " + dbName + L" already exists!"));
        }
        else {
            if(m_pDemo->createDatabase(dbName)) {
        //        message(QString::fromStdWString(L"Database " + dbName + L" has been created!"));
                m_server._dbName.clear();
                m_server._tbName.clear();
                updatePath();
                showDatabases();
            }
            else {
                message(QString::fromStdWString(L"Unable to create database " + dbName + L", maybe it has been created or server is outline!"));
            }
        }
    }

}

void MainWindow::onDeleteDatabase()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"demo";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();

    if(ok && !dbName.empty()) {
        if(!m_pDemo->checkExist(dbName)) {
            message(QString::fromStdWString(L"Database " + dbName + L" doesn't exist!"));
        }
        else{
            if(m_pDemo->deleteDatabase(dbName)) {
        //        message(QString::fromStdWString(L"Database " + dbName + L" has been deleted!"));
                m_server._dbName.clear();
                m_server._tbName.clear();
                updatePath();
                showDatabases();
            }
            else {
                message(QString::fromStdWString(L"Unable to delete database " + dbName + L", maybe it doesn't exist or server is outline!"));
            }
        }
    }
}

void MainWindow::onOpenDatabase()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();

    if(ok && !dbName.empty()) {
        if(m_pDemo->checkExist(dbName)) {
            m_server._dbName = dbName;
            m_server._tbName.clear();
            updatePath();
            showTables(dbName);
        }
        else {
            message(QString::fromStdWString(L"Database " + dbName + L" doesn't exist!"));
        }
    }
}

void MainWindow::onCreateTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"students";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    if(m_pDemo->checkExist(dbName, tbName)) {
        message("Table already exists!");
        return;
    }

    std::vector<TableContent> tbContents;

    TableContent temp;
    temp._name = L"id";
    temp._type = DT_INT;
    temp._bKey = true;
    temp._bNull = false;
    temp._bAutoInc = false;

    tbContents.push_back(temp);

    TableContent temp2;
    temp2._name = L"name";
    temp2._type = DT_CHAR;
    temp2._length = 10;
    tbContents.push_back(temp2);

    TableContent temp1;
    temp1._name = L"age";
    temp1._type = DT_INT;
    tbContents.push_back(temp1);

    bool ret = m_pDemo->createTable(dbName, tbName, tbContents);
    if(ret) {
//        message(QString::fromStdWString(L"Table " + tbName + L" has been created!"));
        m_server._dbName = dbName;
        m_server._tbName.clear();
        updatePath();
        showTables(dbName);
    }
    else {
        message(QString::fromStdWString(L"Unable to create table " + dbName + L", maybe it already exists in database or server is outline!"));
    }
}

void MainWindow::onDeleteTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"students";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    bool ret = m_pDemo->deleteTable(dbName, tbName);
    if(ret) {
//        message(QString::fromStdWString(L"Table " + tbName + L" has been deleted!"));
        m_server._dbName = dbName;
        m_server._tbName.clear();
        updatePath();
        showTables(dbName);
    }
    else {
        message(QString::fromStdWString(L"Unable to delete table " + dbName + L", maybe it doesn't exist in database or server is outline!"));
    }
}

void MainWindow::onOpenTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"employee";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    m_server._dbName = dbName;
    m_server._tbName = tbName;
    updatePath();
    m_curPage = 0;
    showTable(dbName, tbName, m_perPageNum, m_curPage);
}

void MainWindow::onInsertToTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"employee";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    TableItems tbItems;
    tbItems._headers = {
        L"id", L"name", L"age"
    };

    tbItems._headerTypes = {
        L"int", L"string", L"int"
    };

    TableItem tbItem;
    tbItem._str = {
        L"111", L"flick", L"18"
    };
    tbItems._items.push_back(tbItem);

    TableItem tbItem1;
    tbItem1._str = {
        L"12", L"jack", L"19"
    };
    tbItems._items.push_back(tbItem1);

    bool ret = m_pDemo->insertToTable(dbName, tbName, tbItems);
    if(ret) {
//        message(QString::fromStdWString(L"Data has been inserted!"));
        m_server._dbName = dbName;
        m_server._tbName = tbName;
        updatePath();
        m_curPage = 0;
        showTable(dbName, tbName, m_perPageNum, m_curPage);
    }
    else {
        message(QString::fromStdWString(L"Unable to insert data, maybe it already exists in table or server is outline!"));
    }
}

void MainWindow::onDeleteFromTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"employee";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    Condition cond;
    cond._colName = L"age";
    cond._ctHigh = CT_L;
    cond._high = 20;
    cond._type = DT_INT;

    bool ret = m_pDemo->deleteFromTable(dbName, tbName, cond);
    if(ret) {
//        message(QString::fromStdWString(L"Data has been inserted!"));
        m_server._dbName = dbName;
        m_server._tbName = tbName;
        updatePath();
        m_curPage = 0;
        showTable(dbName, tbName, m_perPageNum, m_curPage);
    }
    else {
        message(QString::fromStdWString(L"Unable to insert data, maybe it already exists in table or server is outline!"));
    }
}

void MainWindow::onSearchInTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"employee";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    Condition cond;
    cond._colName = L"age";
    cond._ctLow = CT_GE;
    cond._low = 20;
    cond._type = DT_INT;
    cond._order = L"id desc";
    cond._headers.push_back(L"id");
    cond._headers.push_back(L"age");

    TableItems tbItems;
    m_pDemo->searchInTable(dbName, tbName, cond, tbItems, 2, 1);
    setTableView(tbItems._headers, tbItems._items);

    m_server._dbName = dbName;
    m_server._tbName = tbName;
    m_curPage = 0;
    m_tolPages = tbItems._tolItems / m_perPageNum;
    updatePath();
}

void MainWindow::onModifyInTable()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"employee";
    bool ok;
    dbName = QInputDialog::getText(this, tr("Input the name of database"), tr("name of database"),
                          QLineEdit::Normal, QString::fromStdWString(dbName), &ok).toStdWString();
    if(!ok || dbName.empty()) {
        return;
    }
    tbName = QInputDialog::getText(this, tr("Input the name of table"), tr("name of table"),
                          QLineEdit::Normal, QString::fromStdWString(tbName), &ok).toStdWString();
    if(!ok || tbName.empty()) {
        return;
    }

    Condition cond;
    cond._colName = L"id";
    cond._ctLow = CT_E;
    cond._low = 111;
    cond._type = DT_INT;

    std::wstring strCol = L"age";
    std::wstring strValue = L"12";
    m_pDemo->modifyInTable(dbName, tbName, cond, strCol, strValue);
    showTable(dbName, tbName);

    m_server._dbName = dbName;
    m_server._tbName = tbName;
    updatePath();
}

void MainWindow::onOpenImageDemo()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"tiffs";

    Condition cond;
    cond._colName = L"id";
    cond._ctLow = CT_E;
    cond._low = 0;
    cond._type = DT_INT;

    TableItems tbItems;
    m_pDemo->searchInTable(dbName, tbName, cond, tbItems, 1, 0);

    saveBytes2Image(L"/home/hj/images/test.tiff", tbItems._items[0]._data[0]);
}

void MainWindow::onSpeedCheckInsert()
{
    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"students";

    if(m_pDemo->checkExist(dbName, tbName)) {
        m_pDemo->deleteTable(dbName, tbName);
    }

    {
        std::vector<TableContent> tbContents;
        TableContent temp;
        temp._name = L"id";
        temp._type = DT_INT;
        temp._bKey = true;
    //        temp._bNull = true;
    //        temp._bAutoInc = false;
        tbContents.push_back(temp);
        TableContent temp2;
        temp2._name = L"name";
        temp2._type = DT_CHAR;
        temp2._length = 10;
        tbContents.push_back(temp2);
        TableContent temp1;
        temp1._name = L"age";
        temp1._type = DT_INT;
        tbContents.push_back(temp1);
        TableContent temp3;
        temp3._name = L"class";
        temp3._type = DT_INT;
        tbContents.push_back(temp3);
        TableContent temp4;
        temp4._name = L"height";
        temp4._type = DT_INT;
        tbContents.push_back(temp4);

        m_pDemo->createTable(dbName, tbName, tbContents);
    }

    int number = 1000000;
    std::wstring name = L"test";
    std::wstring age = L"19";
    std::wstring cls = L"7";
    std::wstring height = L"150";
    timeval start, end;

//    gettimeofday(&start, NULL);
//    TableItems tbItems;
//    tbItems._headers = {
//        L"id", L"name", L"age", L"class"
//    };

//    for (int i=1;i<=number;i++) {
//        std::vector<std::wstring> temp = {
//            ctype2string(i), name + ctype2string(i),
//            age + ctype2string(i), cls + ctype2string(i)
//        };
//        tbItems._items.push_back(temp);
//    }

//    gettimeofday(&end, NULL);
//    double buildTime = 1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000;
//    qDebug() << "Building " << number << " records elapsed "
//             << buildTime << "ms.";

    gettimeofday(&start, NULL);

    std::wstring file = L"/var/lib/mysql-files/test.txt";
    m_pDemo->loadDataFromFile(dbName, tbName, file);

//    m_pDemo->insertToTable(dbName, tbName, tbItems);

    gettimeofday(&end, NULL);
    double libTime = 1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000;
    qDebug() << "Speed check for inserting " << number << " records to database elapsed "
             << libTime << "ms.";

    gettimeofday(&start, NULL);
    std::string txt = "test.txt";
    std::wofstream of;
    of.open(txt);
    of << L"id name age class height\r\n";
    for (int i=0;i<number;i++) {
        std::wstring stri = ctype2string(i);
        of << stri << L" " << name + stri << L" "
           << age + stri << L" " << cls + stri
           << L" " << height + stri << L"\r\n";
    }
    of.flush();
    of.close();
    gettimeofday(&end, NULL);
    double txtTime = 1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000;
    qDebug() << "Speed check for writing " << number << " records to txt elapsed " <<
                txtTime << "ms.";

    qDebug() << "Speed ratio for lib/txt: " << libTime/txtTime;
}

void MainWindow::onSpeedCheckImage()
{

    if(!checkLink()) {
        message("Please link to server at first!");
        return;
    }

    std::wstring dbName = L"test";
    std::wstring tbName = L"tiffs";

    if(m_pDemo->checkExist(dbName, tbName)) {
        m_pDemo->deleteTable(dbName, tbName);
    }

    {
        std::vector<TableContent> tbContents;
        TableContent temp;
        temp._name = L"id";
        temp._type = DT_INT;
        temp._bKey = false;
        tbContents.push_back(temp);

        TableContent temp1;
        temp1._name = L"image";
        temp1._type = DT_BLOB;
        tbContents.push_back(temp1);

        m_pDemo->createTable(dbName, tbName, tbContents);
    }

    timeval start, end;

    gettimeofday(&start, NULL);
    TableItems tbItems;
    tbItems._headers = {
        L"id", L"image"
    };
    tbItems._headerTypes = {
        L"int", L"longblob"
    };

    std::wstring fileDir = L"/var/lib/mysql-files/mysql_examples/";
    std::vector<std::wstring> files;
    findFilesFromDir(fileDir, files);
    int number = files.size();

    for (int i=0;i<number;++i) {
        TableItem temp;
        temp._str = {
            ctype2string(i), fileDir + files[i]
        };
        tbItems._items.push_back(temp);
    }

    gettimeofday(&end, NULL);
    double buildTime = 1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000;
    qDebug() << "Building " << number << " records elapsed "
             << buildTime << "ms.";

    gettimeofday(&start, NULL);

    m_pDemo->insertToTable(dbName, tbName, tbItems);

    gettimeofday(&end, NULL);
    double libTime = 1000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000;
    qDebug() << "Speed check for inserting " << number << " records to database elapsed "
             << libTime << "ms.";

    double cpTime = 300;
    qDebug() << "Speed check for coping " << number << " records will elapse " <<
                cpTime << "ms.";

    qDebug() << "Speed ratio for lib/cp: " << libTime/cpTime;
}






