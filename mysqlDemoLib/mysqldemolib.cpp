#include "mysqldemolib.h"
#include <iostream>
#include <sys/time.h>

Bytes copyBytes(const bytes& bts) {
    Bytes bytes;
    char* pt = new char[bts.size()];
    memcpy(pt, bts.begin(), bts.size());
    bytes._data = pt;
    bytes._length = bts.size();
    return bytes;
}

std::wstring MysqlDemoLib::condToString(const Condition &cond)
{
    std::wstring strCond;

    switch(cond._type) {
       case DataType::DT_INT:
        switch (cond._ctLow) {
        case Condtype::CT_G:
            strCond += cond._colName + L" > " + ctype2string(cond._low);
            break;
        case Condtype::CT_GE:
            strCond += cond._colName + L" >= " + ctype2string(cond._low);
            break;
        case Condtype::CT_E:
            strCond += cond._colName + L" = " + ctype2string(cond._low);
            break;
        default:
            break;
        }

        switch (cond._ctHigh) {
        case Condtype::CT_L:
            if(cond._ctLow != Condtype::CT_NONE)
                strCond += L" AND ";
            strCond += cond._colName + L" < " + ctype2string(cond._high);
            break;
        case Condtype::CT_LE:
            if(cond._ctLow != Condtype::CT_NONE)
                strCond += L" AND ";
            strCond += cond._colName + L" <= " + ctype2string(cond._high);
            break;
        default:
            break;
        }

        break;
       case DataType::DT_CHAR:
        strCond += cond._colName + L" like " + cond._pattern;
        break;
       default:
        break;
    }

    return strCond;
}

MysqlDemoLib::MysqlDemoLib()
{
    m_pClient = NULL;
    m_mMappingDataType[DT_INT] = L"int";
    m_mMappingDataType[DT_DOUBLE] = L"double";
    m_mMappingDataType[DT_CHAR] = L"char";
    m_mMappingDataType[DT_BLOB] = L"longblob";
}

MysqlDemoLib::~MysqlDemoLib()
{
    closeClient();
}

bool MysqlDemoLib::initClient(const std::wstring& url) {
    if(m_pClient != NULL) {
        closeClient();
    }

    m_pClient = new Client(url, ClientOption::POOL_MAX_SIZE, 5);

    if(!checkLink()) {
        return false;
    }

    return true;
}

bool MysqlDemoLib::closeClient()
{
    if(!checkLink()) {
        return false;
    }

    m_pClient->close();
    delete m_pClient;
    m_pClient = NULL;

    return true;
}

bool MysqlDemoLib::checkExist(const std::wstring &dbName, const std::wstring &tbName)
{
    if(!checkLink() || !checkExist(dbName)) {
        return false;
    }

    Session session = m_pClient->getSession();
    std::vector<std::wstring> tbList = session.getSchema(dbName).getTableNames();
    foreach (std::wstring tb, tbList) {
        if(tb.compare(tbName) == 0) {
            return true;
        }
    }

    session.close();
    return false;
}

bool MysqlDemoLib::checkExist(const std::wstring& dbName)
{
    if(!checkLink()) {
        return false;
    }

    Session session = m_pClient->getSession();
    std::list<Schema> schemaList = session.getSchemas();
    foreach (Schema schema, schemaList) {
        if(schema.getName().compare(fromStdString(dbName)) == 0) {
            return true;
        }
    }

    session.close();
    return false;
}

bool MysqlDemoLib::createDatabase(const std::wstring &name)
{
    if(!checkLink()) {
        return false;
    }

    if(!checkExist(name)) {
        Session session = m_pClient->getSession();
        session.createSchema(name);
        session.close();
        return true;
    }
    else {
        return false;
    }
}

bool MysqlDemoLib::deleteDatabase(const std::wstring &name)
{
    if(!checkLink()) {
        return false;
    }
    if(checkExist(name)) {
        Session session = m_pClient->getSession();
        session.dropSchema(name);
        session.close();
        return true;
    }
    else {
        return false;
    }
}

std::vector<std::wstring> MysqlDemoLib::getAllDatabasesNames()
{
    std::vector<std::wstring> dbNames;
    if(!checkLink()) {
        return dbNames;
    }

    Session session = m_pClient->getSession();
    std::list<Schema> schemaList = session.getSchemas();
    foreach (Schema schema, schemaList) {
        dbNames.push_back(toStdString(schema.getName()));
    }

    session.close();

    return dbNames;
}

