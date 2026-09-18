#pragma once
#include <string>
#include <optional>
#include <variant>
#include "Data/ObservableValue.h"
#include "Data/JSException.h"
#include "Data/JsVariant.h"

extern "C"
{
#include <quickjs.h>
}

namespace assistant
{
    namespace js
    {
        std::optional<std::string> to_string(JSContext *ctx, JSValue value);
        std::optional<std::string> to_string(JSContext *ctx, JSValue value, std::string propertyName);
        std::optional<int> to_int(JSContext *ctx, JSValue value);
        std::optional<int64_t> to_int_64(JSContext *ctx, JSValue value);
        std::optional<bool> to_bool(JSContext *ctx, JSValue value);
        ObservableValue<JSException>* exceptions = new ObservableValue<JSException>();
        ObservableValue<std::string>* logs = new ObservableValue<std::string>();
        ObservableValue<std::string>* warning = new ObservableValue<std::string>();

        namespace pprivate
        {
            bool processException(JSContext *ctx, JSValue result) {
                if (JS_IsException(result)) {
                    JSValue exception_message = JS_GetException(ctx);
                    JSValue exception_stack = JS_GetPropertyStr(ctx, exception_message, "stack");

                    auto message = assistant::js::to_string(ctx, exception_message, "message");
                    auto stack = assistant::js::to_string(ctx, exception_message, "stack");

                    JS_FreeValue(ctx, exception_message);
                    JS_FreeValue(ctx, exception_stack);
                    JS_FreeValue(ctx, result);

                    if (message.has_value()) {
                        JSException ex{};
                        ex.error = message.value();
                        ex.stack = stack.value_or("");
                        assistant::js::exceptions->set(ex);
                    }

                    return true;
                }

                return false;
            }
        }

