#include "BPlusTree.h"
#include <iostream>

void BPlusTree::insertInternal(int key, BPlusNode *child, BPlusNode *parent)
    {
        if (parent == nullptr)
        {
            root = new BPlusNode(false);
            root->keys.push_back(key);
            root->children.push_back(child);
            child->parent = root;
            return;
        }

        auto pos = std::lower_bound(parent->keys.begin(), parent->keys.end(), key);
        int idx = pos - parent->keys.begin();
        parent->keys.insert(pos, key);
        parent->children.insert(parent->children.begin() + idx + 1, child);

        if (parent->keys.size() > ORDER)
        {
            BPlusNode *new_node = new BPlusNode(false);
            int split_pos = parent->keys.size() / 2;
            int split_key = parent->keys[split_pos];

            new_node->keys.assign(parent->keys.begin() + split_pos + 1, parent->keys.end());
            new_node->children.assign(parent->children.begin() + split_pos + 1, parent->children.end());

            parent->keys.resize(split_pos);
            parent->children.resize(split_pos + 1);

            insertInternal(split_key, new_node, parent->parent);
        }
    }

    void BPlusTree::insert(int key, int value)
    {
        if (root == nullptr)
        {
            root = new BPlusNode(true);
            root->keys.push_back(key);
            root->values.push_back(value);
            return;
        }

        BPlusNode *current = root;
        BPlusNode *parent = nullptr;

        while (!current->is_leaf)
        {
            parent = current;
            auto pos = std::upper_bound(current->keys.begin(), current->keys.end(), key);
            int idx = pos - current->keys.begin();
            current = current->children[idx];
        }

        auto pos = std::lower_bound(current->keys.begin(), current->keys.end(), key);
        size_t idx = pos - current->keys.begin();

        // Check for duplicate key
        if (idx < current->keys.size() && current->keys[idx] == key)
        {
            throw std::runtime_error("Duplicate key");
        }

        current->keys.insert(pos, key);
        current->values.insert(current->values.begin() + idx, value);

        if (current->keys.size() > ORDER)
        {
            BPlusNode *new_node = new BPlusNode(true);
            int split_pos = current->keys.size() / 2;

            new_node->keys.assign(current->keys.begin() + split_pos, current->keys.end());
            new_node->values.assign(current->values.begin() + split_pos, current->values.end());
            current->keys.resize(split_pos);
            current->values.resize(split_pos);

            new_node->next = current->next;
            current->next = new_node;

            if (parent == nullptr)
            {
                parent = new BPlusNode(false);
                root = parent;
                parent->children.push_back(current);
            }

            new_node->parent = parent;
            insertInternal(new_node->keys[0], new_node, parent);
        }
    }

    void BPlusTree::remove(int key)
    {
        if (!root)
            return;
        BPlusNode *leaf = find_leaf(key);
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it != leaf->keys.end() && *it == key)
        {
            int idx = it - leaf->keys.begin();
            leaf->keys.erase(it);
            leaf->values.erase(leaf->values.begin() + idx);
        }
    }

    std::vector<int> BPlusTree::range_search(int min_key, int max_key)
    {
        std::vector<int> results;
        if (!root)
            return results;

        BPlusNode *node = find_leaf(min_key);
        std::cerr << "Range search from " << min_key << " to " << max_key << std::endl;
        while (node)
        {
            std::cerr << "Current node keys: ";
            for (int k : node->keys)
                std::cerr << k << " ";
            std::cerr << std::endl;
            for (size_t i = 0; i < node->keys.size(); i++)
            {
                int key = node->keys[i];
                if (key > max_key)
                {
                    std::cerr << "Key " << key << " exceeds max, stopping." << std::endl;
                    return results;
                }
                if (key >= min_key)
                {
                    std::cerr << "Adding value " << node->values[i] << " for key " << key << std::endl;
                    results.push_back(node->values[i]);
                }
            }
            node = node->next;
        }
        return results;
    }

    std::vector<int> BPlusTree::search(int key)
    {
        std::vector<int> results;
        BPlusNode *leaf = find_leaf(key);
        if (!leaf)
            return results;

        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it != leaf->keys.end() && *it == key)
        {
            int idx = it - leaf->keys.begin();
            results.push_back(leaf->values[idx]);
        }
        return results;
    }

    BPlusNode * BPlusTree::find_leaf(int key)
    {
        BPlusNode *current = root;
        if (!current)
            return nullptr;
        while (!current->is_leaf)
        {
            auto it = std::lower_bound(current->keys.begin(), current->keys.end(), key);
            size_t idx = it - current->keys.begin();
            // Ensure idx does not exceed children size
            if (idx >= current->children.size())
                idx = current->children.size() - 1;
            current = current->children[idx];
        }
        return current;
    }