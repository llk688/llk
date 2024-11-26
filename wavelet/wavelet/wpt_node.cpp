//#include <iostream>
//#include <vector>
//#include <stdexcept>
//#include "wavelets.h"
//#include "wpt_node.h"
//#include "common.h"
////#include "wt.template.c"
//
//
////ϵ������
//template <typename T, size_t N>
//void updateCoefficients(const std::vector<float>& input_signal, Node<T, N>& root,
//    const DiscreteWavelet& wavelet,size_t output_len,MODE mode) {
//    size_t data_size = input_signal.size();
//
//    size_t output_len = dwt_buffer_length(data_size, wavelet.dec_len, mode);
//
//    if (input_signal.size() < 2 * N) {
//        throw std::runtime_error("Input signal is too short for the given filter length.");
//    }
//
//    size_t step = 2; // ����
//    std::stack<Node<T, N>*> node_stack; // ջ�洢�ڵ�ָ��
//
//    // ��ʼ�����ڵ�
//    memmove(root.getInputBuffer(), root.getInputBuffer() + 1, (2 * N - 1) * sizeof(T));
//    root.getInputBuffer()[2 * N - 1] = input_signal[0]; // ��һ�������������ڻ�����ĩβ
//    root.setStartIndex(root.getStartIndex() + 1);
//
//    // �����ڵ���ջ
//    node_stack.push(&root);
//
//    // ����������
//    while (!node_stack.empty()) {
//        // ��ջ��ǰ�ڵ�
//        Node<T, N>* current_node = node_stack.top();
//        node_stack.pop();
//
//        // ������������
//        if (current_node->getStartIndex() < 0) {
//            continue;
//        }
//
//        // ���������ͻ����ƶ���־
//        current_node->setStartIndex(current_node->getStartIndex() % step);
//        bool shift_buffer = (current_node->getStartIndex() == 1);
//
//        if (current_node->getParent()) {
//            shift_buffer = current_node->getParent()->getShiftBuffer() && shift_buffer;
//        }
//        current_node->setShiftBuffer(shift_buffer);
//
//        // �����Ҷ�ӽڵ㣬����С��ϵ��
//        if (current_node->isLeaf()) {
//            // ����� N ������ִ��ȥ�루�˴��޲���������ʵ�֣�
//            // �����ڴ˴����ȥ���߼�
//            continue;
//        }
//
//        // �Ե�ǰ�ڵ���ӽڵ���д���
//        Node<T, N>* children[] = { current_node->getRight(), current_node->getLeft() }; // ϸ������
//        for (Node<T, N>* child : children) {
//            if (!child) continue; // �ӽڵ�Ϊ��������
//
//            if (shift_buffer) {
//                child->setStartIndex(child->getStartIndex() + 1);
//                memmove(child->getInputBuffer(), child->getInputBuffer() + 1, (N - 1) * sizeof(T));
//            }
//
//            // ����С���ֽ⺯��
//            if (child->getType() == NodeType::APPROX) {
//                CAT(float, _dec_a)(current_node->getInputBuffer() + current_node->getStartIndex(), 2 * N - current_node->getStartIndex(), wavelet, child->getInputBuffer() + N, output_len, mode);
//            }
//            else if (child->getType() == NodeType::DETAIL) {
//                CAT(float, _dec_d)(current_node->getInputBuffer() + current_node->getStartIndex(), 2 * N - current_node->getStartIndex(), wavelet, child->getInputBuffer() + N, output_len, mode);
//            }
//
//            // ����ӽڵ㻺���Ѿ����Դ���������ջ
//            if (child->getStartIndex() >= 0) {
//                node_stack.push(child);
//            }
//        }
//    }
//}
