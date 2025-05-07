#include "Database.h"

int Database::get_col_index(const std::string &table_name, const std::string &col_name)
{
    auto &cols = tables[table_name]->columns;
    for (size_t i = 0; i < cols.size(); i++)
        if (cols[i].name == col_name)
            return i;
    return -1;
}

Value Database::parse_value(const std::string &str, const std::string &type)
{

    try
    {
        if (type == "INT")
            return std::stoi(str);
        if (type == "FLOAT")
            return std::stof(str);
        // Handle STRING type
        std::string trimmed = trim(str); // Use the Database::trim function
        // Remove surrounding quotes
        if (!trimmed.empty() && trimmed.front() == '\'' && trimmed.back() == '\'')
        {
            trimmed = trimmed.substr(1, trimmed.size() - 2);
            trimmed = trim(trimmed); // Trim again after removing quotes
        }
        return trimmed;
    }
    catch (...)
    {
        throw std::runtime_error("Invalid value for type " + type);
    }
}

void Database::determine_range(const Condition &cond, int &min_key, int &max_key)
{
    int value = 0;
    if (std::holds_alternative<int>(cond.value))
    {
        value = std::get<int>(cond.value);
    }
    else if (std::holds_alternative<float>(cond.value))
    {
        value = static_cast<int>(std::get<float>(cond.value));
    }
    else
    {
        try
        {
            value = std::stoi(std::get<std::string>(cond.value));
        }
        catch (...)
        {
            throw std::runtime_error("Invalid index value");
        }
    }

    if (cond.op == "=")
    {
        min_key = value;
        max_key = value;
    }
    else if (cond.op == ">")
    {
        min_key = value + 1;
        max_key = INT_MAX;
    }
    else if (cond.op == ">=")
    {
        min_key = value;
        max_key = INT_MAX;
    }
    else if (cond.op == "<")
    {
        min_key = INT_MIN;
        max_key = value - 1;
    }
    else if (cond.op == "<=")
    {
        min_key = INT_MIN;
        max_key = value;
    }
}

bool Database::evaluate_condition(const std::vector<Value> &row,
                        const Condition &cond, const Table &table)
{
    int col_idx = get_col_index(table.name, cond.column);
    if (col_idx == -1)
        return false;

    const Column &col = table.columns[col_idx];
    const Value &row_val = row[col_idx];
    const Value &cond_val = cond.value;

    try
    {
        if (col.type == "INT")
        {
            int row_num = std::get<int>(row_val);
            int cond_num = 0;

            if (std::holds_alternative<int>(cond_val))
            {
                cond_num = std::get<int>(cond_val);
            }
            else if (std::holds_alternative<float>(cond_val))
            {
                cond_num = static_cast<int>(std::get<float>(cond_val));
            }
            else
            {
                cond_num = std::stoi(std::get<std::string>(cond_val));
            }

            if (cond.op == "=")
                return row_num == cond_num;
            if (cond.op == "!=")
                return row_num != cond_num;
            if (cond.op == ">")
                return row_num > cond_num;
            if (cond.op == "<")
                return row_num < cond_num;
            if (cond.op == ">=")
                return row_num >= cond_num;
            if (cond.op == "<=")
                return row_num <= cond_num;
        }
        else if (col.type == "FLOAT")
        {
            float row_num = std::holds_alternative<int>(row_val)
                                ? static_cast<float>(std::get<int>(row_val))
                                : std::get<float>(row_val);
            float cond_num = 0;

            if (std::holds_alternative<int>(cond_val))
            {
                cond_num = static_cast<float>(std::get<int>(cond_val));
            }
            else if (std::holds_alternative<float>(cond_val))
            {
                cond_num = std::get<float>(cond_val);
            }
            else
            {
                cond_num = std::stof(std::get<std::string>(cond_val));
            }

            if (cond.op == "=")
                return std::abs(row_num - cond_num) < 1e-6;
            if (cond.op == "!=")
                return std::abs(row_num - cond_num) >= 1e-6;
            if (cond.op == ">")
                return row_num > cond_num;
            if (cond.op == "<")
                return row_num < cond_num;
            if (cond.op == ">=")
                return row_num >= cond_num;
            if (cond.op == "<=")
                return row_num <= cond_num;
        }
        else
        { // STRING

            std::string row_str = std::get<std::string>(row_val);
            std::string cond_str;

            // Extract condition value as string
            if (std::holds_alternative<std::string>(cond_val))
            {
                cond_str = std::get<std::string>(cond_val);
            }
            else
            {
                std::stringstream ss;
                ss << cond_val;
                cond_str = ss.str();
            }

            // Use Database::trim instead of a lambda
            row_str = Database::trim(row_str);
            cond_str = Database::trim(cond_str);

            // Debug output
            std::cerr << "DEBUG: Comparing '" << row_str << "' vs '" << cond_str << "'\n";

            if (cond.op == "=")
                return row_str == cond_str;
            if (cond.op == "!=")
                return row_str != cond_str;
            if (cond.op == ">")
                return row_str > cond_str;
            if (cond.op == "<")
                return row_str < cond_str;
        }
    }
    catch (...)
    {
        return false;
    }
    return false;
}

