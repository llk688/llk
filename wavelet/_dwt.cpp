#include <iostream>
#include <vector>
#include <stack>
#include <stdexcept>
#include "convolution.h"
#include "wavelets.c"
#include "wavelets.h"
#include "wt.template.c"
#include "wt.h"
#include "common.c"
#include "wt.template.h"
#include "templating.h"
#include "wpt_node.h"

using namespace std;

//单层小波变换
pair<vector<float>, vector<float>> dwt_single
    (const vector<float>& data, const DiscreteWavelet& wavelet, MODE mode){
    size_t data_size = data.size();
    
    size_t output_len = dwt_buffer_length(data_size, wavelet.dec_len, mode);

    // 检查输出长度是否合法
    if (output_len < 1) {
        throw runtime_error("Invalid output length.");
    }

    // 检查边界条件
    if (data_size == 1 && mode == MODE_REFLECT) {
        throw invalid_argument("Input data length must be greater than 1 for [anti]reflect mode.");
    }

    // 初始化输出向量
    vector<float> cA(output_len, 0.0f); // 低频系数
    vector<float> cD(output_len, 0.0f); // 高频系数

    // 调用外部 C 函数执行小波变换
    int retval_a = CAT(float, _dec_a)(data.data(), data_size, &wavelet, cA.data(), output_len, mode);
    int retval_d = CAT(float, _dec_d)(data.data(), data_size, &wavelet, cD.data(), output_len, mode);
    // 检查计算结果
    if (retval_a < 0 || retval_d < 0) {
        throw runtime_error("C dwt failed.");
    }

    return { cA, cD };
}
//单层小波逆变换
vector<float> idwt_single(
    const vector<float>& cA, 
    const vector<float>& cD, 
    const DiscreteWavelet& wavelet,       
    MODE mode) {
    
    // 检查输入向量的大小是否一致
    if (cA.size() != cD.size()) {
        throw invalid_argument("Low-frequency and high-frequency coefficients must have the same size.");
    }

    size_t input_len = cA.size();

    // 检查重构信号的长度
    size_t rec_len = idwt_buffer_length(input_len, wavelet.rec_len, mode);
    if (rec_len < 1) {
        throw runtime_error("Invalid coefficient arrays length for the specified wavelet and mode.");
    }

    // 初始化重构信号向量
    vector<float> rec(rec_len, 0.0f);

    // 调用适当的 C 函数进行逆变换
    int retval = float_idwt(
        cA.data(), input_len,       
        cD.data(), input_len,       
        rec.data(), rec_len,        
        &wavelet,                   
        mode                        
        );

    if (retval < 0) {
        throw runtime_error("C idwt failed.");
    }

    return rec;
}
//三层小波变换
pair<vector<vector<float>>, vector<vector<float>>> dwt_three_levels(
    const vector<float>& data,
    const DiscreteWavelet& wavelet,
    MODE mode) {
    vector<float> current_data = data;
    vector<vector<float>> cA; // 所以的低频分量
    vector<vector<float>> cD; // 所有的高频分量

    for (int level = 0; level < 3; ++level) {
        auto result = dwt_single(current_data, wavelet, mode);
        cA.push_back(result.first); // 更新低频分量
        cD.push_back(result.second); // 保存当前高频分量
        current_data = cA[level]; // 迭代下一层
    }

    return { cA, cD };
}
//三层小波逆变换
vector<float> idwt_three_levels(
    const vector<vector<float>>& cA,
    const vector<vector<float>>& cD,
    const DiscreteWavelet& wavelet,
    MODE mode) {
     
    vector<float> current_data;
    for (int level = 2; level >= 0; --level) {
        current_data = cA[level];
        current_data = idwt_single(current_data, cD[level], wavelet, mode);
    }

    return current_data;
}

