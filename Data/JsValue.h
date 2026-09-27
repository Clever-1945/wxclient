#pragma once

#include <string>
#include <memory>
#include <optional>

extern "C"
{
#include <quickjs.h>
}

#define GET_VALUE(index) \
    JsValue::CreateJsValue(ctx, this_val, argc, argv, index)

/** JavaScript значение. Значение должно освобождаться само после выхода из области видимости */
class JsValue {
private:
    JSContext *ctx = nullptr;
    JSValue value = JS_UNDEFINED;
    /** Отключить освобождение */
    bool disableFree = false;

    /** Получить целочисленное значение */
    std::optional<int> to_int(JSValue raw_value) {
        if (JS_IsUndefined(raw_value)) {
            return std::nullopt;
        }

        int result = 0;
        int error = JS_ToInt32(ctx, &result, raw_value);

        if (error < 0) {
            return std::nullopt;
        }

        return result;
    }

    /** Получить строку */
    std::optional<std::string> to_string(JSValue current_value) {
        if (!this->ctx) {
            return std::nullopt;
        }
        if (JS_IsString(current_value)) {
            const char *text = JS_ToCString(ctx, current_value);
            auto str = std::string(text);
            JS_FreeCString(ctx, text);
            return str;
        }

        return std::nullopt;
    }

public:
    JsValue(JSContext *ctx, JSValue value, bool disableFree = false) {
        this->ctx = ctx;
        this->value = value;
        this->disableFree = disableFree;
    }

    JsValue(JSContext *ctx, JSValue value, const std::string &propertyName, bool disableFree = false) {
        this->ctx = ctx;
        this->value = JS_GetPropertyStr(this->ctx, value, propertyName.c_str());
        this->disableFree = disableFree;
    }

    ~JsValue() {
        if (ctx && !JS_IsUndefined(value) && !this->disableFree) {
            JS_FreeValue(ctx, value);
        }
    }