        /** Завершить все фоновые микротаски */
        void pendingJob (JSContext *ctx) {
            JSContext *pctx;
            while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &pctx) > 0) {

            }
        }

        JSValue getUndefined() {
            return JS_UNDEFINED;
        }

        JSValue to_value(JSContext *ctx, std::string value) {
            return JS_NewString(ctx, value.c_str());
        }

        JSValue to_value(JSContext *ctx, int value) {
            return JS_NewInt32(ctx, value);
        }

        JSValue to_value(JSContext *ctx, long value) {
            return JS_NewInt64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, int64_t value) {
            return JS_NewInt64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, double value) {
            return JS_NewFloat64(ctx, value);
        }

        JSValue to_value(JSContext *ctx, bool value) {
            return JS_NewBool(ctx, value);
        }

        /** Преобразовать JSON строку в JS значение */
        JSValue to_value_from_json(JSContext *ctx, std::string json) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue json_obj = JS_GetPropertyStr(ctx, global_obj, "JSON");
            JSValue parse_fn = JS_GetPropertyStr(ctx, json_obj, "parse");
            JSValue json_js_string = JS_NewString(ctx, json.c_str());
            JSValue result = JS_Call(ctx, parse_fn, json_obj, 1, &json_js_string);

            JS_FreeValue(ctx, global_obj);
            JS_FreeValue(ctx, json_obj);
            JS_FreeValue(ctx, parse_fn);
            JS_FreeValue(ctx, json_js_string);

            return result;
        }

        /** Преобразовать JS объект в JSON строку */
        std::optional<std::string> to_json(JSContext *ctx, JSValue value) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue json_obj = JS_GetPropertyStr(ctx, global_obj, "JSON");
            JSValue stringify_fn = JS_GetPropertyStr(ctx, json_obj, "stringify");

            JSValue argv[3];
            argv[0] = value;
            argv[1] = JS_NULL;
            argv[2] = JS_NewInt32(ctx, 4);

            JSValue json_js_string = JS_Call(ctx, stringify_fn, json_obj, 3, argv);
            auto text_json = to_string(ctx, json_js_string);

            JS_FreeValue(ctx, argv[2]);
            JS_FreeValue(ctx, global_obj);
            JS_FreeValue(ctx, json_obj);
            JS_FreeValue(ctx, stringify_fn);
            JS_FreeValue(ctx, json_js_string);

            return text_json;
        }

        /** Факт того, что значение является промисом */
        bool is_promise(JSContext *ctx, JSValue value)
        {
            if (!JS_IsObject(value))
            {
                return false;
            }

            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue promise_ctor = JS_GetPropertyStr(ctx, global_obj, "Promise");
            JS_FreeValue(ctx, global_obj);

            int res = JS_IsInstanceOf(ctx, value, promise_ctor);

            JS_FreeValue(ctx, promise_ctor);
            return res > 0;
        }

        JsVariant to_variant(JSContext *ctx, JSValue value)
        {
            if (JS_IsUndefined(value) || JS_IsNull(value))
            {
                return JsVariant(std::monostate{});
            }
            if (JS_IsBool(value))
            {
                return JsVariant(assistant::js::to_bool(ctx, value).value_or(false));
            }
            if (JS_IsNumber(value))
            {
                if (JS_VALUE_GET_TAG(value) == JS_TAG_INT) 
                {
                    auto int_64 = assistant::js::to_int_64(ctx, value);
                    return JsVariant(int_64.value_or(0));
                }
                
                double v = 0.0;
                JS_ToFloat64(ctx, &v, value);
                return JsVariant(v);
            }
            
            if (JS_IsString(value))
            {
                auto str = assistant::js::to_string(ctx, value);
                return JsVariant(str.value_or(""));
            }
            if (JS_IsArray(ctx, value))
            {
                JsArray arr;
                JSValue len_val = JS_GetPropertyStr(ctx, value, "length");
                int32_t len = 0;
                JS_ToInt32(ctx, &len, len_val);
                JS_FreeValue(ctx, len_val);

                arr.reserve(len);
                for (int32_t i = 0; i < len; i++)
                {
                    JSValue elem = JS_GetPropertyUint32(ctx, value, i);
                    arr.push_back(to_variant(ctx, elem));
                    JS_FreeValue(ctx, elem);
                }
                return JsVariant(arr);
            }
            if (JS_IsObject(value))
            {
                JsObject obj;
                JSPropertyEnum *ptab = nullptr;
                uint32_t plen = 0;

                if (JS_GetOwnPropertyNames(ctx, &ptab, &plen, value, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) >= 0)
                {
                    for (uint32_t i = 0; i < plen; i++)
                    {
                        const char *key = JS_AtomToCString(ctx, ptab[i].atom);
                        JSValue prop_val = JS_GetProperty(ctx, value, ptab[i].atom);

                        if (key)
                        {
                            obj[std::string(key)] = to_variant(ctx, prop_val);
                            JS_FreeCString(ctx, key);
                        }

                        JS_FreeValue(ctx, prop_val);
                        JS_FreeAtom(ctx, ptab[i].atom);
                    }
                    js_free(ctx, ptab);
                }
                return JsVariant(obj);
            }

            return JsVariant(std::monostate{});
        }

        JSValue to_js_value(JSContext* ctx, const JsVariant& var) 
        {
            return std::visit([ctx](auto&& arg) -> JSValue 
            {
                using T = std::decay_t<decltype(arg)>;
                
                if constexpr (std::is_same_v<T, std::monostate>) 
                {
                    return getUndefined();
                }
                else if constexpr (std::is_same_v<T, bool>) 
                {
                    return JS_NewBool(ctx, arg);
                }
                else if constexpr (std::is_same_v<T, int64_t>) 
                {
                    return JS_NewInt64(ctx, arg);
                }
                else if constexpr (std::is_same_v<T, double>) 
                {
                    return JS_NewFloat64(ctx, arg);
                }
                else if constexpr (std::is_same_v<T, std::string>) 
                {
                    return JS_NewStringLen(ctx, arg.c_str(), arg.length());
                }
                else if constexpr (std::is_same_v<T, JsArray>) 
                {
                    JSValue js_arr = JS_NewArray(ctx);
                    if (JS_IsException(js_arr)) 
                    {
                        return getUndefined();
                    }
                    
                    for (size_t i = 0; i < arg.size(); i++) 
                    {
                        JSValue elem = to_js_value(ctx, arg[i]);
                        JS_DefinePropertyValueUint32(ctx, js_arr, i, elem, JS_PROP_C_W_E);
                    }
                    return js_arr;
                }
                else if constexpr (std::is_same_v<T, JsObject>) 
                {
                    JSValue js_obj = JS_NewObject(ctx);
                    if (JS_IsException(js_obj))
                    {
                        return getUndefined();
                    }
                    
                    for (const auto& [key, value] : arg) 
                    {
                        JSValue prop_val = to_js_value(ctx, value);
                        JS_SetPropertyStr(ctx, js_obj, key.c_str(), prop_val);
                    }
                    return js_obj;
                }
                
                return getUndefined();;
            }, var);
        }

        /** Выполнить функцию в скрипте */
        JSValue call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, int argc, JSValueConst *argv)
        {
            JSValue result = JS_Call(ctx, func_obj, this_obj, argc, argv);
            JSContext *pctx;
            while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &pctx) > 0) {
            }
            assistant::js::pprivate::processException(ctx, result);
            return result;
        }

        /** Выполнить скрипт из файла */
        void evalFile(JSContext *ctx, std::string fileName)
        {
            auto executableDirectory = assistant::directory::getDirectoryExecutable();
            auto fullFileName = assistant::path::combine(executableDirectory, fileName);
            std::string script = assistant::file::read(fullFileName);
            if(script.empty())
            {
                return;
            }

            JSValue result = JS_Eval(ctx, script.c_str(), script.length(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);
            if (assistant::js::pprivate::processException(ctx, result))
            {
                JS_FreeValue(ctx, result);
            }

            JS_FreeValue(ctx, result);
            assistant::js::pendingJob(ctx);
        }
        
        /** Получить значение из массива */
        JSValue getValue(int argc, JSValueConst *argv, int index)
        {
            if (index < 0)
            {
                return assistant::js::getUndefined();
            }
            if (index >= argc)
            {
                return assistant::js::getUndefined();
            }

            // return JS_DupValue(ctx, argv[index]);
            return argv[index];
        }

        /** Получить значение по ключу в объекте */
        JSValue getValue(JSContext *ctx, JSValue value, std::string propertyName)
        {
            JSValue prop = JS_GetPropertyStr(ctx, value, propertyName.c_str());
            return prop;
        }
        
        /** Получить строку */
        std::optional<std::string> to_string(JSContext *ctx, JSValue value)
        {
            if (JS_IsString(value))
            {
                const char *text = JS_ToCString(ctx, value);
                auto str = std::string(text);
                JS_FreeCString(ctx, text);
                return str;
            }

            return std::nullopt;
        }

        /** Получить строку */
        std::optional<std::string> to_string(JSContext *ctx, JSValue value, std::string propertyName) {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_string(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Вернуть функцию */
        std::optional<JSValue> to_function(JSContext *ctx, JSValue value) {
            if (JS_IsFunction(ctx, value)) {
                return value;
            }

            return std::nullopt;
        }

        /** Вернуть функцию */
        std::optional<JSValue> to_function(JSContext *ctx, JSValue value, std::string propertyName) {
            auto fn = assistant::js::getValue(ctx, value, propertyName);
            if (JS_IsFunction(ctx, fn)) {
                return fn;
            }

            JS_FreeValue(ctx, fn);
            return std::nullopt;
        }

        /** Получить целочисленное значение */
        std::optional<int> to_int(JSContext *ctx, JSValue value)
        {
            if(JS_IsUndefined(value))
            {
                return std::nullopt;
            }

            int result = 0;
            int error = JS_ToInt32(ctx, &result, value);

            if (error < 0)
            {
                return std::nullopt;
            }

            return result;
        }

        /** Получить целочисленное значение */
        std::optional<int> to_int(JSContext *ctx, JSValue value, std::string propertyName)
        {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_int(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить целочисленное значение */
        std::optional<int64_t> to_int_64(JSContext *ctx, JSValue value)
        {
            int64_t result = 0;
            int error = JS_ToInt64(ctx, &result, value);
            if(JS_IsUndefined(value))
            {
                return std::nullopt;
            }

            if (error < 0)
            {
                return std::nullopt;
            }

            return result;
        }

        /** Получить целочисленное значение */
        std::optional<int64_t> to_int_64(JSContext *ctx, JSValue value, std::string propertyName)
        {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_int_64(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить булевное значение */
        std::optional<bool> to_bool(JSContext *ctx, JSValue value)
        {
            if (JS_IsBool(value))
            {
                return JS_ToBool(ctx, value) != 0 ? true : false;
            }
            return std::nullopt;
        }

        /** Получить булевное значение */
        std::optional<bool> to_bool(JSContext *ctx, JSValue value, std::string propertyName)
        {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_bool(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить дробное число */
        std::optional<double> to_double(JSContext *ctx, JSValue value) {
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

        /** Получить дробное число */
        std::optional<double> to_double(JSContext *ctx, JSValue value, std::string propertyName) {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_double(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить имя класса */
        std::string getClassName(JSContext *ctx, JSValue obj)
        {
            std::string className;
            if (!JS_IsObject(obj))
            {
                return className;
            }

            JSValue constructor = JS_GetPropertyStr(ctx, obj, "constructor");

            if (!JS_IsException(constructor) && JS_IsObject(constructor))
            {
                JSValue name_val = JS_GetPropertyStr(ctx, constructor, "name");

                if (!JS_IsException(name_val) && JS_IsString(name_val))
                {
                    const char *class_name = JS_ToCString(ctx, name_val);
                    if (class_name)
                    {
                        className = std::string(class_name);
                        JS_FreeCString(ctx, class_name);
                    }
                }
                JS_FreeValue(ctx, name_val);
            }

            JS_FreeValue(ctx, constructor);
            return className;
        }

        /** Послать событие обратно в JS и передать в событии ссылку на компонент */
        void emitEvent(wxEventControlJsData data, std::string eventName) {
            auto context = data.clientData->getContext();
            auto instance = data.clientData->getValue();
            auto changed = assistant::js::to_function(context, instance, eventName);
            if (changed.has_value()) {
                JSValue argv[1] = {
                        instance
                };
                auto changedFn = changed.value();
                JSValue ret = assistant::js::call(context, changedFn, assistant::js::getUndefined(), 1, argv);
                JS_FreeValue(context, ret);
                JS_FreeValue(context, changedFn);
            }
        }
    }
}
// namespace assistant
