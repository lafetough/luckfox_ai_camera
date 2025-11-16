#include "rknn_interface.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <numeric>

// ============ Вспомогательные функции ============

std::string RKNNInference::GetFormatString(rknn_tensor_format fmt) {
    switch (fmt) {
    case RKNN_TENSOR_NCHW: return "NCHW";
    case RKNN_TENSOR_NHWC: return "NHWC";
    case RKNN_TENSOR_NC1HWC2: return "NC1HWC2";
    default: return "Unknown";
    }
}

std::string RKNNInference::GetTypeString(rknn_tensor_type type) {
    switch (type) {
    case RKNN_TENSOR_UINT8: return "uint8";
    case RKNN_TENSOR_INT8: return "int8";
    case RKNN_TENSOR_INT16: return "int16";
    case RKNN_TENSOR_INT32: return "int32";
    case RKNN_TENSOR_INT64: return "int64";
    case RKNN_TENSOR_FLOAT16: return "float16";
    case RKNN_TENSOR_FLOAT32: return "float32";
    default: return "Unknown";
    }
}

std::string RKNNInference::GetQntTypeString(rknn_tensor_qnt_type qnt_type) {
    switch (qnt_type) {
    case RKNN_TENSOR_QNT_NONE: return "NONE";
    case RKNN_TENSOR_QNT_DFP: return "DFP";
    case RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC: return "AFFINE_ASYMMETRIC";
    default: return "Unknown";
    }
}

// ============ RKNNInference реализация ============

RKNNInference::RKNNInference() {
    memset(&m_ctx, 0, sizeof(RKNNContext));
}

RKNNInference::~RKNNInference() {
    if (m_ctx.initialized) {
        Deinit();
    }
}

int RKNNInference::Init(const std::string& model_path) {
    if (m_ctx.initialized) {
        printf("RKNN: Model already initialized\n");
        return -1;
    }

    int ret = 0;

    // Инициализация контекста RKNN
    ret = rknn_init(&m_ctx.ctx, (char*)model_path.c_str(), 0, 0, NULL);
    if (ret < 0) {
        printf("RKNN: rknn_init failed! ret=%d\n", ret);
        return -1;
    }

    // Получение информации о модели
    ret = QueryModelInfo();
    if (ret < 0) {
        printf("RKNN: Failed to query model info\n");
        rknn_destroy(m_ctx.ctx);
        m_ctx.ctx = 0;
        return -1;
    }

    // Инициализация памяти для входов/выходов
    ret = SetupIOMemory();
    if (ret < 0) {
        printf("RKNN: Failed to setup IO memory\n");
        rknn_destroy(m_ctx.ctx);
        m_ctx.ctx = 0;
        return -1;
    }

    m_ctx.initialized = true;

    printf("RKNN: Model initialized successfully\n");
    printf("RKNN: Inputs: %d, Outputs: %d\n", m_ctx.n_inputs, m_ctx.n_outputs);
    printf("RKNN: Quantized: %s\n", m_ctx.is_quantized ? "yes" : "no");

    return 0;
}

