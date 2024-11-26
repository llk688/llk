#pragma once
#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstring>  
#include <stdexcept> 
#include <vector>
#include "wavelets.h"

// �ڵ����Ͷ���
enum class NodeType {
    APPROX,       // ����ϵ���ڵ�
    DETAIL,       // ϸ��ϵ���ڵ�
    INTERMEDIATE  // �м�ڵ㣨��Ҷ�ӽڵ㣩
};

// �ڵ���ģ��
template <typename T>
class Node {
public:
    // ���캯��
    Node(const std::string& path, NodeType type, bool is_leaf,size_t N, Node* parent = nullptr);

    // ��������
    ~Node();
 
    // ��ȡ·��
    const std::string& getPath() const { return path; }

    // �ж��Ƿ�ΪҶ�ӽڵ�
    bool isLeaf() const { return is_leaf; }

    // ��ȡ�ڵ�����
    NodeType getType() const { return type; }

    // ������ز���
    T* getInputBuffer() { return input_buffer; }
    const T* getInputBuffer() const { return input_buffer; }
    size_t getBufferSize() const { return buffer_size; }
    int getStartIndex() const { return start_index; }
    void setStartIndex(int index) { start_index = index; }

    //���ڵ����
    Node* getParent() const { return parent; }
    
    // �ӽڵ����
    Node<T>* getLeft() const { return left; }
    Node<T>* getRight() const { return right; }
    void setLeft(Node<T>* node) { left = node; }
    void setRight(Node<T>* node) { right = node; }

    // ��սڵ㻺��
    void clearBuffer();
    
    void setShiftBuffer(bool shift) {
        shift_buffer = shift;
    }
    bool getShiftBuffer() const {
        return shift_buffer;
    }

private: 
    Node* parent;          // ���ڵ�ָ��
    std::string path;      // �ڵ�·��
    NodeType type;         // �ڵ�����
    bool is_leaf;          // �Ƿ�ΪҶ�ӽڵ�
    bool shift_buffer;     // �Ƿ���Ҫ��λ

    T* input_buffer;        // ���뻺��
    size_t buffer_size;     // �����С
    int start_index;

    Node<T>* left;      // ���ӽڵ�ָ�루���ƣ�
    Node<T>* right;     // ���ӽڵ�ָ�루ϸ�ڣ�
};


template <typename T>
Node<T>::Node(const std::string& path, NodeType type, bool is_leaf,size_t N, Node* parent = nullptr)
    : path(path), type(type), is_leaf(is_leaf), parent(nullptr), start_index(0), left(nullptr), right(nullptr) {
    //if (is_leaf) {
    //    buffer_size = N;
    //}
    //else if (path.empty()) { // ���ڵ�
    //    buffer_size = 2 * N;
    //}
    //else { // �Ǹ�����Ҷ�ӽڵ�
    //    buffer_size = 2 * N + 1;
    //}
    //
    buffer_size = 2 * N;
    input_buffer = new T[buffer_size]();// ��ʼ������Ϊ 0
    if (!input_buffer) {
        throw std::bad_alloc(); // �������ʧ�ܣ��׳��쳣
    }
}

// ��������
template <typename T>
Node<T>::~Node() {
    if(left)
        delete left;  // �������ӽڵ�
    if(right)
        delete right; // �������ӽڵ�
    delete[] input_buffer; // �ͷŷ�����ڴ�
    input_buffer = nullptr; // ��������ָ��
}


template <typename T>
class WaveletPacketTree {
public:
    WaveletPacketTree(size_t N, int levels)
        : root(nullptr), max_levels(levels), buffer_size(N) {}

    ~WaveletPacketTree() {
        deleteTree(root);
    }

    // ������
    void buildTree(Node<T>* node) {
        //root = new Node<T>("", NodeType::INTERMEDIATE, false, N);
        buildSubTree(node, 0);
    }

    // ��ӡ���ṹ
    void printTree(Node<T>* node) const {
        printSubTree(node, 0);
    }

private:
    Node<T>* root;         // ���ĸ��ڵ�
    int max_levels;        // ������
    size_t buffer_size;    // ���㻺���С

    // �ݹ鹹������
    void buildSubTree(Node<T>* node, int current_level) {
        if (current_level >= max_levels) return;

        // �������ӽڵ㣨����ϵ����
        std::string left_path = node->getPath() + "a";
        NodeType left_type = current_level == max_levels - 1 ? NodeType::APPROX : NodeType::INTERMEDIATE;
        bool left_is_leaf = (current_level == max_levels - 1);
        node->setLeft(new Node<T>(left_path, left_type, left_is_leaf, buffer_size, node));

        // �������ӽڵ㣨ϸ��ϵ����
        std::string right_path = node->getPath() + "d";
        NodeType right_type = current_level == max_levels - 1 ? NodeType::DETAIL : NodeType::INTERMEDIATE;
        bool right_is_leaf = (current_level == max_levels - 1);
        node->setRight(new Node<T>(right_path, right_type, right_is_leaf, buffer_size, node));

        // �ݹ鹹���ӽڵ�
        buildSubTree(node->getLeft(), current_level + 1);
        buildSubTree(node->getRight(), current_level + 1);
    }

    // ɾ������
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
                current->setLeft(nullptr); // ������ӽڵ�ָ��
            }
            if (current->getRight()) {
                nodeStack.push(current->getRight());
                current->setRight(nullptr); // ������ӽڵ�ָ��
            }

            delete current;
        }
    }


    // ��ӡ�����ṹ
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


// ��սڵ㻺��
template <typename T>
void Node<T>::clearBuffer() {
    memset(input_buffer, 0, sizeof(input_buffer)); // ���û�������
    start_index = 0;
}



#endif // NODE_H

