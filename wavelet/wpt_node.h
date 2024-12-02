#pragma once
#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstring>  
#include <stdexcept> 
#include <vector>
#include <unordered_map>
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
    Node(const std::string& path, NodeType type, bool is_leaf, Node* parent = nullptr);

    // ��������
    ~Node();

    //��ʼ��node
    bool Node<T>::initialize(size_t N);
 
    // ��ȡ·��
    const std::string& getPath() const { return path; }

    // �ж��Ƿ�ΪҶ�ӽڵ�
    //bool isLeaf() const { return is_leaf; }
    bool isLeaf() const { return left == nullptr && right == nullptr; }

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

    
    void setShiftBuffer(bool shift) {
        shift_buffer = shift;
    }
    bool getShiftBuffer() const {
        return shift_buffer;
    }

    // ��սڵ㻺��
    void Node<T>::clearBuffer() {
        memset(input_buffer, 0, sizeof(input_buffer)); // ���û�������
        start_index = 0;
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

    bool initialized;      //�ڵ��Ƿ��ʼ���ɹ�
};


template <typename T>
Node<T>::Node(const std::string& path, NodeType type, bool is_leaf, Node* parent = nullptr)
    : path(path), type(type), is_leaf(is_leaf), parent(nullptr), start_index(0), left(nullptr), right(nullptr) {}

template <typename T>
bool Node<T>::initialize(size_t N) {
    buffer_size = 2 * N;
    input_buffer = new(std::nothrow) T[buffer_size]();  // ʹ�� std::nothrow �����쳣

    // ����ڴ�����Ƿ�ɹ�
    if (!input_buffer) {
        return false;  // �ڴ����ʧ��
    }

    // ��ʼ���ɹ�
    return true;
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
        : root(nullptr), max_levels(levels), buffer_size(N) {
        node_stack = new Node<T>* [max_levels]; // �̶���С��ջlevels��
    }

    ~WaveletPacketTree() {
        deleteTree(root);
    }

    // ����ָ�� node_stack ��ָ��
    Node<T>** getNodeStack() {
        return node_stack; 
    }

    // ������
    bool buildTree(Node<T>*root,size_t N, const std::string& path) {
        if (!root || !buildSubTree(root, path)) {
            return false;
        };
        if (!initializeCaches(root,N)) {
            return false;  // �����ʼ��ʧ��
        }

        return true;
    }

    // ��ʼ�����Ļ���
    bool initializeCaches(Node<float>* node,size_t N) {
        if (!node) return true;

        // ��ʼ����ǰ�ڵ�Ļ���
        if (!node->initialize(N)) {
            return false;  // �����ʼ��ʧ��
        }

        // �ݹ��ʼ�����������Ļ���
        return initializeCaches(node->getLeft(),N) && initializeCaches(node->getRight(),N);
    }

    // ��ӡ���ṹ
    void printTree(Node<T>* node) const {
        printSubTree(node, 0);
    }

private:
    Node<T>* root;         // ���ĸ��ڵ�
    int max_levels;        // ������
    size_t buffer_size;    // ���㻺���С
    Node<T>** node_stack;  // �ڵ�ָ��ջ
    std::unordered_map<std::string, Node<T>*> node_map;   //·�����ڵ��ӳ��


    bool buildSubTree(Node<T>* current, const std::string& path) {
        if (path.empty()) return false;  // ·���������κβ���

        buildPath(current, path,1);
        return true;
    }

    void buildPath(Node<T>* current, const std::string& path,int i) {
        if (path.empty()) return;
        
        if (i > max_levels) {
            return;
        }
            // ���·���Ƿ��Ѿ��������ýڵ�
            if (node_map.find(path.substr(0,i)) != node_map.end()) {   // �����·���Ľڵ��Ѿ����ڣ�����֤��һ�ڵ�
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
                // �����½ڵ�
                NodeType type;
                if (i == max_levels) {
                    // ���i == max_levels����ʾ��ǰ�ڵ���Ҷ�ӽڵ�
                    type = (path[i - 1] == 'a') ? NodeType::APPROX : NodeType::DETAIL;
                }
                else {
                    // ���򣬵�ǰ�ڵ����м�ڵ�
                    type = NodeType::INTERMEDIATE;
                }
                //NodeType type = (path[i-1] == 'a') ? NodeType::APPROX : NodeType::DETAIL;
                Node<T>* newNode = new Node<T>(path.substr(0, i), type, current);
                node_map[path.substr(0, i)] = newNode;  // ��·����ڵ��ӳ���ϵ��������

                // ����·���ַ����ͣ��������ýڵ���Ϊ���ӽڵ�����ӽڵ�
                if (path[i] == 'a') {
                    current->setLeft(newNode);
                }
                else {
                    current->setRight(newNode);
                }
                buildPath(newNode, path, i += 1);
            }
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