std::vector<int> Database::find_matching_rows(Table &table,
                                    const std::vector<Condition> &conditions)
{
    std::vector<int> matches;
    if (conditions.empty())
    {
        for (size_t i = 0; i < table.rows.size(); i++)
            matches.push_back(i);
        return matches;
    }

    for (size_t i = 0; i < table.rows.size(); i++)
    {
        bool result = evaluate_condition(table.rows[i], conditions[0], table);
        for (size_t j = 1; j < conditions.size(); j++)
        {
            bool current = evaluate_condition(table.rows[i], conditions[j], table);
            const std::string &op = conditions[j].logical_op; // Use current condition's logical_op
            if (op == "OR")
            {
                result = result || current;
            }
            else
            {
                result = result && current;
            }
        }
        if (result)
            matches.push_back(i);
    }
    return matches;
}

std::string Database::trim(const std::string &s)
{
    auto start = s.find_first_not_of(" \t\n\r\f\v");
    auto end = s.find_last_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}
Table * Database::get_table(const std::string &name)
{
    if (tables.find(name) != tables.end())
        return tables[name].get();
    return nullptr;
}

int Database::public_get_col_index(const std::string &table_name, const std::string &col_name)
{
    return get_col_index(table_name, col_name);
}

Value Database::public_parse_value(const std::string &str, const std::string &type)
{
    return parse_value(str, type);
}

void Database::create_table(const std::string &name, const std::vector<Column> &columns)
{
    tables[name] = std::make_unique<Table>();
    tables[name]->name = name;
    tables[name]->columns = columns;
}

void Database::select_join(const std::string &table1_name,
                 const std::string &table2_name,
                 const std::vector<Condition> &join_conditions,
                 const std::vector<Condition> &where_conditions,
                 const std::vector<std::string> &selected_columns,
                 bool select_all)
{
    Table *table1 = get_table(table1_name);
    Table *table2 = get_table(table2_name);
    if (!table1 || !table2)
        return;

    // Get join column indices using proper table names
    const auto &jc = join_conditions[0];
    int col1_idx = get_col_index(jc.left_table, jc.left_col);
    int col2_idx = get_col_index(jc.right_table, jc.right_col);

    if (col1_idx == -1 || col2_idx == -1)
    {
        std::cerr << "Join columns not found: "
                  << jc.left_table << "." << jc.left_col << " vs "
                  << jc.right_table << "." << jc.right_col << "\n";
        return;
    }

    std::vector<std::vector<Value>> results;

    // Nested loop join implementation
    for (size_t i = 0; i < table1->rows.size(); i++)
    {
        const Value &join_val = table1->rows[i][col1_idx];
        for (size_t j = 0; j < table2->rows.size(); j++)
        {
            if (table2->rows[j][col2_idx] == join_val)
            {
                auto combined_row = table1->rows[i];
                combined_row.insert(combined_row.end(),
                                    table2->rows[j].begin(),
                                    table2->rows[j].end());

                // Apply WHERE conditions
                bool valid = true;
                for (const auto &cond : where_conditions)
                {
                    if (!evaluate_condition(combined_row, cond, *table1))
                    {
                        valid = false;
                        break;
                    }
                }
                if (valid)
                    results.push_back(combined_row);
            }
        }
    }

    // In Database::select_join method - replace the output section with:
    // Calculate column widths
    std::vector<size_t> col_widths;

    if (select_all)
    {
        // For all columns in both tables
        for (const auto &col : table1->columns)
        {
            size_t width = (table1->name + "." + col.name).length();
            for (const auto &row : results)
            {
                std::stringstream ss;
                ss << row[get_col_index(table1->name, col.name)];
                width = std::max(width, ss.str().length());
            }
            col_widths.push_back(width + 2);
        }
        for (const auto &col : table2->columns)
        {
            size_t width = (table2->name + "." + col.name).length();
            for (const auto &row : results)
            {
                std::stringstream ss;
                ss << row[table1->columns.size() + get_col_index(table2->name, col.name)];
                width = std::max(width, ss.str().length());
            }
            col_widths.push_back(width + 2);
        }
    }
    else
    {
        // For selected columns
        for (const auto &col_name : selected_columns)
        {
            size_t width = col_name.length();
            size_t dot_pos = col_name.find('.');
            std::string table_part = col_name.substr(0, dot_pos);
            std::string field_part = col_name.substr(dot_pos + 1);

            for (const auto &row : results)
            {
                int idx = -1;
                if (table_part == table1->name)
                {
                    idx = get_col_index(table_part, field_part);
                }
                else
                {
                    idx = table1->columns.size() + get_col_index(table_part, field_part);
                }

                if (idx != -1)
                {
                    std::stringstream ss;
                    ss << row[idx];
                    width = std::max(width, ss.str().length());
                }
            }
            col_widths.push_back(width + 2);
        }
    }

    // Print headers
    std::cout << "\nResults (" << results.size() << " rows):\n";
    if (select_all)
    {
        for (size_t i = 0; i < table1->columns.size(); i++)
        {
            std::cout << std::left << std::setw(col_widths[i])
                      << table1->name + "." + table1->columns[i].name;
        }
        for (size_t i = 0; i < table2->columns.size(); i++)
        {
            std::cout << std::left << std::setw(col_widths[table1->columns.size() + i])
                      << table2->name + "." + table2->columns[i].name;
        }
    }
    else
    {
        for (size_t i = 0; i < selected_columns.size(); i++)
        {
            std::cout << std::left << std::setw(col_widths[i]) << selected_columns[i];
        }
    }
    std::cout << "\n"
              << std::string(std::accumulate(col_widths.begin(), col_widths.end(), 0), '-') << "\n";

    // Print rows
    for (const auto &row : results)
    {
        for (size_t i = 0; i < row.size(); i++)
        {
            if (i < col_widths.size())
            {
                std::cout << std::left << std::setw(col_widths[i]) << row[i];
            }
        }
        std::cout << "\n";
    }
}