int RKNNInference::QueryModelInfo() {
    int ret = 0;

    // Получение количества входов/выходов
    rknn_input_output_num io_num;
    ret = rknn_query(m_ctx.ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        printf("RKNN: rknn_query IN_OUT_NUM failed! ret=%d\n", ret);
        return -1;
    }

    m_ctx.n_inputs = io_num.n_input;
    m_ctx.n_outputs = io_num.n_output;

    // Получение информации о входах
    m_ctx.input_attrs.resize(m_ctx.n_inputs);
    for (int i = 0; i < m_ctx.n_inputs; i++) {
        memset(&m_ctx.input_attrs[i], 0, sizeof(rknn_tensor_attr));
        m_ctx.input_attrs[i].index = i;

        ret = rknn_query(m_ctx.ctx, RKNN_QUERY_NATIVE_INPUT_ATTR, &m_ctx.input_attrs[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("RKNN: Failed to query input %d\n", i);
            return -1;
        }

        TensorInfo info = QueryTensorInfo(&m_ctx.input_attrs[i], true);
        m_ctx.input_infos.push_back(info);

        printf("RKNN: Input[%d]: %s, shape=[%d,%d,%d,%d], fmt=%s, type=%s\n",
               i, m_ctx.input_attrs[i].name, m_ctx.input_attrs[i].dims[0], m_ctx.input_attrs[i].dims[1],
               m_ctx.input_attrs[i].dims[2], m_ctx.input_attrs[i].dims[3],
               GetFormatString(m_ctx.input_attrs[i].fmt).c_str(), GetTypeString(m_ctx.input_attrs[i].type).c_str());
    }

    // Получение информации о выходах
    m_ctx.output_attrs.resize(m_ctx.n_outputs);
    for (int i = 0; i < m_ctx.n_outputs; i++) {
        memset(&m_ctx.output_attrs[i], 0, sizeof(rknn_tensor_attr));
        m_ctx.output_attrs[i].index = i;

        ret = rknn_query(m_ctx.ctx, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &m_ctx.output_attrs[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            printf("RKNN: Failed to query output %d\n", i);
            return -1;
        }

        TensorInfo info = QueryTensorInfo(&m_ctx.output_attrs[i], false);
        m_ctx.output_infos.push_back(info);

        printf("RKNN: Output[%d]: %s, shape=[%d,%d,%d,%d], fmt=%s, type=%s, qnt=%s\n",
               i, m_ctx.output_attrs[i].name, m_ctx.output_attrs[i].dims[0], m_ctx.output_attrs[i].dims[1],
               m_ctx.output_attrs[i].dims[2], m_ctx.output_attrs[i].dims[3],
               GetFormatString(m_ctx.output_attrs[i].fmt).c_str(), GetTypeString(m_ctx.output_attrs[i].type).c_str(),
               GetQntTypeString(m_ctx.output_attrs[i].qnt_type).c_str());
    }

    // Определение квантизации по первому выходу
    if (m_ctx.output_infos.size() > 0) {
        m_ctx.is_quantized = (m_ctx.output_infos[0].qnt_type == QuantizationType::AFFINE_ASYMMETRIC);
    }

    return 0;
}

TensorInfo RKNNInference::QueryTensorInfo(const rknn_tensor_attr* attr, bool is_input) {
    TensorInfo info;
    info.index = attr->index;
    info.name = attr->name;
    info.n_dims = attr->n_dims;
    info.n_elems = attr->n_elems;
    info.size = attr->size;
    info.size_with_stride = attr->size_with_stride;
    info.fmt = (TensorFormat)attr->fmt;
    info.type = (TensorType)attr->type;
    info.qnt_type = (QuantizationType)attr->qnt_type;
    info.zp = attr->zp;
    info.scale = attr->scale;

    for (int i = 0; i < 4; i++) {
        info.dims[i] = attr->dims[i];
    }

    return info;
}

int RKNNInference::SetupIOMemory() {
    int ret = 0;

    // Выделение памяти для входов
    m_ctx.input_mems.resize(m_ctx.n_inputs);
    for (int i = 0; i < m_ctx.n_inputs; i++) {
        m_ctx.input_mems[i] = rknn_create_mem(m_ctx.ctx, m_ctx.input_infos[i].size_with_stride);
        if (!m_ctx.input_mems[i]) {
            printf("RKNN: Failed to allocate input memory %d\n", i);
            return -1;
        }

        // ИСПРАВЛЕНИЕ: Передаём указатель на rknn_tensor_attr из input_attrs вектора
        ret = rknn_set_io_mem(m_ctx.ctx, m_ctx.input_mems[i], &m_ctx.input_attrs[i]);
        if (ret < 0) {
            printf("RKNN: Failed to set input memory %d\n", i);
            return -1;
        }
    }

    // Выделение памяти для выходов
    m_ctx.output_mems.resize(m_ctx.n_outputs);
    for (int i = 0; i < m_ctx.n_outputs; i++) {
        m_ctx.output_mems[i] = rknn_create_mem(m_ctx.ctx, m_ctx.output_infos[i].size_with_stride);
        if (!m_ctx.output_mems[i]) {
            printf("RKNN: Failed to allocate output memory %d\n", i);
            return -1;
        }

        // ИСПРАВЛЕНИЕ: Передаём указатель на rknn_tensor_attr из output_attrs вектора
        ret = rknn_set_io_mem(m_ctx.ctx, m_ctx.output_mems[i], &m_ctx.output_attrs[i]);
        if (ret < 0) {
            printf("RKNN: Failed to set output memory %d\n", i);
            return -1;
        }
    }

    return 0;
}

int RKNNInference::CleanupIOMemory() {
    for (int i = 0; i < m_ctx.n_inputs; i++) {
        if (m_ctx.input_mems[i]) {
            rknn_destroy_mem(m_ctx.ctx, m_ctx.input_mems[i]);
            m_ctx.input_mems[i] = nullptr;
        }
    }

    for (int i = 0; i < m_ctx.n_outputs; i++) {
        if (m_ctx.output_mems[i]) {
            rknn_destroy_mem(m_ctx.ctx, m_ctx.output_mems[i]);
            m_ctx.output_mems[i] = nullptr;
        }
    }

    return 0;
}

int RKNNInference::Deinit() {
    if (!m_ctx.initialized) {
        return 0;
    }

    CleanupIOMemory();
    m_ctx.input_mems.clear();
    m_ctx.output_mems.clear();
    m_ctx.input_infos.clear();
    m_ctx.output_infos.clear();
    m_ctx.input_attrs.clear();
    m_ctx.output_attrs.clear();

    if (m_ctx.ctx) {
        rknn_destroy(m_ctx.ctx);
        m_ctx.ctx = 0;
    }

    m_ctx.initialized = false;
    printf("RKNN: Model deinitialized\n");

    return 0;
}

int RKNNInference::SetInput(int input_index, const uint8_t* input_data, size_t size) {
    if (!m_ctx.initialized) {
        printf("RKNN: Model not initialized\n");
        return -1;
    }

    if (input_index < 0 || input_index >= m_ctx.n_inputs) {
        printf("RKNN: Invalid input index: %d\n", input_index);
        return -1;
    }

    if (!m_ctx.input_mems[input_index]) {
        printf("RKNN: Input memory not allocated\n");
        return -1;
    }

    size_t expected_size = m_ctx.input_infos[input_index].size_with_stride;
    if (size != expected_size) {
        printf("RKNN: Input size mismatch: expected %zu, got %zu\n", expected_size, size);
        return -1;
    }

    void* input_addr = m_ctx.input_mems[input_index]->virt_addr;
    memcpy(input_addr, input_data, size);

    return 0;
}

int RKNNInference::Run() {
    if (!m_ctx.initialized) {
        printf("RKNN: Model not initialized\n");
        return -1;
    }

    int ret = rknn_run(m_ctx.ctx, nullptr);
    if (ret < 0) {
        printf("RKNN: rknn_run failed! ret=%d\n", ret);
        return -1;
    }

    return 0;
}

int RKNNInference::GetOutput(int output_index, uint8_t* output_data, size_t size) {
    if (!m_ctx.initialized) {
        printf("RKNN: Model not initialized\n");
        return -1;
    }

    if (output_index < 0 || output_index >= m_ctx.n_outputs) {
        printf("RKNN: Invalid output index: %d\n", output_index);
        return -1;
    }

    if (!m_ctx.output_mems[output_index]) {
        printf("RKNN: Output memory not allocated\n");
        return -1;
    }

    size_t output_size = m_ctx.output_infos[output_index].size;
    size_t copy_size = (size < output_size) ? size : output_size;

    void* output_addr = m_ctx.output_mems[output_index]->virt_addr;
    memcpy(output_data, output_addr, copy_size);

    return (int)copy_size;
}

const void* RKNNInference::GetOutputPtr(int output_index) {
    if (!m_ctx.initialized || output_index < 0 || output_index >= m_ctx.n_outputs) {
        return nullptr;
    }

    if (!m_ctx.output_mems[output_index]) {
        return nullptr;
    }

    return m_ctx.output_mems[output_index]->virt_addr;
}

int RKNNInference::GetOutputSize(int output_index) const {
    if (output_index < 0 || output_index >= m_ctx.n_outputs) {
        return -1;
    }

    return m_ctx.output_infos[output_index].size;
}

const TensorInfo& RKNNInference::GetInputInfo(int index) const {
    static TensorInfo dummy = {};
    if (index < 0 || index >= m_ctx.n_inputs) {
        printf("RKNN: Invalid input index: %d\n", index);
        return dummy;
    }
    return m_ctx.input_infos[index];
}

const TensorInfo& RKNNInference::GetOutputInfo(int index) const {
    static TensorInfo dummy = {};
    if (index < 0 || index >= m_ctx.n_outputs) {
        printf("RKNN: Invalid output index: %d\n", index);
        return dummy;
    }
    return m_ctx.output_infos[index];
}

QuantizationType RKNNInference::GetOutputQuantizationType(int output_index) const {
    if (output_index < 0 || output_index >= m_ctx.n_outputs) {
        return QuantizationType::NONE;
    }
    return m_ctx.output_infos[output_index].qnt_type;
}

void RKNNInference::GetQuantizationParams(int output_index, int32_t& zp, float& scale) const {
    if (output_index >= 0 && output_index < m_ctx.n_outputs) {
        zp = m_ctx.output_infos[output_index].zp;
        scale = m_ctx.output_infos[output_index].scale;
    } else {
        zp = 0;
        scale = 1.0f;
    }
}

// ============ RKNNOutputProcessor реализация ============

std::vector<float> RKNNOutputProcessor::GetOutputAsFloat(RKNNInference& inference, int output_index) {
    std::vector<float> result;

    const TensorInfo& info = inference.GetOutputInfo(output_index);
    const void* output_ptr = inference.GetOutputPtr(output_index);

    if (!output_ptr) {
        return result;
    }

    int n_elems = info.n_elems;
    result.resize(n_elems);

    if (info.type == TensorType::FLOAT32) {
        memcpy(result.data(), output_ptr, n_elems * sizeof(float));
    } else if (info.type == TensorType::INT8 || info.type == TensorType::UINT8) {
        // Декватизация
        int32_t zp = info.zp;
        float scale = info.scale;

        if (info.type == TensorType::INT8) {
            const int8_t* data = (const int8_t*)output_ptr;
            for (int i = 0; i < n_elems; i++) {
                result[i] = RKNNInference::Dequantize(data[i], zp, scale);
            }
        } else {
            const uint8_t* data = (const uint8_t*)output_ptr;
            for (int i = 0; i < n_elems; i++) {
                result[i] = RKNNInference::Dequantize((int8_t)data[i], zp, scale);
            }
        }
    }

    return result;
}

std::vector<int8_t> RKNNOutputProcessor::GetOutputAsInt8(RKNNInference& inference, int output_index) {
    std::vector<int8_t> result;

    const TensorInfo& info = inference.GetOutputInfo(output_index);
    const void* output_ptr = inference.GetOutputPtr(output_index);

    if (!output_ptr) {
        return result;
    }

    int n_elems = info.n_elems;
    result.resize(n_elems);

    memcpy(result.data(), output_ptr, n_elems * sizeof(int8_t));

    return result;
}

std::vector<float> RKNNOutputProcessor::ApplySoftmax(const std::vector<float>& logits) {
    std::vector<float> result(logits.size());

    // Найти максимум для численной стабильности
    float max_logit = *std::max_element(logits.begin(), logits.end());

    // Вычислить экспоненты
    std::vector<float> exps(logits.size());
    float sum_exp = 0.0f;

    for (size_t i = 0; i < logits.size(); i++) {
        exps[i] = std::exp(logits[i] - max_logit);
        sum_exp += exps[i];
    }

    // Нормализовать
    for (size_t i = 0; i < logits.size(); i++) {
        result[i] = exps[i] / sum_exp;
    }

    return result;
}

std::vector<std::pair<int, float>> RKNNOutputProcessor::GetTopK(
    const std::vector<float>& scores, int k) {

    std::vector<std::pair<int, float>> indexed_scores;
    for (size_t i = 0; i < scores.size(); i++) {
        indexed_scores.push_back({i, scores[i]});
    }

    // Сортировка по убыванию оценок
    std::sort(indexed_scores.begin(), indexed_scores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Возвращение top-k
    std::vector<std::pair<int, float>> result;
    for (int i = 0; i < std::min(k, (int)indexed_scores.size()); i++) {
        result.push_back(indexed_scores[i]);
    }

    return result;
}
