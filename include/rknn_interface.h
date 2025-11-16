#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <map>
#include <string>
#include "rknn_api.h"

/**
 * Универсальный интерфейс для работы с RKNN моделями
 * Поддерживает любые типы задач: детекция, классификация, поза, распознавание и т.д.
 */

// ============ Типы данных ============

/** Перечисление типов тензоров */
enum class TensorType {
    UINT8 = RKNN_TENSOR_UINT8,
    INT8 = RKNN_TENSOR_INT8,
    INT16 = RKNN_TENSOR_INT16,
    INT32 = RKNN_TENSOR_INT32,
    INT64 = RKNN_TENSOR_INT64,
    FLOAT16 = RKNN_TENSOR_FLOAT16,
    FLOAT32 = RKNN_TENSOR_FLOAT32
};

/** Перечисление форматов тензоров */
enum class TensorFormat {
    NCHW = RKNN_TENSOR_NCHW,
    NHWC = RKNN_TENSOR_NHWC,
    NC1HWC2 = RKNN_TENSOR_NC1HWC2
};

/** Перечисление типов квантизации */
enum class QuantizationType {
    NONE = RKNN_TENSOR_QNT_NONE,
    DFP = RKNN_TENSOR_QNT_DFP,
    AFFINE_ASYMMETRIC = RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC
};

/**
 * Структура описания тензора
 */
struct TensorInfo {
    int index;
    std::string name;
    int n_dims;
    int dims[4];
    int n_elems;
    int size;
    int size_with_stride;
    TensorFormat fmt;
    TensorType type;
    QuantizationType qnt_type;
    int32_t zp;
    float scale;
};

/**
 * Структура для хранения атрибута тензора RKNN
 */
struct RKNNTensorAttr {
    rknn_tensor_attr attr;
};

/**
 * Структура для хранения данных тензора
 */
struct TensorData {
    void* virt_addr;         // Виртуальный адрес
    int size;                // Размер данных
    TensorInfo info;         // Информация о тензоре
    bool is_owned;           // Владеем ли мы памятью
};

/**
 * Контекст для работы с RKNN моделью
 */
struct RKNNContext {
    rknn_context ctx;

    // Информация о модели
    int n_inputs;
    int n_outputs;
    std::vector<TensorInfo> input_infos;
    std::vector<TensorInfo> output_infos;

    // Сырые атрибуты для rknn_set_io_mem
    std::vector<rknn_tensor_attr> input_attrs;
    std::vector<rknn_tensor_attr> output_attrs;

    // Память для входов/выходов
    std::vector<rknn_tensor_mem*> input_mems;
    std::vector<rknn_tensor_mem*> output_mems;

    // Кэшированные данные
    bool is_quantized;
    bool initialized;
};

// ============ Основной класс интерфейса ============

/**
 * Универсальный RKNN интерфейс для работы с любыми типами моделей
 */
class RKNNInference {
public:
    RKNNInference();
    ~RKNNInference();

    /**
     * Инициализация модели
     * @param model_path Путь к файлу модели (.rknn)
     * @return 0 при успехе, < 0 при ошибке
     */
    int Init(const std::string& model_path);

    /**
     * Освобождение ресурсов
     * @return 0 при успехе, < 0 при ошибке
     */
    int Deinit();

    /**
     * Установка входных данных для всех входов
     * @param input_index Индекс входа (0 для первого входа)
     * @param input_data Указатель на данные
     * @param size Размер данных в байтах
     * @return 0 при успехе, < 0 при ошибке
     */
    int SetInput(int input_index, const uint8_t* input_data, size_t size);

    /**
     * Установка входных данных для первого входа (удобство)
     */
    int SetInput(const uint8_t* input_data, size_t size) {
        return SetInput(0, input_data, size);
    }

    /**
     * Выполнение инференса
     * @return 0 при успехе, < 0 при ошибке
     */
    int Run();

    /**
     * Получение выходных данных
     * @param output_index Индекс выхода (0 для первого выхода)
     * @param output_data Буфер для выходных данных
     * @param size Размер буфера в байтах
     * @return Количество скопированных байт, < 0 при ошибке
     */
    int GetOutput(int output_index, uint8_t* output_data, size_t size);

