extern "C"
{
#include <quickjs.h>
}
#include <string>
#include <optional>
#include <thread>

class QuickJsEngine
{
private:
    JSRuntime *rt = nullptr;
    JSContext *ctx = nullptr;
    std::string errorInit;
    std::optional<bool> isInit;

    static void processingPromiseRejection(JSContext* ctx, JSValueConst promise, JSValueConst reason, JS_BOOL is_handled, void* opaque) {
        if (!is_handled) {
            JSException ex = {};
            const char* error_msg = JS_ToCString(ctx, reason);
            if (error_msg) {
                ex.error = std::string(error_msg);
                JS_FreeCString(ctx, error_msg);
            }

            auto stack = assistant::js::to_string(ctx, reason, "stack");
            if (!ex.error.empty()) {
                ex.stack = stack.value_or("");
                assistant::js::exceptions->set(ex);
            }
        }
    }

public:
    /*** Инициализировать движок */
    bool init() {
        if (isInit.has_value()) {
            return isInit.value();
        }

        rt = JS_NewRuntime();
        if (!rt) {
            errorInit = "Ошибка: Не удалось создать JSRuntime";
            isInit = false;
            return false;
        }

        ctx = JS_NewContext(rt);
        if (!ctx) {
            JS_FreeRuntime(rt);
            errorInit = "Ошибка: Не удалось создать JSContext";
            isInit = false;
            return false;
        }

        JS_SetHostPromiseRejectionTracker(JS_GetRuntime(ctx), processingPromiseRejection, nullptr);
        isInit = true;
        return true;
    }

    /** Получить текст ошибки при инициализации */
    std::string getErrorInit()
    {
        return errorInit;
    }

    /*** Освободить ресурсы */
    void free() {
        if (ctx) {
            JS_FreeContext(ctx);
        }
        ctx = nullptr;

        if (rt) {
            JS_FreeRuntime(rt);
        }

        rt = nullptr;
    }

    /** Выполнить скрипт из файла */
    void evalFile(std::string fileName) {
        assistant::js::evalFile(ctx, fileName);
    }

    void registerFn(std::string objectName, std::string functionName, JSCFunction *func) {
        assistant::js::registerFn(ctx, objectName, functionName, func);
    }

    /** Регистрация функции, которая возвращает промис, для объекта ( например app.otherObject.functionName ) */
    void registerPromiseInObject(std::string objectName, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)> &fn) {
        assistant::js::registerPromiseInObject(ctx, objectName, functionName, fn);
    }

    /** Регистрация функции, которая возвращает промис, для класса ( портотип ) */
    void registerPromiseInClass(std::string objectName, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)> &fn) {
        assistant::js::registerPromiseInClass(ctx, objectName, functionName, fn);
    }

    /** Зарегистрировать портатип для класса */
    void registerPrototype(std::string className, std::string functionName, JSCFunction *func) {
        assistant::js::registerPrototype(ctx, className, functionName, func);
    }

    /** Установить данные для контекста */
    void setContextOpaque(void *opaque) {
        JS_SetContextOpaque(ctx, opaque);
    }

    /** Получить ранее установленные данные из контекста */
    template <typename T>
    T *getContextOpaque()
    {
        return QuickJsEngine::getContextOpaque<T>(this->ctx);
    }

    /** Получить ранее установленные данные из контекста */
    template <typename T>
    static T *getContextOpaque(JSContext *ctx)
    {
        // Проверка на этапе компиляции: T должен быть классом или структурой, а не числом
        static_assert(std::is_class<T>::value, "Шаблонный параметр T должен быть классом или структурой!");

        if (!ctx)
        {
            return nullptr;
        }

        void *opaque = JS_GetContextOpaque(ctx);
        return reinterpret_cast<T *>(opaque);
    }

    /** Получить строку */
    std::optional<std::string> to_string(JSValue value) {
        return assistant::js::to_string(ctx, value);
    }

    /** Получить целочисленное значение */
    std::optional<int> to_int(JSValue value) {
        return assistant::js::to_int(ctx, value);
    }

    /** Получить булевное значение */
    std::optional<bool> to_bool(JSValue value) {
        return assistant::js::to_bool(ctx, value);
    }

    /** Получить имя класса */
    std::string getClassName(JSValue value) {
        return assistant::js::getClassName(ctx, value);
    }

    JSContext* getCtx() {
        return this->ctx;
    }
};
