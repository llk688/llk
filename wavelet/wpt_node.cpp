//#include <iostream>
//#include <vector>
//#include <stdexcept>
//#include "wavelets.h"
//#include "wpt_node.h"
//#include "common.h"
////#include "wt.template.c"
//
//
////系数更新
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
//    size_t step = 2; // 步长
//    std::stack<Node<T, N>*> node_stack; // 栈存储节点指针
//
//    // 初始化根节点
//    memmove(root.getInputBuffer(), root.getInputBuffer() + 1, (2 * N - 1) * sizeof(T));
//    root.getInputBuffer()[2 * N - 1] = input_signal[0]; // 第一个输入样本放在缓冲区末尾
//    root.setStartIndex(root.getStartIndex() + 1);
//
//    // 将根节点入栈
//    node_stack.push(&root);
//
//    // 遍历整个树
//    while (!node_stack.empty()) {
//        // 出栈当前节点
//        Node<T, N>* current_node = node_stack.top();
//        node_stack.pop();
//
//        // 缓存样本不足
//        if (current_node->getStartIndex() < 0) {
//            continue;
//        }
//
//        // 更新索引和缓存移动标志
//        current_node->setStartIndex(current_node->getStartIndex() % step);
//        bool shift_buffer = (current_node->getStartIndex() == 1);
//
//        if (current_node->getParent()) {
//            shift_buffer = current_node->getParent()->getShiftBuffer() && shift_buffer;
//        }
//        current_node->setShiftBuffer(shift_buffer);
//
//        // 如果是叶子节点，处理小波系数
//        if (current_node->isLeaf()) {
//            // 对最后 N 个数据执行去噪（此处无操作，留空实现）
//            // 可以在此处添加去噪逻辑
//            continue;
//        }
//
//        // 对当前节点的子节点进行处理
//        Node<T, N>* children[] = { current_node->getRight(), current_node->getLeft() }; // 细节优先
//        for (Node<T, N>* child : children) {
//            if (!child) continue; // 子节点为空则跳过
//
//            if (shift_buffer) {
//                child->setStartIndex(child->getStartIndex() + 1);
//                memmove(child->getInputBuffer(), child->getInputBuffer() + 1, (N - 1) * sizeof(T));
//            }
//
//            // 调用小波分解函数
//            if (child->getType() == NodeType::APPROX) {
//                CAT(float, _dec_a)(current_node->getInputBuffer() + current_node->getStartIndex(), 2 * N - current_node->getStartIndex(), wavelet, child->getInputBuffer() + N, output_len, mode);
//            }
//            else if (child->getType() == NodeType::DETAIL) {
//                CAT(float, _dec_d)(current_node->getInputBuffer() + current_node->getStartIndex(), 2 * N - current_node->getStartIndex(), wavelet, child->getInputBuffer() + N, output_len, mode);
//            }
//
//            // 如果子节点缓存已经可以处理，将其入栈
//            if (child->getStartIndex() >= 0) {
//                node_stack.push(child);
//            }
//        }
//    }
//}