    /**
     * Получение информации о выходе без копирования
     * Возвращает прямой доступ к памяти модели (осторожно!)
     */
    const void* GetOutputPtr(int output_index);

    /**
     * Получение размера выходных данных
     */
    int GetOutputSize(int output_index) const;

    // ============ Информационные методы ============

    /**
     * Получение количества входов
     */
    int GetInputCount() const { return m_ctx.n_inputs; }

    /**
     * Получение количества выходов
     */
    int GetOutputCount() const { return m_ctx.n_outputs; }

    /**
     * Получение информации о входе
     */
    const TensorInfo& GetInputInfo(int index) const;

    /**
     * Получение информации о выходе
     */
    const TensorInfo& GetOutputInfo(int index) const;

    /**
     * Получение информации о первом входе (удобство)
     */
    const TensorInfo& GetInputInfo() const { return GetInputInfo(0); }

    /**
     * Получение информации о первом выходе (удобство)
     */
    const TensorInfo& GetOutputInfo() const { return GetOutputInfo(0); }

    /**
     * Проверка, квантизирована ли модель
     */
    bool IsQuantized() const { return m_ctx.is_quantized; }

    /**
     * Получение тип квантизации для выхода
     */
    QuantizationType GetOutputQuantizationType(int output_index) const;

    /**
     * Получение параметров квантизации (zero point и scale)
     */
    void GetQuantizationParams(int output_index, int32_t& zp, float& scale) const;

    // ============ Утилиты для работы с данными ============

    /**
     * Конвертация квантизованных значений в float
     * @param qnt_val Квантизованное значение
     * @param zp Zero point параметр квантизации
     * @param scale Scale параметр квантизации
     */
    static float Dequantize(int8_t qnt_val, int32_t zp, float scale) {
        return ((float)qnt_val - (float)zp) * scale;
    }

    /**
     * Конвертация float в квантизованное значение
     */
    static int8_t Quantize(float val, int32_t zp, float scale) {
        float dst_val = (val / scale) + zp;
        return (int8_t)(dst_val <= -128 ? -128 : (dst_val >= 127 ? 127 : dst_val));
    }

    /**
     * Проверка инициализации
     */
    bool IsInitialized() const { return m_ctx.initialized; }

    /**
     * Получение прямого доступа к контексту RKNN (для продвинутого использования)
     */
    RKNNContext& GetContext() { return m_ctx; }
    const RKNNContext& GetContext() const { return m_ctx; }

private:
    RKNNContext m_ctx;

    /**
     * Внутренние методы
     */
    int QueryModelInfo();
    int SetupIOMemory();
    int CleanupIOMemory();
    TensorInfo QueryTensorInfo(const rknn_tensor_attr* attr, bool is_input);

    /**
     * Вспомогательные функции для информации о тензорах
     */
    static std::string GetFormatString(rknn_tensor_format fmt);
    static std::string GetTypeString(rknn_tensor_type type);
    static std::string GetQntTypeString(rknn_tensor_qnt_type qnt_type);
};

// ============ Специализированный интерфейс для инструментов постпроцессинга ============

/**
 * Вспомогательный класс для работы с выходными данными
 */
class RKNNOutputProcessor {
public:
    /**
     * Получение выходных данных как float32 массива
     * (автоматическая декватизация если требуется)
     */
    static std::vector<float> GetOutputAsFloat(
        RKNNInference& inference,
        int output_index
        );

    /**
     * Получение выходных данных как int8 массива
     */
    static std::vector<int8_t> GetOutputAsInt8(
        RKNNInference& inference,
        int output_index
        );

    /**
     * Применение Softmax к выходу (для классификации)
     */
    static std::vector<float> ApplySoftmax(
        const std::vector<float>& logits
        );

    /**
     * Получение Top-K классов (для классификации)
     */
    static std::vector<std::pair<int, float>> GetTopK(
        const std::vector<float>& scores,
        int k = 5
        );
};