void Database::update(const std::string &table_name,
            const std::vector<std::pair<std::string, Value>> &updates,
            const std::vector<Condition> &conditions)
{
    auto &table = *tables[table_name];
    auto matches = find_matching_rows(table, conditions);

    // Safety: ensure all update columns exist
    for (auto &upd : updates)
    {
        int ci = get_col_index(table_name, upd.first);
        if (ci == -1)
        {
            throw std::runtime_error("Invalid column in UPDATE: " + upd.first);
        }
    }

    int pk_col = -1;
    for (const auto &update : updates)
    {
        int col_idx = get_col_index(table_name, update.first);
        if (col_idx != -1 && table.columns[col_idx].indexed)
        {
            pk_col = col_idx;
            break;
        }
    }

    for (int idx : matches)
    {
        if (pk_col != -1)
        {
            Value old_pk = table.rows[idx][pk_col];
            try
            {
                int old_key = std::get<int>(old_pk);
                table.index.remove(old_key);
            }
            catch (...)
            {
                std::cerr << "Error removing old key\n";
            }
        }

        for (const auto &update : updates)
        {
            int col_idx = get_col_index(table_name, update.first);
            if (col_idx != -1)
            {
                table.rows[idx][col_idx] = update.second;
            }
        }

        if (pk_col != -1)
        {
            Value new_pk = table.rows[idx][pk_col];
            try
            {
                int new_key = std::get<int>(new_pk);
                table.index.insert(new_key, idx);
            }
            catch (...)
            {
                std::cerr << "Error inserting new key\n";
            }
        }
    }
}

void Database::delete_rows(const std::string &table_name,
                 const std::vector<Condition> &conditions)
{
    auto &table = *tables[table_name];

    // Safety: ensure all condition columns exist
    for (auto &cond : conditions)
    {
        int col_idx = get_col_index(table_name, cond.column);
        if (col_idx == -1)
        {
            throw std::runtime_error("Invalid column in DELETE WHERE: " + cond.column);
        }
    }
    auto matches = find_matching_rows(table, conditions);

    int pk_col = -1;
    for (size_t i = 0; i < table.columns.size(); i++)
    {
        if (table.columns[i].indexed)
        {
            pk_col = i;
            break;
        }
    }

    std::sort(matches.rbegin(), matches.rend());
    for (int idx : matches)
    {
        if (pk_col != -1)
        {
            Value pk_val = table.rows[idx][pk_col];
            try
            {
                int key = std::get<int>(pk_val);
                table.index.remove(key);
            }
            catch (...)
            {
                std::cerr << "Error removing key from index\n";
            }
        }
        table.rows.erase(table.rows.begin() + idx);
    }
}

