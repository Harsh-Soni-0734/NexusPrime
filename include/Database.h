#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <variant>
#include "BPlusTree.h"
#include <iomanip> // for std::setw
#include <numeric> // for std::accumulate
#include <iostream>
#include <unordered_map>
#include <sstream>

// ------------------- Database Components -------------------
using Value = std::variant<int, float, std::string>;

struct Condition
{
    std::string column;
    std::string op;
    Value value;
    std::string logical_op = "AND";
    // Add JOIN-specific fields
    bool is_join = false;
    std::string left_table;
    std::string left_col;
    std::string right_table;
    std::string right_col;
};

struct Column
{
    std::string name;
    std::string type;
    bool indexed;
};

struct Table
{
    std::string name;
    std::vector<Column> columns;
    std::vector<std::vector<Value>> rows;
    BPlusTree index;
};

std::ostream &operator<<(std::ostream &os, const Value &val);

class Database
{
private:
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;

    int get_col_index(const std::string &table_name, const std::string &col_name);

    Value parse_value(const std::string &str, const std::string &type);

    void determine_range(const Condition &cond, int &min_key, int &max_key);

    bool evaluate_condition(const std::vector<Value> &row,
                            const Condition &cond, const Table &table);

    std::vector<int> find_matching_rows(Table &table,
                                        const std::vector<Condition> &conditions);
public:
    // Add this static trim function
    static std::string trim(const std::string &s);
    Table *get_table(const std::string &name);
    int public_get_col_index(const std::string &table_name, const std::string &col_name);
    Value public_parse_value(const std::string &str, const std::string &type);

    void create_table(const std::string &name, const std::vector<Column> &columns);

    void select_join(const std::string &table1_name,
                     const std::string &table2_name,
                     const std::vector<Condition> &join_conditions,
                     const std::vector<Condition> &where_conditions,
                     const std::vector<std::string> &selected_columns,
                     bool select_all);
    
    void update(const std::string &table_name,
                const std::vector<std::pair<std::string, Value>> &updates,
                const std::vector<Condition> &conditions);

    void delete_rows(const std::string &table_name,
                     const std::vector<Condition> &conditions);
    

    void insert_into(const std::string &table_name, const std::vector<Value> &values);
   

    void select(const std::string &table_name,
                const std::vector<Condition> &conditions,
                const std::vector<std::string> &selected_columns,
                bool select_all);
   
};

#endif // DATABASE_H