    inline static std::unique_ptr<JsValue> CreateJsValue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int index) {
        if (index < argc) {
            return std::make_unique<JsValue>(ctx,argv[index], true);
        }
        return std::make_unique<JsValue>(ctx, JS_UNDEFINED);
    }

    inline static std::unique_ptr<JsValue> fromValue(JSContext *ctx, JSValue value, bool disableFree = false) {
        return std::make_unique<JsValue>(ctx, value, disableFree);
    }

    JsValue(const JsValue &) = delete;

    JsValue &operator=(const JsValue &) = delete;

    /** Получить значение по ключу в объекте */
    std::unique_ptr<JsValue> getValue(const std::string &propertyName) const {
        if (!this->ctx) {
            return std::make_unique<JsValue>(nullptr, JS_UNDEFINED);
        }
        auto raw_value = JS_GetPropertyStr(this->ctx, this->value, propertyName.c_str());
        return std::make_unique<JsValue>(this->ctx, raw_value);
    }

    /** Получить конфигурацию объекта */
    std::unique_ptr<JsValue> getConfigValue() {
        return this->getValue("config");
    }

    /** Получить оригинальное значение */
    JSValue getRawValue() {
        return this->value;
    }

    /** Получить копию оригинального значения */
    JSValue getRawDupValue() {
        if (!this->ctx) {
            return JS_UNDEFINED;
        }
        if (JS_IsUndefined(this->value)) {
            return JS_UNDEFINED;
        }
        return JS_DupValue(this->ctx, this->value);
    }

    /** Получить строку */
    std::optional<std::string> to_string() {
        if (!this->ctx) {
            return std::nullopt;
        }
        if (JS_IsString(value)) {
            const char *text = JS_ToCString(ctx, value);
            auto str = std::string(text);
            JS_FreeCString(ctx, text);
            return str;
        }

        return std::nullopt;
    }

    /** Вернуть функцию */
    bool is_function() {
        if (!this->ctx) {
            return false;
        }
        return JS_IsFunction(ctx, value);
    }

    /** Получить целочисленное значение */
    std::optional<int> to_int() {
        if (!this->ctx) {
            return std::nullopt;
        }
        return to_int(this->value);
    }

    /** Получить целочисленное значение */
    std::optional<int64_t> to_int_64() {
        if (!this->ctx) {
            return std::nullopt;
        }
        if (JS_IsUndefined(value)) {
            return std::nullopt;
        }

        int64_t result = 0;
        int error = JS_ToInt64(ctx, &result, value);

        if (error < 0) {
            return std::nullopt;
        }

        return result;
    }

    /** Получить булевное значение */
    std::optional<bool> to_bool() {
        if (!this->ctx) {
            return std::nullopt;
        }
        if (JS_IsBool(value)) {
            return JS_ToBool(ctx, value) != 0 ? true : false;
        }
        return std::nullopt;
    }

    /** Получить дробное число */
    std::optional<double> to_double() {
        if (!this->ctx) {
            return std::nullopt;
        }
        if (JS_IsUndefined(value)) {
            return std::nullopt;
        }

        double result = 0;
        int error = JS_ToFloat64(ctx, &result, value);
        if (error < 0) {
            return std::nullopt;
        }

        return result;
    }

    /** Преобразовать JS объект в JSON строку */
    std::optional<std::string> to_json() {
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue json_obj = JS_GetPropertyStr(ctx, global_obj, "JSON");
        JSValue stringify_fn = JS_GetPropertyStr(ctx, json_obj, "stringify");

        JSValue argv[3];
        argv[0] = value;
        argv[1] = JS_NULL;
        argv[2] = JS_NewInt32(ctx, 4);

        JSValue json_js_string = JS_Call(ctx, stringify_fn, json_obj, 3, argv);
        auto text_json = to_string(json_js_string);

        JS_FreeValue(ctx, argv[2]);
        JS_FreeValue(ctx, global_obj);
        JS_FreeValue(ctx, json_obj);
        JS_FreeValue(ctx, stringify_fn);
        JS_FreeValue(ctx, json_js_string);

        return text_json;
    }

    /** Факт того, что значение является объектом */
    bool isObject() {
        return JS_IsObject(this->value);
    }

    /** Значение не определено */
    bool isUndefined() {
        return JS_IsUndefined(this->value);
    }

    /** Факт того что значение null */
    bool isNull() {
        return JS_IsNull(this->value);
    }

    /** Проверка на массив */
    bool isArray() {
        if (!this->ctx) {
            return false;
        }
        return JS_IsArray(this->ctx, this->value);
    }

    /** Получить размер массива если значение считаетмя массивом */
    std::optional<int> getLength() {
        if (this->isArray()) {
            return this->getValue("length")->to_int();
        }

        return std::nullopt;
    }

    /** Получить значение массива по индексу */
    std::unique_ptr<JsValue> at(int index) {
        if (!this->isArray()) {
            return std::make_unique<JsValue>(this->ctx, JS_UNDEFINED);
        }

        JSValue raw_value = JS_GetPropertyUint32(ctx, this->value, index);
        return std::make_unique<JsValue>(this->ctx, raw_value);
    }

    /** Получить имя класса текущего значения */
    std::optional<std::string> getClassName() {
        return this->getValue("constructor")->getValue("name")->to_string();
    }

    /** Факт того, что значение является промисом */
    bool is_promise() {
        if (!ctx || !JS_IsObject(value)) {
            return false;
        }

        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue promise_ctor = JS_GetPropertyStr(ctx, global_obj, "Promise");
        JS_FreeValue(ctx, global_obj);

        int res = JS_IsInstanceOf(ctx, value, promise_ctor);

        JS_FreeValue(ctx, promise_ctor);
        return res > 0;
    }

    /** Получиь контекст */
    JSContext* getContext() {
        return this->ctx;
    }
};