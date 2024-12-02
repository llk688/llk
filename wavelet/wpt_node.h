#pragma once
#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstring>  
#include <stdexcept> 
#include <vector>
#include <unordered_map>
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
    Node(const std::string& path, NodeType type, bool is_leaf, Node* parent = nullptr);

    // 析构函数
    ~Node();

    //初始化node
    bool Node<T>::initialize(size_t N);
 
    // 获取路径
    const std::string& getPath() const { return path; }

    // 判断是否为叶子节点
    //bool isLeaf() const { return is_leaf; }
    bool isLeaf() const { return left == nullptr && right == nullptr; }

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

    
    void setShiftBuffer(bool shift) {
        shift_buffer = shift;
    }
    bool getShiftBuffer() const {
        return shift_buffer;
    }

    // 清空节点缓存
    void Node<T>::clearBuffer() {
        memset(input_buffer, 0, sizeof(input_buffer)); // 重置缓存数据
        start_index = 0;
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

    bool initialized;      //节点是否初始化成功
};


template <typename T>
Node<T>::Node(const std::string& path, NodeType type, bool is_leaf, Node* parent = nullptr)
    : path(path), type(type), is_leaf(is_leaf), parent(nullptr), start_index(0), left(nullptr), right(nullptr) {}

template <typename T>
bool Node<T>::initialize(size_t N) {
    buffer_size = 2 * N;
    input_buffer = new(std::nothrow) T[buffer_size]();  // 使用 std::nothrow 避免异常

    // 检查内存分配是否成功
    if (!input_buffer) {
        return false;  // 内存分配失败
    }

    // 初始化成功
    return true;
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
        : root(nullptr), max_levels(levels), buffer_size(N) {
        node_stack = new Node<T>* [max_levels]; // 固定大小的栈levels层
    }

    ~WaveletPacketTree() {
        deleteTree(root);
    }

    // 返回指向 node_stack 的指针
    Node<T>** getNodeStack() {
        return node_stack; 
    }

    // 构建树
    bool buildTree(Node<T>*root,size_t N, const std::string& path) {
        if (!root || !buildSubTree(root, path)) {
            return false;
        };
        if (!initializeCaches(root,N)) {
            return false;  // 缓存初始化失败
        }

        return true;
    }

    // 初始化树的缓存
    bool initializeCaches(Node<float>* node,size_t N) {
        if (!node) return true;

        // 初始化当前节点的缓存
        if (!node->initialize(N)) {
            return false;  // 缓存初始化失败
        }

        // 递归初始化左右子树的缓存
        return initializeCaches(node->getLeft(),N) && initializeCaches(node->getRight(),N);
    }

    // 打印树结构
    void printTree(Node<T>* node) const {
        printSubTree(node, 0);
    }

private:
    Node<T>* root;         // 树的根节点
    int max_levels;        // 最大层数
    size_t buffer_size;    // 单层缓存大小
    Node<T>** node_stack;  // 节点指针栈
    std::unordered_map<std::string, Node<T>*> node_map;   //路径到节点的映射


    bool buildSubTree(Node<T>* current, const std::string& path) {
        if (path.empty()) return false;  // 路径空则不做任何操作

        buildPath(current, path,1);
        return true;
    }

    void buildPath(Node<T>* current, const std::string& path,int i) {
        if (path.empty()) return;
        
        if (i > max_levels) {
            return;
        }
            // 检查路径是否已经创建过该节点
            if (node_map.find(path.substr(0,i)) != node_map.end()) {   // 如果该路径的节点已经存在，则验证下一节点
                if (path[i-1] == 'a') {
                    Node<T>* newNode = current->getLeft();
                    buildPath(newNode, path, i += 1);
                }
                else {
                    Node<T>* newNode = current->getRight();
                    buildPath(newNode, path, i += 1);
                }
                  
            }
            else {
                // 创建新节点
                NodeType type;
                if (i == max_levels) {
                    // 如果i == max_levels，表示当前节点是叶子节点
                    type = (path[i - 1] == 'a') ? NodeType::APPROX : NodeType::DETAIL;
                }
                else {
                    // 否则，当前节点是中间节点
                    type = NodeType::INTERMEDIATE;
                }
                //NodeType type = (path[i-1] == 'a') ? NodeType::APPROX : NodeType::DETAIL;
                Node<T>* newNode = new Node<T>(path.substr(0, i), type, current);
                node_map[path.substr(0, i)] = newNode;  // 将路径与节点的映射关系保存下来

                // 根据路径字符类型，决定将该节点作为左子节点或右子节点
                if (path[i] == 'a') {
                    current->setLeft(newNode);
                }
                else {
                    current->setRight(newNode);
                }
                buildPath(newNode, path, i += 1);
            }
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
            std::cout << " Input Buffer: [";
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


#endif // NODE_H