std::vector<std::wstring> MysqlDemoLib::getTableNames(const std::wstring &dbName)
{
    std::vector<std::wstring> tbNames;
    if(!checkLink()) {
        return tbNames;
    }

    Session session = m_pClient->getSession();
    Schema db = session.getSchema(dbName);
    tbNames = db.getTableNames();

    session.close();
    return tbNames;
}

std::vector<std::wstring> MysqlDemoLib::getTableColumns(const std::wstring &dbName, const std::wstring &tbName)
{
    std::vector<std::wstring> columnNames;
    if(!checkLink()) {
        return columnNames;
    }

    Session session = m_pClient->getSession();
    string quoted_name = fromStdString(dbName) + L"." + fromStdString(tbName);

    string sql = L"Select * from " + quoted_name + L" limit 0,0";
    SqlResult result = session.sql(sql).execute();
    result.getColumns();
    for (size_t i=0;i<result.getColumnCount();i++) {
        columnNames.push_back(result.getColumn(i).getColumnName());
    }
    session.close();
    return columnNames;
}

bool MysqlDemoLib::resultToItems(RowResult &result, TableItems & tbItems)
{
    for (size_t i=0;i<result.getColumnCount();i++) {
        tbItems._headers.push_back(result.getColumn(i).getColumnName());
    }

    std::vector<Row> rows = result.fetchAll();
    for (size_t i=0;i<rows.size();i++) {
        TableItem temp;
        int index = -1;
        for(size_t j=0;j<result.getColumnCount();j++) {
            if(rows[i][j].getType() == Value::Type::RAW) {
                temp._data.push_back(copyBytes(bytes(rows[i][j])));
                ++index;
                temp._str.push_back(ctype2string(index));
            }
            else {
                temp._str.push_back(valueToString(rows[i][j]));
            }
        }
        tbItems._items.push_back(temp);
    }

    return true;
}

bool MysqlDemoLib::createTable(const std::wstring &dbName, const std::wstring &tbName, std::vector<TableContent>& tbContents)
{
    if(!checkLink()) {
        return false;
    }

    Session session = m_pClient->getSession();
    Schema db = session.getSchema(dbName);
    std::vector<std::wstring> tbNames = db.getTableNames();
    foreach (std::wstring name, tbNames) {
        if(name.compare(tbName) == 0) {
            return false;
        }
    }

    string quoted_name = fromStdString(dbName) + L"." + fromStdString(tbName);
    string create = "CREATE TABLE ";
    create += quoted_name;
    create += L" (";

    foreach (TableContent content, tbContents) {
        create += content._name + L" " + m_mMappingDataType[content._type];

        if(content._type == DT_CHAR) {
            create += L"(" + std::to_wstring(content._length) + L")";
        }

        if(!content._bNull) {
            create += L" not null";
        }

        if(content._bKey) {
            create += L" primary key";
        }

        if(content._bAutoInc) {
            create += L" AUTO_INCREMENT";
        }

        create += L",";
    }
    create.pop_back();

//    create += L")";
    create += L") ENGINE = MYISAM";

//    std::cout << create << std::endl;

    session.sql(create).execute();
    session.close();
    return true;
}

bool MysqlDemoLib::deleteTable(const std::wstring &dbName, const std::wstring &tbName)
{
    if(!checkLink()) {
        return false;
    }

    Session sess = m_pClient->getSession();
    string quoted_name = fromStdString(dbName) + L"." + fromStdString(tbName);
    sess.sql(L"Drop table if exists " + quoted_name).execute();

    sess.close();
    return true;
}

