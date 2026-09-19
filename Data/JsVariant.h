#pragma once
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <optional>
extern "C"
{
#include <quickjs.h>
}

struct JsVariant;

using JsArray = std::vector<JsVariant>;
using JsObject = std::map<std::string, JsVariant>;

struct JsVariant : std::variant<
        std::monostate, // null / undefined
        bool,
        int64_t,
        double,
        std::string,
        JsArray,
        JsObject> {
    using variant::variant;

    std::optional<int64_t> to_int_64() const {
        if (std::holds_alternative<int64_t>(*this)) {
            return std::get<int64_t>(*this);
        }
        return std::nullopt;
    }

    std::optional<int> to_int() const {
        if (std::holds_alternative<int64_t>(*this)) {
            return static_cast<int>(std::get<int64_t>(*this));
        }
        return std::nullopt;
    }

    std::optional<bool> to_bool() const {
        if (std::holds_alternative<bool>(*this)) {
            return std::get<bool>(*this);
        }
        return std::nullopt;
    }

    std::optional<double> to_double() const {
        if (std::holds_alternative<double>(*this)) {
            return std::get<double>(*this);
        }
        return std::nullopt;
    }

    std::optional<JsArray> to_array() const {
        if (std::holds_alternative<JsArray>(*this)) {
            return std::get<JsArray>(*this);
        }
        return std::nullopt;
    }

    std::optional<JsObject> to_object() const {
        if (std::holds_alternative<JsObject>(*this)) {
            return std::get<JsObject>(*this);
        }
        return std::nullopt;
    }

    std::optional<std::string> to_string() const {
        if (std::holds_alternative<std::string>(*this)) {
            return std::get<std::string>(*this);
        }
        return std::nullopt;
    }

    bool isUndefined() const {
        return std::holds_alternative<std::monostate>(*this);
    }

    JSValue to_js_value(JSContext* ctx) const {
        if (isUndefined()) {
            return JS_UNDEFINED;
        }
        auto bool_value = to_bool();
        if (bool_value.has_value()) {
            return JS_NewBool(ctx, bool_value.value());
        }

        auto int64_value = to_int_64();
        if (int64_value.has_value()) {
            return JS_NewInt64(ctx, int64_value.value());
        }

        auto double_value = to_double();
        if (double_value.has_value()) {
            return JS_NewFloat64(ctx, double_value.value());
        }

        auto string_value = to_string();
        if (string_value.has_value()) {
            return JS_NewStringLen(ctx, string_value.value().c_str(), string_value.value().length());
        }

        auto array_value = to_array();
        if (array_value.has_value()) {
            JSValue js_arr = JS_NewArray(ctx);
            if (JS_IsException(js_arr))
            {
                return JS_UNDEFINED;
            }

            auto arg = array_value.value();
            for (size_t i = 0; i < arg.size(); i++)
            {
                JSValue elem = arg[i].to_js_value(ctx);
                JS_DefinePropertyValueUint32(ctx, js_arr, i, elem, JS_PROP_C_W_E);
            }
            return js_arr;
        }

        auto object_value = to_object();
        if (object_value.has_value()) {
            auto arg = object_value.value();
            JSValue js_obj = JS_NewObject(ctx);
            if (JS_IsException(js_obj))
            {
                return JS_UNDEFINED;
            }

            for (const auto& [key, value] : arg)
            {
                JSValue prop_val = value.to_js_value(ctx);
                JS_SetPropertyStr(ctx, js_obj, key.c_str(), prop_val);
            }
            return js_obj;
        }

        return JS_UNDEFINED;
    }
};