//WPT系数更新
template <typename T>
void updateCoefficients(float input_signal, Node<T>& root, WaveletPacketTree<float> tree,const DiscreteWavelet* wavelet,MODE mode) {
    size_t N = wavelet->dec_len;
    size_t step = 2; // 步长
    
    size_t stack_top = 0;  // 栈顶指针

    // 初始化根节点
    memmove(root.getInputBuffer(), root.getInputBuffer() + 1, (2 * N - 1) * sizeof(T));
    root.getInputBuffer()[2 * N - 1] = input_signal; // 第一个输入样本放在缓冲区末尾
    root.setStartIndex(root.getStartIndex() + 1);

    // 将根节点压入栈
    tree.getNodeStack()[stack_top++] = &root;
    //node_stack[stack_top++] = &root;

    
    // 遍历整个树
    while (stack_top > 0) {
        // 出栈当前节点
        Node<T>* current_node = tree.getNodeStack()[--stack_top];

        // 缓存样本不足
        if (current_node->getStartIndex() < 0) {
            continue;
        }

        // 更新索引和缓存移动标志
        current_node->setStartIndex(current_node->getStartIndex() % step);
        bool shift_buffer = (current_node->getStartIndex() == 1);

        if (current_node->getParent()) {
            shift_buffer = current_node->getParent()->getShiftBuffer() && shift_buffer;
        }
        current_node->setShiftBuffer(shift_buffer);

        // 如果是叶子节点，处理小波系数
        if (current_node->isLeaf()) {
            // 对最后 N 个数据执行去噪（此处无操作，留空实现）
            // 可以在此处添加去噪逻辑
            continue;
        }

        // 对当前节点的子节点进行处理
        
        Node<T>* left_child = current_node->getLeft();  // 左子节点：近似系数
        //if (left_child) {
            if (current_node->getShiftBuffer()) {
                if (left_child) {
                    left_child->setStartIndex(left_child->getStartIndex() + 1);
                    memmove(left_child->getInputBuffer(), left_child->getInputBuffer() + 1, (N) * sizeof(T));
                }
            }

            // 调用小波分解函数
            if (left_child) {
                size_t output_len = dwt_buffer_length(2 * N - current_node->getStartIndex(), wavelet->dec_len, mode);
                CAT(float, _dec_a)(
                    current_node->getInputBuffer() + current_node->getStartIndex(),
                    2 * N - current_node->getStartIndex(),
                    wavelet,
                    left_child->getInputBuffer() + N-1,
                    output_len,
                    mode
                    );
            }
            // 如果子节点缓存已经可以处理，将其入栈
            if (left_child && left_child->getStartIndex() >= 0) {
                tree.getNodeStack()[stack_top++] = (left_child);
            }
        //}
        Node<T>* right_child = current_node->getRight(); // 右子节点：细节系数
            if (current_node->getShiftBuffer()) {
                if (right_child) {
                    right_child->setStartIndex(right_child->getStartIndex() + 1);
                    memmove(right_child->getInputBuffer(), right_child->getInputBuffer() + 1, (N) * sizeof(T));
                }
            }
            if (right_child) {
                size_t output_len = dwt_buffer_length(2 * N - current_node->getStartIndex(), wavelet->dec_len, mode);
                CAT(float, _dec_d)(
                    current_node->getInputBuffer() + current_node->getStartIndex(),
                    2 * N - current_node->getStartIndex(),
                    wavelet,
                    right_child->getInputBuffer() + N-1,
                    output_len,
                    mode
                    );
            }
            if (right_child && right_child->getStartIndex() >= 0) {
                tree.getNodeStack()[stack_top++] = (right_child);
            }
    }
}

template <typename T>
void reconstructCoefficients(Node<T>* node, const DiscreteWavelet* wavelet, MODE mode,bool isRoot,vector<float>*rec) {
    if (!node) return;
    
    //递归处理左右子节点
     reconstructCoefficients<T>(node->getLeft(), wavelet, mode, false,rec);
     reconstructCoefficients<T>(node->getRight(), wavelet, mode, false,rec);

    size_t N = wavelet->dec_len;
    size_t output_len = idwt_buffer_length(N, wavelet->rec_len, mode);
    size_t step = 2; // 步长

    if (node->getLeft() != nullptr && node->getRight() != nullptr) { 
        if (!isRoot) {  //非根节点
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                node->getRight()->getInputBuffer() + N, N,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //根节点
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                node->getRight()->getInputBuffer() + N, N,
                rec->data(), output_len,
                wavelet,
                mode
                );
        }
    }
    if (node->getLeft() != nullptr && node->getRight() == nullptr) {
        if (!isRoot) {  //非根节点
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                NULL, 0,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //根节点
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                NULL, 0,
                //node->getInputBuffer() + N, output_len,
                rec->data()+N, output_len,
                wavelet,
                mode
                );
            //CAT(float, _idwt) (
            //    node->getLeft()->getInputBuffer() + N, N,
            //    NULL, 0,
            //    node->getInputBuffer() + N, output_len,
            //    //rec.data() + N, output_len,
            //    wavelet,
            //    mode
            //    );
        }
    }
    if (node->getLeft() == nullptr && node->getRight() != nullptr) {
        if (!isRoot) {  //非根节点
            CAT(float, _idwt) (
                NULL, 0,
                node->getRight()->getInputBuffer() + N, N,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //根节点
            CAT(float, _idwt) (
                NULL, 0,
                node->getRight()->getInputBuffer() + N, N,
                rec->data(), output_len,
                wavelet,
                mode
                );
        }
    }
}
