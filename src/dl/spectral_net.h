#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

class Linear {
private:
    std::vector<std::vector<float>> weights;
    std::vector<float> bias;
    int in_features, out_features;

public:
    Linear(int in_features, int out_features) : in_features(in_features), out_features(out_features) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-0.1, 0.1);

        weights.resize(out_features, std::vector<float>(in_features));
        bias.resize(out_features);

        for (auto& row : weights) {
            for (auto& weight : row) {
                weight = dis(gen);
            }
        }

        for (auto& b : bias) {
            b = dis(gen);
        }
    }

    std::vector<float> forward(const std::vector<float>& input) {
        std::vector<float> output(out_features);
        for (int i = 0; i < out_features; ++i) {
            output[i] = bias[i];
            for (int j = 0; j < in_features; ++j) {
                output[i] += weights[i][j] * input[j];
            }
        }
        return output;
    }

    std::vector<float> backward(const std::vector<float>& input, const std::vector<float>& grad_output, float learning_rate) {
        std::vector<float> grad_input(in_features, 0.0f);

        for (int i = 0; i < out_features; ++i) {
            for (int j = 0; j < in_features; ++j) {
                float grad_weight = input[j] * grad_output[i];
                grad_input[j] += weights[i][j] * grad_output[i];
                weights[i][j] -= learning_rate * grad_weight;
            }
            bias[i] -= learning_rate * grad_output[i];
        }

        return grad_input;
    }
    void serializeWeights(std::ofstream& file) const {
        for (const auto& row : weights) {
            for (float weight : row) {
                file << weight << " ";
            }
        }
        for (float b : bias) {
            file << b << " ";
        }
    }

    void loadWeights(std::ifstream& file) {
        for (auto& row : weights) {
            for (float& weight : row) {
                file >> weight;
            }
        }
        for (float& b : bias) {
            file >> b;
        }
    }
};

float relu(float x) {
    return std::max(0.0f, x);
}

float relu_derivative(float x) {
    return x > 0 ? 1.0f : 0.0f;
}

class SpectralNet {
private:
    Linear fc1;
    Linear fc2;
    Linear fc3;
    Linear fc4;
    std::vector<float> h1, h2, h3;

    float mse_loss(const std::vector<float>& predictions, const std::vector<float>& targets) {
        float loss = 0.0f;
        for (size_t i = 0; i < predictions.size(); ++i) {
            float diff = predictions[i] - targets[i];
            loss += diff * diff;
        }
        return loss / predictions.size();
    }

    std::vector<float> mse_loss_grad(const std::vector<float>& predictions, const std::vector<float>& targets) {
        std::vector<float> grad(predictions.size());
        for (size_t i = 0; i < predictions.size(); ++i) {
            grad[i] = 2 * (predictions[i] - targets[i]) / predictions.size();
        }
        return grad;
    }

public:
    SpectralNet() : fc1(3, 64), fc2(64, 128), fc3(128, 256), fc4(256, 31) {}

    std::vector<float> forward(const std::vector<float>& x) {
        h1 = fc1.forward(x);
        std::transform(h1.begin(), h1.end(), h1.begin(), relu);

        h2 = fc2.forward(h1);
        std::transform(h2.begin(), h2.end(), h2.begin(), relu);

        h3 = fc3.forward(h2);
        std::transform(h3.begin(), h3.end(), h3.begin(), relu);

        return fc4.forward(h3);
    }

    void backward(const std::vector<float>& input, const std::vector<float>& grad_output, float learning_rate) {
        auto grad_h3 = fc4.backward(h3, grad_output, learning_rate);
        std::transform(grad_h3.begin(), grad_h3.end(), h3.begin(), grad_h3.begin(), 
                       [](float grad, float h) { return grad * relu_derivative(h); });

        auto grad_h2 = fc3.backward(h2, grad_h3, learning_rate);
        std::transform(grad_h2.begin(), grad_h2.end(), h2.begin(), grad_h2.begin(), 
                       [](float grad, float h) { return grad * relu_derivative(h); });

        auto grad_h1 = fc2.backward(h1, grad_h2, learning_rate);
        std::transform(grad_h1.begin(), grad_h1.end(), h1.begin(), grad_h1.begin(), 
                       [](float grad, float h) { return grad * relu_derivative(h); });

        fc1.backward(input, grad_h1, learning_rate);
    }
    void train(const std::vector<std::vector<float>>& inputs, 
               const std::vector<std::vector<float>>& targets, 
               int epochs, float learning_rate) {
        for (int epoch = 0; epoch < epochs; ++epoch) {
            float total_loss = 0.0f;
            for (size_t i = 0; i < inputs.size(); ++i) {
                auto predictions = forward(inputs[i]);
                float loss = mse_loss(predictions, targets[i]);
                total_loss += loss;

                auto loss_grad = mse_loss_grad(predictions, targets[i]);
                backward(inputs[i], loss_grad, learning_rate);
            }
            std::cout << "Epoch " << epoch + 1 << ", Average Loss: " << total_loss / inputs.size() << std::endl;
        }
    }

    void saveWeights(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file for writing: " + filename);
        }

        fc1.serializeWeights(file);
        fc2.serializeWeights(file);
        fc3.serializeWeights(file);
        fc4.serializeWeights(file);

        file.close();
    }

    void loadWeights(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file for reading: " + filename);
        }

        fc1.loadWeights(file);
        fc2.loadWeights(file);
        fc3.loadWeights(file);
        fc4.loadWeights(file);

        file.close();
    }
};

