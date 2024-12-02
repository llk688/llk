#include <iostream>
#include <vector>
#include <stdexcept>
#include "_dwt.cpp"
#include "wpt_node.h"
//#include "wpt_node.cpp"

void test_dwt() {
    // ���������������
    vector<float> input_data = { -0.45247328,
-0.051487215,
-0.13417718,
0.19155322,
-0.44609484,
0.12364525,
-0.1288879,
0.19040743,
-0.009572665,
0.07659797,
0.06149954,
0.22331221,
0.023448698,
-0.003031011,
0.06290513,
0.32736248,

    };
    //0.05390203,
        //0.018119449,
      //  0.11614434,
       // -0.01914533,
    cout << "Input Data: ";
    for (float val : input_data) cout << val << " ";
    cout << endl;

    // ����һ�� db4 С��
    DiscreteWavelet* wavelet = discrete_wavelet(DB, 4);
    MODE mode = MODE_REFLECT;
    size_t data_size = input_data.size();


    size_t output_len = dwt_buffer_length(data_size, wavelet->dec_len, mode);



    //// ִ��С���任
    pair<vector<float>, vector<float>> result = dwt_single(input_data, *wavelet, mode);
    vector<float> cA = result.first;
    vector<float> cD = result.second;

    cout << "Low-frequency coefficients (cA): ";
    for (float val : cA) cout << val << " ";
    cout << endl;

    cout << "High-frequency coefficients (cD): ";
    for (float val : cD) cout << val << " ";
    cout << endl;

    ////ִ��С����任
    //vector<float> reconstructed_data = idwt_single(cA, cD, *wavelet, mode);

    //// �������Ƿ�С�� 1e-7
    //double max_error = 0.0;
    //for (size_t i = 0; i < input_data.size(); ++i) {
    //    double error = abs(input_data[i] - reconstructed_data[i]);
    //    max_error = max(max_error, error);
    //}

    //// ������
    //cout << "Input Data: ";
    //for (float val : input_data) cout << val << " ";
    //cout << "\n";

    //cout << "Reconstructed Data: ";
    //for (float val : reconstructed_data) cout << val << " ";
    //cout << "\n";

    //cout << "Max Error: " << max_error << "\n";

    //// �ж�����Ƿ�������Χ��
    //if (max_error < 1e-7) {
    //    cout << "Reconstruction successful: Error within acceptable range." << endl;
    //}
    //else {
    //    cerr << "Reconstruction failed: Error exceeds acceptable range." << endl;
    //}


    pair<vector<vector<float>>, vector<vector<float>>> result1 = dwt_three_levels(input_data, *wavelet, mode);
    vector<vector<float>> cA1 = result1.first;
    vector<vector<float>> cD1 = result1.second;

    for (size_t level = 0; level < cA1.size(); ++level) {
        cout << "Low-frequency coefficients at level " << level + 1 << " (cA1): ";
        for (size_t i = 0; i < cA1[level].size(); ++i) {
            cout << cA1[level][i] << " ";
        }
        cout << endl;
    }

    for (size_t level = 0; level < cD1.size(); ++level) {
        cout << "High-frequency coefficients at level " << level + 1 << " (cD1): ";
        for (size_t i = 0; i < cD1[level].size(); ++i) {
            cout << cD1[level][i] << " ";
        }
        cout << endl;
    }

    // ִ������С����任
    /*vector<float> reconstructed_data1 = idwt_three_levels(cA1, cD1, *wavelet, mode);

    cout << "Reconstructed Data1: ";
    for (float val : reconstructed_data1) cout << val << " ";
    cout << endl;*/
    





    size_t N = wavelet->dec_len;
    // С���任����
    int levels = 5;

    // ���� Wavelet Packet Tree
    WaveletPacketTree<float> wpt(N, levels);
   
    Node<float>* root = new(std::nothrow) Node<float>("", NodeType::INTERMEDIATE, false);
    root->setStartIndex(-2);
    string path1 = "aaada";
    string path2 = "addad";
    wpt.buildTree(root,N,path1);
    wpt.buildTree(root, N, path2);
    

    std::cout << "Wavelet Packet Tree structure:\n";




    // ִ��ϵ������
    for (size_t i = 0; i < input_data.size(); ++i) {
        std::cout << "Processing sample: " << input_data[i] << std::endl;
        updateCoefficients<float>(input_data[i], *root,wpt, wavelet, mode);
        // ��ӡ���ṹ�Թ۲�ʵʱ�仯
        wpt.printTree(root);
        vector<float> reconstructed_data(N*2,0.0f);
        reconstructCoefficients<float>(root, wavelet, mode, true, &reconstructed_data);
        cout << " reconstructed_data:\n";
        for (float val : reconstructed_data) cout << val << " ";
        cout << endl; 
        cout << " reconstructed_tree:\n";
        wpt.printTree(root);
    }
    return;
}
int main() {
    MODE mode = MODE_REFLECT;
    test_dwt();
    return 0;
}