bool MysqlDemoLib::insertToTable(const std::wstring &dbName, const std::wstring &tbName, TableItems &tbItems)
{
    if(!checkLink()) {
        return false;
    }

//    timeval start, end;
    Session sess = m_pClient->getSession();
    size_t piece;
    size_t times = 10;
    if(tbItems._items.size() < times) {
        piece = 1;
    }
    else {
        piece = tbItems._items.size()/times;
    }

    size_t maxk = tbItems._items.size()/piece;
    if(tbItems._items.size() % piece !=0){
        maxk += 1;
    }

    try {
        std::wostringstream osh;
        string quoted_name = fromStdString(dbName) + L"." + fromStdString(tbName);
        osh << L"insert into " << quoted_name << L"(";
        for (size_t i=0;i<tbItems._headers.size();i++) {
            osh << tbItems._headers[i];
            if(i != tbItems._headers.size() - 1) {
                osh << L",";
            }
        }
        osh << L") values";
        std::wstring head = osh.str();

        sess.sql("set autocommit=0").execute();
        sess.startTransaction();
        for(size_t k=0; k<maxk; k++)
        {
            std::wostringstream os;
            os << head;

            for (size_t i=k*piece;i<(k+1)*piece && i<tbItems._items.size();i++) {
                os << L"(";
                for (size_t j=0;j<tbItems._headers.size();j++) {
                    if(tbItems._headerTypes[j] == L"longblob") {
                        os << L"LOAD_FILE(\"" << tbItems._items[i]._str[j] << L"\")";
                    }
                    else {
                        os << L"\"" << tbItems._items[i]._str[j] << L"\"";
                    }

                    if(j != tbItems._headers.size() - 1) {
                        os << L",";
                    }
                }
                os << L")";
                if(i != (k+1)*piece -1 && i != tbItems._items.size() - 1) {
                    os << L",";
                }
            }
//            std::cout << os.str() << std::endl;
            sess.sql(os.str()).execute();
        }

        sess.commit();
        sess.close();
        return true;
    }
    catch (const Error &err) {
        sess.rollback();
        std::cout << "Following error occurred: " << err << std::endl;
        return false;
    }

}

bool MysqlDemoLib::deleteFromTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond)
{
    if(!checkLink()) {
        return false;
    }

    Session sess = m_pClient->getSession();
    Schema db = sess.getSchema(dbName);
    Table tb = db.getTable(tbName);

    std::wstring strCond = condToString(cond);

//    std::cout << strCond << std::endl;
    tb.remove().where(strCond).execute();

    sess.close();

    return true;
}

bool MysqlDemoLib::searchInTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond,
                                 TableItems& tbItems, const int perPageNum, const int page)
{
    if(!checkLink()) {
        return false;
    }

    Session sess = m_pClient->getSession();

    try {
        Schema db = sess.getSchema(dbName);
        Table tb = db.getTable(tbName);
        std::wstring strCond = condToString(cond);
    //    std::cout << strCond << std::endl;
        std::vector<std::wstring> headers = cond._headers.size()? cond._headers : getTableColumns(dbName, tbName);

        TableSelect tbs = tb.select(headers);
        if(!strCond.empty()) {
            tbs.where(strCond);
        }

        if(!cond._order.empty()) {
            tbs.orderBy(cond._order);
        }

        TableSelect tbsall = tbs;

        tbs.offset(perPageNum*page);
        tbs.limit(perPageNum);
        RowResult result = tbs.execute();
        resultToItems(result, tbItems);

        int maxPage = 10;
        tbsall.limit(maxPage * perPageNum);
        result = tbsall.execute();
        tbItems._tolItems = result.count();
    }
    catch (const Error &err) {
        std::cout << "Following error occurred: " << err << std::endl;
    }

    sess.close();
    return true;
}

bool MysqlDemoLib::modifyInTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond, std::wstring &colName, std::wstring &value)
{
    if(!checkLink()) {
        return false;
    }

    try{
        Session sess = m_pClient->getSession();
        Schema db = sess.getSchema(dbName);
        Table tb = db.getTable(tbName);

        std::wstring strCond = condToString(cond);
        tb.update().set(colName, value).where(strCond).execute();

        sess.close();
        return true;
    }
    catch (const Error &err) {
        std::cout << "Following error occurred: " << err << std::endl;
        return false;
    }
}

bool MysqlDemoLib::loadDataFromFile(const std::wstring &dbName, const std::wstring &tbName, std::wstring &fileName)
{
    try{
        Session sess = m_pClient->getSession();
        string quoted_name = fromStdString(dbName) + L"." + fromStdString(tbName);

        std::wstring sql = L"ALTER TABLE " + quoted_name + L" DISABLE KEYS";
        sess.sql(sql).execute();

        sql = L"load data infile \"" + fileName + L"\" into table " + quoted_name;
        sql += L" fields terminated by \" \"";
        sess.sql(sql).execute();

        sql = L"ALTER TABLE " + quoted_name + L" ENABLE KEYS";
        sess.sql(sql).execute();

        sess.close();
        return true;
    }
    catch (const Error &err) {
        std::cout << "Following error occurred: " << err << std::endl;
        return false;
    }
}
