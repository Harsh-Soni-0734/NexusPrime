#include "Database.h"


// ------------------- SQL Parser -------------------
class SQLParser
{
public:
    void parse(const std::string &query, Database &db);

private:
    void parse_create(std::stringstream &ss, Database &db);

    void parse_insert(std::stringstream &ss, Database &db);

    std::vector<Condition> parse_where_clause(std::stringstream &ss, Database &db, const std::string &table_name);

    void parse_select(std::stringstream &ss, Database &db);

    void parse_update(std::stringstream &ss, Database &db);

    void parse_delete(std::stringstream &ss, Database &db);
};
