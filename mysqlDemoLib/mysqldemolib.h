#ifndef MYSQLDEMOLIB_H
#define MYSQLDEMOLIB_H

#include "mysqldemolib_global.h"
#include <mysqlx/xdevapi.h>
#include <map>
#include <string>
using namespace mysqlx;

enum DataType {
    DT_INT=0,
    DT_DOUBLE,
    DT_CHAR,
    DT_BLOB,
    DT_NONE
};

struct TableContent {
    std::wstring    _name;
    DataType        _type;
    int             _length;
    bool            _bNull = true;
    bool            _bKey = false;
    bool            _bAutoInc = false;
};

enum Condtype {
    CT_L = 0, // less
    CT_LE,  // less equal
    CT_E,  // equal
    CT_G,   // greater
    CT_GE,  // greater equal
    CT_NONE
};

struct Condition {
    std::wstring _colName;
    DataType _type = DT_NONE;
    int _low;
    int _high;
    Condtype _ctLow = CT_NONE;
    Condtype _ctHigh = CT_NONE;
    std::wstring _pattern;
    std::wstring _order;
    std::vector<std::wstring> _headers;
};

struct Bytes {
    char * _data;
    int _length;
};

struct TableItem {
    std::vector<Bytes> _data;
    std::vector<std::wstring> _str;
};

struct TableItems {
    std::vector<std::wstring> _headers;
    std::vector<std::wstring> _headerTypes;
    std::vector<TableItem> _items;
    int _tolItems = -1;
};

template<class T>
std::wstring ctype2string(T value) {
    std::wostringstream os;
    os << value;
    std::wstring result = os.str();
    return result;
}


class MYSQLDEMOLIBSHARED_EXPORT MysqlDemoLib
{
private:
    Client* m_pClient;
    std::map<DataType, std::wstring> m_mMappingDataType;

private:
    std::wstring valueToString(Value& value) {
        std::wstring ret;

        switch (value.getType()) {
        case Value::Type::INT64:
            ret += ctype2string(long(value));
            break;
    //    case Value::Type::FLOAT:
    //        ret += value.get_float();
    //        break;
    //    case Value::Type::DOUBLE:
    //        ret += value.get_double();
    //        break;
        case Value::Type::STRING:
            ret += string(value);
            break;
        default:
            break;
        }

        return ret;
    }
    mysqlx::string fromStdString(const std::wstring &str) {return mysqlx::string(str.c_str());}
    const std::wstring toStdString(const mysqlx::string &str) {return std::wstring(str.data());}
    std::wstring condToString(const Condition & cond);

    bool resultToItems(RowResult &result, TableItems & tbItems);

public:
    MysqlDemoLib();
    ~MysqlDemoLib();

    bool checkLink() {return m_pClient != NULL;}
    bool initClient(const std::wstring &url);
    bool closeClient();

    bool checkExist(const std::wstring& dbName);
    bool checkExist(const std::wstring& dbName, const std::wstring& tbName);

    bool createDatabase(const std::wstring &name);
    bool deleteDatabase(const std::wstring &name);
    std::vector<std::wstring> getAllDatabasesNames();
    std::vector<std::wstring> getTableNames(const std::wstring &dbName);
    std::vector<std::wstring> getTableColumns(const std::wstring &dbName, const std::wstring &tbName);

    bool createTable(const std::wstring &dbName, const std::wstring &tbName, std::vector<TableContent>& tbContents);
    bool deleteTable(const std::wstring &dbName, const std::wstring &tbName);
    bool insertToTable(const std::wstring &dbName, const std::wstring &tbName, TableItems &tbItems);
    bool deleteFromTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond);
    bool searchInTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond,
                       TableItems& tbItems ,const int perPageNum = 5, const int page = 0);
    bool modifyInTable(const std::wstring &dbName, const std::wstring &tbName, Condition &cond, std::wstring &colName, std::wstring &value);
    bool loadDataFromFile(const std::wstring &dbName, const std::wstring &tbName, std::wstring &fileName);
};

#endif // MYSQLDEMOLIB_H