void Database::insert_into(const std::string &table_name, const std::vector<Value> &values)
{
    auto &table = *tables[table_name];

    // Check primary key constraint
    for (size_t i = 0; i < table.columns.size(); i++)
    {
        if (table.columns[i].indexed)
        {
            int key = 0;
            try
            {
                if (std::holds_alternative<int>(values[i]))
                {
                    key = std::get<int>(values[i]);
                }
                else if (std::holds_alternative<float>(values[i]))
                {
                    key = static_cast<int>(std::get<float>(values[i]));
                }
                else
                {
                    key = std::stoi(std::get<std::string>(values[i]));
                }

                if (!table.index.search(key).empty())
                {
                    throw std::runtime_error("Duplicate primary key");
                }
            }
            catch (const std::exception &e)
            {
                throw std::runtime_error(std::string("Index error: ") + e.what());
            }
        }
    }

    table.rows.push_back(values);
    int row_index = table.rows.size() - 1;

    for (size_t i = 0; i < table.columns.size(); i++)
    {
        if (table.columns[i].indexed)
        {
            try
            {
                int key = 0;
                if (std::holds_alternative<int>(values[i]))
                {
                    key = std::get<int>(values[i]);
                }
                else if (std::holds_alternative<float>(values[i]))
                {
                    key = static_cast<int>(std::get<float>(values[i]));
                }
                else
                {
                    key = std::stoi(std::get<std::string>(values[i]));
                }
                table.index.insert(key, row_index);
            }
            catch (...)
            {
                std::cerr << "Index update failed\n";
            }
            break;
        }
    }
}

void Database::select(const std::string &table_name,
            const std::vector<Condition> &conditions,
            const std::vector<std::string> &selected_columns,
            bool select_all)
{
    auto &table = *tables[table_name];
    if (!select_all)
    {
        // Safety: ensure all requested columns exist
        for (auto &col_name : selected_columns)
        {
            if (get_col_index(table_name, col_name) == -1)
            {
                throw std::runtime_error("Invalid column in SELECT: " + col_name);
            }
        }
    }
    std::vector<int> result_rows = find_matching_rows(table, conditions);

    // In Database::select method - replace the output section with:
    std::vector<size_t> col_widths;

    // Calculate column widths
    if (select_all)
    {
        for (const auto &col : table.columns)
        {
            size_t width = col.name.length() + (col.indexed ? 1 : 0); // Account for *
            for (const auto &row : table.rows)
            {
                std::stringstream ss;
                ss << row[get_col_index(table.name, col.name)];
                width = std::max(width, ss.str().length());
            }
            col_widths.push_back(width + 2); // Add padding
        }
    }
    else
    {
        for (const auto &col_name : selected_columns)
        {
            size_t width = col_name.length();
            int col_idx = get_col_index(table.name, col_name);
            if (col_idx != -1)
            {
                for (const auto &row : table.rows)
                {
                    std::stringstream ss;
                    ss << row[col_idx];
                    width = std::max(width, ss.str().length());
                }
            }
            col_widths.push_back(width + 2); // Add padding
        }
    }

    // Print headers
    std::cout << "\nResults (" << result_rows.size() << " rows):\n";
    if (select_all)
    {
        for (size_t i = 0; i < table.columns.size(); i++)
        {
            std::cout << std::left << std::setw(col_widths[i])
                      << table.columns[i].name + (table.columns[i].indexed ? "*" : "");
        }
    }
    else
    {
        for (size_t i = 0; i < selected_columns.size(); i++)
        {
            std::cout << std::left << std::setw(col_widths[i]) << selected_columns[i];
        }
    }
    std::cout << "\n"
              << std::string(std::accumulate(col_widths.begin(), col_widths.end(), 0), '-') << "\n";

    // Print rows
    for (int idx : result_rows)
    {
        if (select_all)
        {
            for (size_t i = 0; i < table.columns.size(); i++)
            {
                std::cout << std::left << std::setw(col_widths[i]) << table.rows[idx][i];
            }
        }
        else
        {
            for (size_t i = 0; i < selected_columns.size(); i++)
            {
                int col_idx = get_col_index(table.name, selected_columns[i]);
                if (col_idx != -1)
                {
                    std::cout << std::left << std::setw(col_widths[i]) << table.rows[idx][col_idx];
                }
            }
        }
        std::cout << "\n";
    }
}

std::ostream &operator<<(std::ostream &os, const Value &val)
{
    if (std::holds_alternative<int>(val))
    {
        os << std::get<int>(val);
    }
    else if (std::holds_alternative<float>(val))
    {
        os << std::get<float>(val);
    }
    else
    {
        os << std::get<std::string>(val);
    }
    return os;
}