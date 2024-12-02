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

//����С���任
pair<vector<float>, vector<float>> dwt_single
    (const vector<float>& data, const DiscreteWavelet& wavelet, MODE mode){
    size_t data_size = data.size();
    
    size_t output_len = dwt_buffer_length(data_size, wavelet.dec_len, mode);

    // �����������Ƿ�Ϸ�
    if (output_len < 1) {
        throw runtime_error("Invalid output length.");
    }

    // ���߽�����
    if (data_size == 1 && mode == MODE_REFLECT) {
        throw invalid_argument("Input data length must be greater than 1 for [anti]reflect mode.");
    }

    // ��ʼ���������
    vector<float> cA(output_len, 0.0f); // ��Ƶϵ��
    vector<float> cD(output_len, 0.0f); // ��Ƶϵ��

    // �����ⲿ C ����ִ��С���任
    int retval_a = CAT(float, _dec_a)(data.data(), data_size, &wavelet, cA.data(), output_len, mode);
    int retval_d = CAT(float, _dec_d)(data.data(), data_size, &wavelet, cD.data(), output_len, mode);
    // ��������
    if (retval_a < 0 || retval_d < 0) {
        throw runtime_error("C dwt failed.");
    }

    return { cA, cD };
}
//����С����任
vector<float> idwt_single(
    const vector<float>& cA, 
    const vector<float>& cD, 
    const DiscreteWavelet& wavelet,       
    MODE mode) {
    
    // ������������Ĵ�С�Ƿ�һ��
    if (cA.size() != cD.size()) {
        throw invalid_argument("Low-frequency and high-frequency coefficients must have the same size.");
    }

    size_t input_len = cA.size();

    // ����ع��źŵĳ���
    size_t rec_len = idwt_buffer_length(input_len, wavelet.rec_len, mode);
    if (rec_len < 1) {
        throw runtime_error("Invalid coefficient arrays length for the specified wavelet and mode.");
    }

    // ��ʼ���ع��ź�����
    vector<float> rec(rec_len, 0.0f);

    // �����ʵ��� C ����������任
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
//����С���任
pair<vector<vector<float>>, vector<vector<float>>> dwt_three_levels(
    const vector<float>& data,
    const DiscreteWavelet& wavelet,
    MODE mode) {
    vector<float> current_data = data;
    vector<vector<float>> cA; // ���Եĵ�Ƶ����
    vector<vector<float>> cD; // ���еĸ�Ƶ����

    for (int level = 0; level < 3; ++level) {
        auto result = dwt_single(current_data, wavelet, mode);
        cA.push_back(result.first); // ���µ�Ƶ����
        cD.push_back(result.second); // ���浱ǰ��Ƶ����
        current_data = cA[level]; // ������һ��
    }

    return { cA, cD };
}
//����С����任
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

//WPTϵ������
template <typename T>
void updateCoefficients(float input_signal, Node<T>& root, WaveletPacketTree<float> tree,const DiscreteWavelet* wavelet,MODE mode) {
    size_t N = wavelet->dec_len;
    size_t step = 2; // ����
    
    size_t stack_top = 0;  // ջ��ָ��

    // ��ʼ�����ڵ�
    memmove(root.getInputBuffer(), root.getInputBuffer() + 1, (2 * N - 1) * sizeof(T));
    root.getInputBuffer()[2 * N - 1] = input_signal; // ��һ�������������ڻ�����ĩβ
    root.setStartIndex(root.getStartIndex() + 1);

    // �����ڵ�ѹ��ջ
    tree.getNodeStack()[stack_top++] = &root;
    //node_stack[stack_top++] = &root;

    
    // ����������
    while (stack_top > 0) {
        // ��ջ��ǰ�ڵ�
        Node<T>* current_node = tree.getNodeStack()[--stack_top];

        // ������������
        if (current_node->getStartIndex() < 0) {
            continue;
        }

        // ���������ͻ����ƶ���־
        current_node->setStartIndex(current_node->getStartIndex() % step);
        bool shift_buffer = (current_node->getStartIndex() == 1);

        if (current_node->getParent()) {
            shift_buffer = current_node->getParent()->getShiftBuffer() && shift_buffer;
        }
        current_node->setShiftBuffer(shift_buffer);

        // �����Ҷ�ӽڵ㣬����С��ϵ��
        if (current_node->isLeaf()) {
            // ����� N ������ִ��ȥ�루�˴��޲���������ʵ�֣�
            // �����ڴ˴����ȥ���߼�
            continue;
        }

        // �Ե�ǰ�ڵ���ӽڵ���д���
        
        Node<T>* left_child = current_node->getLeft();  // ���ӽڵ㣺����ϵ��
        //if (left_child) {
            if (current_node->getShiftBuffer()) {
                if (left_child) {
                    left_child->setStartIndex(left_child->getStartIndex() + 1);
                    memmove(left_child->getInputBuffer(), left_child->getInputBuffer() + 1, (N) * sizeof(T));
                }
            }

            // ����С���ֽ⺯��
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
            // ����ӽڵ㻺���Ѿ����Դ���������ջ
            if (left_child && left_child->getStartIndex() >= 0) {
                tree.getNodeStack()[stack_top++] = (left_child);
            }
        //}
        Node<T>* right_child = current_node->getRight(); // ���ӽڵ㣺ϸ��ϵ��
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
    
    //�ݹ鴦�������ӽڵ�
     reconstructCoefficients<T>(node->getLeft(), wavelet, mode, false,rec);
     reconstructCoefficients<T>(node->getRight(), wavelet, mode, false,rec);

    size_t N = wavelet->dec_len;
    size_t output_len = idwt_buffer_length(N, wavelet->rec_len, mode);
    size_t step = 2; // ����

    if (node->getLeft() != nullptr && node->getRight() != nullptr) { 
        if (!isRoot) {  //�Ǹ��ڵ�
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                node->getRight()->getInputBuffer() + N, N,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //���ڵ�
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
        if (!isRoot) {  //�Ǹ��ڵ�
            CAT(float, _idwt) (
                node->getLeft()->getInputBuffer() + N, N,
                NULL, 0,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //���ڵ�
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
        if (!isRoot) {  //�Ǹ��ڵ�
            CAT(float, _idwt) (
                NULL, 0,
                node->getRight()->getInputBuffer() + N, N,
                node->getInputBuffer() + N, output_len,
                wavelet,
                mode
                );
        }
        else {   //���ڵ�
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
