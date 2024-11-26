#pragma once
#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstring>  
#include <stdexcept> 
#include <vector>
#include "wavelets.h"

// 节点类型定义
enum class NodeType {
    APPROX,       // 近似系数节点
    DETAIL,       // 细节系数节点
    INTERMEDIATE  // 中间节点（非叶子节点）
};

// 节点类模板
template <typename T>
class Node {
public:
    // 构造函数
    Node(const std::string& path, NodeType type, bool is_leaf,size_t N, Node* parent = nullptr);

    // 析构函数
    ~Node();
 
    // 获取路径
    const std::string& getPath() const { return path; }

    // 判断是否为叶子节点
    bool isLeaf() const { return is_leaf; }

    // 获取节点类型
    NodeType getType() const { return type; }

    // 缓存相关操作
    T* getInputBuffer() { return input_buffer; }
    const T* getInputBuffer() const { return input_buffer; }
    size_t getBufferSize() const { return buffer_size; }
    int getStartIndex() const { return start_index; }
    void setStartIndex(int index) { start_index = index; }

    //父节点管理
    Node* getParent() const { return parent; }
    
    // 子节点管理
    Node<T>* getLeft() const { return left; }
    Node<T>* getRight() const { return right; }
    void setLeft(Node<T>* node) { left = node; }
    void setRight(Node<T>* node) { right = node; }

    // 清空节点缓存
    void clearBuffer();
    
    void setShiftBuffer(bool shift) {
        shift_buffer = shift;
    }
    bool getShiftBuffer() const {
        return shift_buffer;
    }

private: 
    Node* parent;          // 父节点指针
    std::string path;      // 节点路径
    NodeType type;         // 节点类型
    bool is_leaf;          // 是否为叶子节点
    bool shift_buffer;     // 是否需要移位

    T* input_buffer;        // 输入缓存
    size_t buffer_size;     // 缓存大小
    int start_index;

    Node<T>* left;      // 左子节点指针（近似）
    Node<T>* right;     // 右子节点指针（细节）
};


template <typename T>
Node<T>::Node(const std::string& path, NodeType type, bool is_leaf,size_t N, Node* parent = nullptr)
    : path(path), type(type), is_leaf(is_leaf), parent(nullptr), start_index(0), left(nullptr), right(nullptr) {
    //if (is_leaf) {
    //    buffer_size = N;
    //}
    //else if (path.empty()) { // 根节点
    //    buffer_size = 2 * N;
    //}
    //else { // 非根、非叶子节点
    //    buffer_size = 2 * N + 1;
    //}
    //
    buffer_size = 2 * N;
    input_buffer = new T[buffer_size]();// 初始化缓存为 0
    if (!input_buffer) {
        throw std::bad_alloc(); // 如果分配失败，抛出异常
    }
}

// 析构函数
template <typename T>
Node<T>::~Node() {
    if(left)
        delete left;  // 销毁左子节点
    if(right)
        delete right; // 销毁右子节点
    delete[] input_buffer; // 释放分配的内存
    input_buffer = nullptr; // 避免悬挂指针
}


template <typename T>
class WaveletPacketTree {
public:
    WaveletPacketTree(size_t N, int levels)
        : root(nullptr), max_levels(levels), buffer_size(N) {}

    ~WaveletPacketTree() {
        deleteTree(root);
    }

    // 构建树
    void buildTree(Node<T>* node) {
        //root = new Node<T>("", NodeType::INTERMEDIATE, false, N);
        buildSubTree(node, 0);
    }

    // 打印树结构
    void printTree(Node<T>* node) const {
        printSubTree(node, 0);
    }

private:
    Node<T>* root;         // 树的根节点
    int max_levels;        // 最大层数
    size_t buffer_size;    // 单层缓存大小

    // 递归构建子树
    void buildSubTree(Node<T>* node, int current_level) {
        if (current_level >= max_levels) return;

        // 创建左子节点（近似系数）
        std::string left_path = node->getPath() + "a";
        NodeType left_type = current_level == max_levels - 1 ? NodeType::APPROX : NodeType::INTERMEDIATE;
        bool left_is_leaf = (current_level == max_levels - 1);
        node->setLeft(new Node<T>(left_path, left_type, left_is_leaf, buffer_size, node));

        // 创建右子节点（细节系数）
        std::string right_path = node->getPath() + "d";
        NodeType right_type = current_level == max_levels - 1 ? NodeType::DETAIL : NodeType::INTERMEDIATE;
        bool right_is_leaf = (current_level == max_levels - 1);
        node->setRight(new Node<T>(right_path, right_type, right_is_leaf, buffer_size, node));

        // 递归构建子节点
        buildSubTree(node->getLeft(), current_level + 1);
        buildSubTree(node->getRight(), current_level + 1);
    }

    // 删除子树
    void deleteTree(Node<T>* node) {
        if (!node) return;
        //delete node;
        std::stack<Node<T>*> nodeStack;
        nodeStack.push(node);

        while (!nodeStack.empty()) {
            Node<T>* current = nodeStack.top();
            nodeStack.pop();

            if (current->getLeft()) {
                nodeStack.push(current->getLeft());
                current->setLeft(nullptr); // 清空左子节点指针
            }
            if (current->getRight()) {
                nodeStack.push(current->getRight());
                current->setRight(nullptr); // 清空右子节点指针
            }

            delete current;
        }
    }


    // 打印子树结构
    void printSubTree(Node<T>* node, int level) const {
        if (!node) return;
        std::cout << std::string(level * 2, ' ') << "Path: " << node->getPath()
            << ", Leaf: " << node->isLeaf()
            << ", Type: " << (node->getType() == NodeType::APPROX ? "APPROX" :
                (node->getType() == NodeType::DETAIL ? "DETAIL" : "INTERMEDIATE"));
        const T* buffer = node->getInputBuffer();
        if (buffer) {
            std::cout << "Input Buffer: [";
            for (size_t i = 0; i < node->getBufferSize(); ++i) {
                std::cout << buffer[i];
                if (i < node->getBufferSize() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
        cout << "\n";
        printSubTree(node->getLeft(), level + 1);
        printSubTree(node->getRight(), level + 1);
    }
};


// 清空节点缓存
template <typename T>
void Node<T>::clearBuffer() {
    memset(input_buffer, 0, sizeof(input_buffer)); // 重置缓存数据
    start_index = 0;
}



#endif // NODE_H

