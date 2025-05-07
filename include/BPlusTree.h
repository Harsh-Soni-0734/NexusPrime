#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>

// ------------------- B+ Tree Implementation -------------------
const int ORDER = 4;

struct BPlusNode
{
    bool is_leaf;
    std::vector<int> keys;
    std::vector<int> values;
    std::vector<BPlusNode *> children;
    BPlusNode *next;
    BPlusNode *parent;

    BPlusNode(bool leaf = false) : is_leaf(leaf), next(nullptr), parent(nullptr) {}
};

class BPlusTree
{
private:
    BPlusNode *root;
    void insertInternal(int key, BPlusNode *child, BPlusNode *parent);

public:
    BPlusTree() : root(nullptr) {}

    void insert(int key, int value);
    
    void remove(int key);

    std::vector<int> range_search(int min_key, int max_key);

    std::vector<int> search(int key);

private:
    BPlusNode *find_leaf(int key);
};

#endif // BPLUSTREE_H