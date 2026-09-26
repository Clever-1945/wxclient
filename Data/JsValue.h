#pragma once
#include <string>
extern "C"
{
#include <quickjs.h>
}

/** JavaScript значение. Значение должно освобождаться само после выхода из области видимости */
class JsValue {
private:
    JSContext *ctx = nullptr;
    JSValue value = JS_UNDEFINED;
public:
    JsValue(JSContext *ctx, JSValue value) {
        this->ctx = ctx;
        this->value = value;
    }
    ~JsValue() {
        if (ctx && !JS_IsUndefined(value)) {
            JS_FreeValue(ctx, value);
        }
    }

    JsValue(const JsValue&) = delete;
    JsValue& operator=(const JsValue&) = delete;

    /** Получить значение по ключу в объекте */
    JsValue getValue(const std::string& propertyName) const  {
        auto value = JS_GetPropertyStr(this->ctx, this->value, propertyName.c_str());
        return JsValue(this->ctx, value);
    }
};