#include "SQLParser.h"

void SQLParser::parse(const std::string &query, Database &db)
{
    try
    {
        std::stringstream ss(query);
        std::string token;
        ss >> token;
        std::transform(token.begin(), token.end(), token.begin(), ::toupper);

        if (token == "CREATE")
            parse_create(ss, db);
        else if (token == "INSERT")
            parse_insert(ss, db);
        else if (token == "SELECT")
            parse_select(ss, db);
        else if (token == "UPDATE")
            parse_update(ss, db);
        else if (token == "DELETE")
            parse_delete(ss, db);
        else
            throw std::runtime_error("Unknown command");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void SQLParser::parse_create(std::stringstream &ss, Database &db)
{
    std::string table_keyword, table_name;
    ss >> table_keyword >> table_name;

    std::vector<Column> columns;
    std::string full_spec;
    std::getline(ss, full_spec);

    // Extract content between parentheses
    size_t start = full_spec.find('(');
    size_t end = full_spec.rfind(')');
    if (start == std::string::npos || end == std::string::npos || start >= end)
    {
        throw std::runtime_error("Invalid CREATE TABLE syntax");
    }
    full_spec = full_spec.substr(start + 1, end - start - 1);

    // Split column definitions by comma and trim each
    std::vector<std::string> column_defs;
    std::stringstream col_ss(full_spec);
    std::string column_def;
    while (std::getline(col_ss, column_def, ','))
    {
        // Trim whitespace from each column definition
        auto trim = [](std::string &s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
                                            { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
                                 { return !std::isspace(ch); })
                        .base(),
                    s.end());
        };
        trim(column_def);
        if (!column_def.empty())
        {
            column_defs.push_back(column_def);
        }
    }

    for (const auto &column_def : column_defs)
    {
        std::stringstream def_ss(column_def);
        std::vector<std::string> tokens;
        std::string token;
        while (def_ss >> token)
        {
            tokens.push_back(token);
        }
        if (tokens.size() < 2)
        {
            continue; // Invalid column definition
        }

        Column col;
        col.name = tokens[0];
        col.type = tokens[1];
        col.indexed = false;

        // Check for PRIMARY KEY constraint
        for (size_t i = 2; i < tokens.size(); ++i)
        {
            if (i + 1 < tokens.size() && tokens[i] == "PRIMARY" && tokens[i + 1] == "KEY")
            {
                col.indexed = true;
                break;
            }
        }

        columns.push_back(col);
    }

    db.create_table(table_name, columns);
}

void SQLParser::parse_insert(std::stringstream &ss, Database &db)
{
    std::string into_keyword, table_name, values_keyword;
    ss >> into_keyword >> table_name >> values_keyword;

    Table *table = db.get_table(table_name);
    if (!table)
    {
        std::cerr << "Table not found\n";
        return;
    }

    std::vector<Value> parsed_values;
    std::string value_str;
    std::getline(ss, value_str);

    // Remove semicolons and trim
    value_str.erase(std::remove(value_str.begin(), value_str.end(), ';'), value_str.end());
    value_str = value_str.substr(value_str.find('(') + 1);
    value_str = value_str.substr(0, value_str.find(')'));

    std::stringstream values_ss(value_str);

    size_t col_index = 0;
    bool in_quotes = false;
    std::string current_value;
    char c;
    while (values_ss.get(c))
    {
        if (c == '\'')
        {
            if (in_quotes && values_ss.peek() == '\'')
            {
                current_value += '\'';
                values_ss.get(c);
            }
            else
            {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (!in_quotes && c == ',')
        {
            if (col_index >= table->columns.size())
            {
                throw std::runtime_error("Too many values");
            }

            const std::string &col_type = table->columns[col_index].type;

            if (col_type == "STRING")
            {
                current_value = Database::trim(current_value); // Fixed
                if (!current_value.empty() && current_value.front() == '\'' && current_value.back() == '\'')
                {
                    current_value = current_value.substr(1, current_value.size() - 2);
                }
                current_value = Database::trim(current_value); // Fixed
            }

            Value val = db.public_parse_value(current_value, col_type);
            parsed_values.push_back(val);
            current_value.clear();
            col_index++;
        }
        else
        {
            current_value += c;
        }
    }

    if (!current_value.empty())
    {
        if (col_index >= table->columns.size())
        {
            throw std::runtime_error("Too many values");
        }
        const std::string &col_type = table->columns[col_index].type;
        Value val = db.public_parse_value(current_value, col_type);
        parsed_values.push_back(val);
    }

    if (parsed_values.size() != table->columns.size())
    {
        throw std::runtime_error("Column count mismatch");
    }

    try
    {
        db.insert_into(table_name, parsed_values);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Insert error: " << e.what() << "\n";
    }
}

std::vector<Condition> SQLParser::parse_where_clause(std::stringstream &ss, Database &db, const std::string &table_name)
{
    std::vector<Condition> conditions;
    std::string logical_op = "AND"; // Default
    std::cerr << "Starting WHERE clause parsing\n";

    Table *table = db.get_table(table_name);
    if (!table)
    {
        std::cerr << "Table not found during WHERE parsing\n";
        return conditions;
    }

    while (true)
    {
        Condition cond;
        std::cerr << "Attempting to read condition...\n";

        // Read column name
        if (!(ss >> cond.column))
        {
            std::cerr << "Failed to read column name\n";
            break;
        }
        std::cerr << "Read column: " << cond.column << "\n";

        // Read operator
        if (!(ss >> cond.op))
        {
            std::cerr << "Failed to read operator\n";
            break;
        }
        std::cerr << "Read operator: " << cond.op << "\n";

        // Read value
        std::string value_str;
        bool in_quotes = false;
        char c;
        while (ss.get(c))
        {
            if (c == '\'')
            {
                in_quotes = !in_quotes;
                if (!in_quotes)
                    break; // Closing quote found
                continue;
            }
            if (!in_quotes && (c == 'A' || c == 'O'))
            { // Look for AND/OR
                ss.unget();
                std::string peek;
                ss >> peek;
                if (peek == "AND" || peek == "OR")
                {
                    ss.seekg(-static_cast<int>(peek.length()), std::ios::cur);
                    break;
                }
                value_str += c;
                continue;
            }
            value_str += c;
        }
        value_str = Database::trim(value_str);
        std::cerr << "Raw value string: '" << value_str << "'\n";

        // Parse value
        try
        {
            int col_idx = db.public_get_col_index(table_name, cond.column);
            if (col_idx == -1)
                throw std::runtime_error("Column not found");
            cond.value = db.public_parse_value(value_str, table->columns[col_idx].type);
            std::cerr << "Parsed value: " << cond.value << "\n";
        }
        catch (...)
        {
            std::cerr << "Failed to parse value for column " << cond.column << "\n";
            break;
        }

        // Store condition
        cond.logical_op = logical_op;
        conditions.push_back(cond);
        std::cerr << "Added condition: " << cond.column << " " << cond.op << " "
                  << value_str << " (" << cond.logical_op << ")\n";

        // Check for next logical operator
        std::streampos before_op = ss.tellg();
        std::string next_op;
        if (ss >> next_op)
        {
            std::cerr << "Next token: " << next_op << "\n";
            if (next_op == "AND")
            {
                logical_op = "AND";
                std::cerr << "Using AND for next condition\n";
            }
            else if (next_op == "OR")
            {
                logical_op = "OR";
                std::cerr << "Using OR for next condition\n";
            }
            else
            {
                std::cerr << "Not a logical operator, rewinding stream\n";
                ss.seekg(before_op);
                break;
            }
        }
        else
        {
            std::cerr << "End of conditions\n";
            break;
        }
    }
    return conditions;
}

void SQLParser::parse_select(std::stringstream &ss, Database &db)
{
    std::string fields_str;
    std::getline(ss, fields_str, 'F');
    ss.putback('F');

    std::vector<std::string> selected_columns;
    bool select_all = false;

    std::stringstream fields_ss(fields_str);
    std::string field;
    while (std::getline(fields_ss, field, ','))
    {
        field.erase(std::remove_if(field.begin(), field.end(), ::isspace), field.end());
        if (field == "*")
        {
            select_all = true;
            break;
        }
        selected_columns.push_back(field);
    }

    std::string from_keyword, table1_name, join_keyword, table2_name;
    ss >> from_keyword >> table1_name;

    // Remove semicolon from first table
    if (!table1_name.empty() && table1_name.back() == ';')
    {
        table1_name.pop_back();
    }

    // Check for JOIN
    if (ss >> join_keyword && (join_keyword == "JOIN" || join_keyword == "INNER"))
    {
        ss >> table2_name;

        // Remove semicolon from second table
        if (!table2_name.empty() && table2_name.back() == ';')
        {
            table2_name.pop_back();
        }

        std::string on_keyword;
        ss >> on_keyword; // Should be "ON"

        // Parse join condition
        Condition jc;
        jc.is_join = true;
        std::string left_part, op, right_part;
        ss >> left_part >> op >> right_part;

        // Clean right_part from semicolons
        right_part = Database::trim(right_part);
        if (!right_part.empty() && right_part.back() == ';')
        {
            right_part.pop_back();
        }

        // Parse left side
        size_t dot_pos = left_part.find('.');
        jc.left_table = left_part.substr(0, dot_pos);
        jc.left_col = left_part.substr(dot_pos + 1);

        // Parse right side
        dot_pos = right_part.find('.');
        jc.right_table = right_part.substr(0, dot_pos);
        jc.right_col = right_part.substr(dot_pos + 1);
        jc.op = op;

        std::vector<Condition> join_conds = {jc};

        // Parse WHERE clause if exists
        std::vector<Condition> where_conds;
        std::string where_kw;
        if (ss >> where_kw && where_kw == "WHERE")
        {
            where_conds = parse_where_clause(ss, db, table1_name);
        }

        db.select_join(table1_name, table2_name, join_conds,
                       where_conds, selected_columns, select_all);
    }
    else
    {
        // Existing single-table logic
        ss.seekg(-join_keyword.length(), std::ios::cur);
        std::vector<Condition> conditions;
        std::string where_clause;
        if (ss >> where_clause && where_clause == "WHERE")
        {
            conditions = parse_where_clause(ss, db, table1_name);
        }
        db.select(table1_name, conditions, selected_columns, select_all);
    }
}

void SQLParser::parse_update(std::stringstream &ss, Database &db)
{
    std::string table_name;
    ss >> table_name;

    // Remove semicolon from table name
    if (!table_name.empty() && table_name.back() == ';')
    {
        table_name.pop_back();
    }

    std::vector<std::pair<std::string, Value>> updates;
    std::string set_keyword;
    ss >> set_keyword;

    std::string set_clause;
    std::getline(ss, set_clause, 'W');
    ss.unget();
    std::stringstream set_ss(set_clause);

    std::string assignment;
    while (std::getline(set_ss, assignment, ','))
    {
        size_t eq_pos = assignment.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string col = assignment.substr(0, eq_pos);
            col.erase(std::remove_if(col.begin(), col.end(), ::isspace), col.end());
            std::string val_str = assignment.substr(eq_pos + 1);
            val_str.erase(std::remove_if(val_str.begin(), val_str.end(), ::isspace), val_str.end());

            Table *table = db.get_table(table_name);
            if (!table)
                continue;

            try
            {
                int col_idx = db.public_get_col_index(table_name, col);
                Value value = db.public_parse_value(val_str, table->columns[col_idx].type);
                updates.emplace_back(col, value);
            }
            catch (...)
            {
                std::cerr << "Invalid update value\n";
            }
        }
    }

    std::vector<Condition> conditions;
    std::string where_keyword;
    if (ss >> where_keyword && where_keyword == "WHERE")
    {
        conditions = parse_where_clause(ss, db, table_name);
    }

    try
    {
        db.update(table_name, updates, conditions);
    }
    catch (...)
    {
        std::cerr << "Update failed\n";
    }
}

void SQLParser::parse_delete(std::stringstream &ss, Database &db)
{
    std::string from_keyword, table_name;
    ss >> from_keyword >> table_name;

    // Remove semicolon from table name
    if (!table_name.empty() && table_name.back() == ';')
    {
        table_name.pop_back();
    }

    std::vector<Condition> conditions;
    std::string where_keyword;
    if (ss >> where_keyword && where_keyword == "WHERE")
    {
        conditions = parse_where_clause(ss, db, table_name);
    }

    try
    {
        db.delete_rows(table_name, conditions);
    }
    catch (...)
    {
        std::cerr << "Delete failed\n";
    }
}
