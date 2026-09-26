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
        JsVariant to_variant(JSContext *ctx, JSValue value);
        /** Выполнить функцию в скрипте */
        JSValue call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValueConst *argv, int argc);
        JSValue call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValue parameter);
        JSValue getUndefined();
        /** Завершить все фоновые микротаски */
        void pendingJob (JSContext *ctx);
        /** Преобразовать TS контент в JS */
        std::optional<std::string> convert_ts_to_js(JSContext *ctx, const std::string& ts_source_code);

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

            /** Функция гарантирует получение или создание глобального объекта */
            JSValue getOrCreateGlobalObject(JSContext *ctx, std::string name, JSValue parentObject) {
                JSValue target_obj = JS_GetPropertyStr(ctx, parentObject, name.c_str());
                if (JS_IsObject(target_obj)) {
                    return target_obj;
                }

                JS_FreeValue(ctx, target_obj);

                target_obj = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, parentObject, name.c_str(), JS_DupValue(ctx, target_obj));
                return target_obj;
            }

            JSValue c_function_promise(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data) {
                int64_t ptr_val;
                if (JS_ToInt64(ctx, &ptr_val, data[0]) < 0) {
                    JSException ex = {};
                    ex.error = "Failed to retrieve function pointer";
                    ex.stack = "";
                    assistant::js::exceptions->set(ex);
                    return JS_ThrowTypeError(ctx, ex.error.c_str());
                }

                auto *func = reinterpret_cast<std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)> *>(ptr_val);

                JSValue resolving_funcs[2];
                JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);

                JSValue resolve_func = JS_DupValue(ctx, resolving_funcs[0]);
                JSValue reject_func = JS_DupValue(ctx, resolving_funcs[1]);

                JS_FreeValue(ctx, resolving_funcs[0]);
                JS_FreeValue(ctx, resolving_funcs[1]);

                std::vector<JsVariant> args;
                for (int i = 0; i < argc; i++) {
                    args.push_back(assistant::js::to_variant(ctx, argv[i]));
                }

                std::thread worker([reject_func, resolve_func, args, func, ctx, this_val]() {
                    JsVariant result = JsVariant(std::monostate{});
                    bool isSucess = true;
                    try {
                        if (func) {
                            result = (*func)(ctx, this_val, args);
                        }
                    }
                    catch (const std::exception &e) {
                        isSucess = false;
                    }

                    wxTheApp->CallAfter([resolve_func, reject_func, result, ctx, isSucess]() {
                        auto js_result = result.to_js_value(ctx);
                        JSValue ret = isSucess
                                      ? assistant::js::call(ctx, resolve_func, assistant::js::getUndefined(), js_result)
                                      : assistant::js::call(ctx, reject_func, assistant::js::getUndefined(), js_result);
                        JS_FreeValue(ctx, js_result);
                        JS_FreeValue(ctx, ret);
                        JS_FreeValue(ctx, resolve_func);
                        JS_FreeValue(ctx, reject_func);

                        assistant::js::pendingJob(ctx);
                    });
                });
                worker.detach();
                return promise;
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

        JSValue to_value(JSContext *ctx, std::vector<std::string> value) {
            JSValue js_arr = JS_NewArray(ctx);
            if (JS_IsException(js_arr)) {
                return JS_UNDEFINED;
            }

            auto size = value.size();
            for (int i = 0; i < size; i++) {
                JSValue elem = assistant::js::to_value(ctx, value[i]);
                JS_DefinePropertyValueUint32(ctx, js_arr, i, elem, JS_PROP_C_W_E);
            }
            return js_arr;
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
        bool is_promise(JSContext *ctx, JSValue value) {
            if (!JS_IsObject(value)) {
                return false;
            }

            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue promise_ctor = JS_GetPropertyStr(ctx, global_obj, "Promise");
            JS_FreeValue(ctx, global_obj);

            int res = JS_IsInstanceOf(ctx, value, promise_ctor);

            JS_FreeValue(ctx, promise_ctor);
            return res > 0;
        }

        JsVariant to_variant(JSContext *ctx, JSValue value) {
            if (JS_IsUndefined(value) || JS_IsNull(value)) {
                return JsVariant(std::monostate{});
            }
            if (JS_IsBool(value)) {
                return JsVariant(assistant::js::to_bool(ctx, value).value_or(false));
            }
            if (JS_IsNumber(value)) {
                if (JS_VALUE_GET_TAG(value) == JS_TAG_INT) {
                    auto int_64 = assistant::js::to_int_64(ctx, value);
                    return JsVariant(int_64.value_or(0));
                }

                double v = 0.0;
                JS_ToFloat64(ctx, &v, value);
                return JsVariant(v);
            }

            if (JS_IsString(value)) {
                auto str = assistant::js::to_string(ctx, value);
                return JsVariant(str.value_or(""));
            }
            if (JS_IsArray(ctx, value)) {
                JsArray arr;
                JSValue len_val = JS_GetPropertyStr(ctx, value, "length");
                int32_t len = 0;
                JS_ToInt32(ctx, &len, len_val);
                JS_FreeValue(ctx, len_val);

                arr.reserve(len);
                for (int32_t i = 0; i < len; i++) {
                    JSValue elem = JS_GetPropertyUint32(ctx, value, i);
                    arr.push_back(to_variant(ctx, elem));
                    JS_FreeValue(ctx, elem);
                }
                return JsVariant(arr);
            }
            if (JS_IsObject(value)) {
                JsObject obj;
                JSPropertyEnum *ptab = nullptr;
                uint32_t plen = 0;

                if (JS_GetOwnPropertyNames(ctx, &ptab, &plen, value, JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK) >= 0) {
                    for (uint32_t i = 0; i < plen; i++) {
                        const char *key = JS_AtomToCString(ctx, ptab[i].atom);
                        JSValue prop_val = JS_GetProperty(ctx, value, ptab[i].atom);

                        if (key) {
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

        JSValue to_js_value(JSContext *ctx, const JsVariant &var) {
            return var.to_js_value(ctx);
        }

        /** Выполнить функцию в скрипте */
        JSValue call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValueConst *argv, int argc) {
            JSValue result = JS_Call(ctx, func_obj, this_obj, argc, argv);
            assistant::js::pprivate::processException(ctx, result);
            assistant::js::pendingJob(ctx);
            return result;
        }

        /** Выполнить функцию в скрипте */
        JSValue call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj, JSValue parameter) {
            JSValue argv[1] = {parameter};
            return call(ctx, func_obj, this_obj, argv, 1);
        }

        /** Выполнить скрипт из файла */
        void evalScript(JSContext *ctx, std::string script, std::string fileName) {
            if (script.empty()) {
                return;
            }
            JSValue result = JS_Eval(ctx, script.c_str(), script.length(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);
            if (assistant::js::pprivate::processException(ctx, result)) {
                JS_FreeValue(ctx, result);
            }

            JS_FreeValue(ctx, result);
            assistant::js::pendingJob(ctx);
        }

        /** Выполнить скрипт из файла */
        void evalFile(JSContext *ctx, std::string fileName) {
            auto executableDirectory = assistant::directory::getDirectoryExecutable();
            auto fullFileName = assistant::path::combine(executableDirectory, fileName);
            std::string script = assistant::file::read(fullFileName);
            auto ts = assistant::js::convert_ts_to_js(ctx, script).value_or("");
            evalScript(ctx, ts, fileName);
        }

        /** Преобразовать TS контент в JS */
        std::optional<std::string> convert_ts_to_js(JSContext *ctx, const std::string& ts_source_code) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue transpile_fn = JS_GetPropertyStr(ctx, global_obj, "transpileTS");

            if (!JS_IsFunction(ctx, transpile_fn)) {
                JS_FreeValue(ctx, transpile_fn);
                JS_FreeValue(ctx, global_obj);
                JSException ex = {};
                ex.error = "Ошибка: Функция transpileTS не найдена в контексте QuickJS";
                ex.stack = std::string ();
                exceptions->set(ex);
                return std::nullopt;
            }

            JSValue arg_ts_code = JS_NewStringLen(ctx, ts_source_code.c_str(), ts_source_code.size());
            JSValue js_result_value = JS_Call(ctx, transpile_fn, global_obj, 1, &arg_ts_code);
            std::string pure_js_code = "";
            if (!JS_IsException(js_result_value)) {
                const char* c_str = JS_ToCString(ctx, js_result_value);
                if (c_str) {
                    pure_js_code = c_str;
                    JS_FreeCString(ctx, c_str);
                }
            } else {
                JSException ex = {};
                ex.error = "Ошибка во время транспиляции внутри Sucrase";
                ex.stack = std::string ();
                exceptions->set(ex);
            }

            JS_FreeValue(ctx, arg_ts_code);
            JS_FreeValue(ctx, js_result_value);
            JS_FreeValue(ctx, transpile_fn);
            JS_FreeValue(ctx, global_obj);

            return pure_js_code;
        }
        
        /** Получить значение из массива */
        JSValue getValue(int argc, JSValueConst *argv, int index) {
            if (index < 0) {
                return assistant::js::getUndefined();
            }
            if (index >= argc) {
                return assistant::js::getUndefined();
            }

            return argv[index];
        }

        /** Получить значение по ключу в объекте */
        JSValue getValue(JSContext *ctx, JSValue value, std::string propertyName) {
            return JS_GetPropertyStr(ctx, value, propertyName.c_str());
        }
        
        /** Получить строку */
        std::optional<std::string> to_string(JSContext *ctx, JSValue value) {
            if (JS_IsString(value)) {
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
        std::optional<int> to_int(JSContext *ctx, JSValue value) {
            if (JS_IsUndefined(value)) {
                return std::nullopt;
            }

            int result = 0;
            int error = JS_ToInt32(ctx, &result, value);

            if (error < 0) {
                return std::nullopt;
            }

            return result;
        }

        /** Получить целочисленное значение */
        std::optional<int> to_int(JSContext *ctx, JSValue value, std::string propertyName) {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_int(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить целочисленное значение */
        std::optional<int64_t> to_int_64(JSContext *ctx, JSValue value) {
            int64_t result = 0;
            int error = JS_ToInt64(ctx, &result, value);
            if (JS_IsUndefined(value)) {
                return std::nullopt;
            }

            if (error < 0) {
                return std::nullopt;
            }

            return result;
        }

        /** Получить целочисленное значение */
        std::optional<int64_t> to_int_64(JSContext *ctx, JSValue value, std::string propertyName) {
            auto propertyValue = assistant::js::getValue(ctx, value, propertyName);
            auto returnValue = to_int_64(ctx, propertyValue);

            JS_FreeValue(ctx, propertyValue);
            return returnValue;
        }

        /** Получить булевное значение */
        std::optional<bool> to_bool(JSContext *ctx, JSValue value) {
            if (JS_IsBool(value)) {
                return JS_ToBool(ctx, value) != 0 ? true : false;
            }
            return std::nullopt;
        }

        /** Получить булевное значение */
        std::optional<bool> to_bool(JSContext *ctx, JSValue value, std::string propertyName) {
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
        std::string getClassName(JSContext *ctx, JSValue obj) {
            std::string className;
            if (!JS_IsObject(obj)) {
                return className;
            }

            JSValue constructor = JS_GetPropertyStr(ctx, obj, "constructor");

            if (!JS_IsException(constructor) && JS_IsObject(constructor)) {
                JSValue name_val = JS_GetPropertyStr(ctx, constructor, "name");

                if (!JS_IsException(name_val) && JS_IsString(name_val)) {
                    const char *class_name = JS_ToCString(ctx, name_val);
                    if (class_name) {
                        className = std::string(class_name);
                        JS_FreeCString(ctx, class_name);
                    }
                }
                JS_FreeValue(ctx, name_val);
            }

            JS_FreeValue(ctx, constructor);
            return className;
        }

        /** Функция гарантирует получение или создание глобального объекта */
        JSValue getOrCreateGlobalObject(JSContext *ctx, std::string name) {
            auto segments = assistant::core::split(name, ".");
            JSValue currentObject = JS_GetGlobalObject(ctx);
            for (const auto &s: segments) {
                auto segment = assistant::core::trim(s);
                if (segment.empty()) {
                    continue;
                }

                JSValue nextObject = assistant::js::pprivate::getOrCreateGlobalObject(ctx, segment, currentObject);
                JS_FreeValue(ctx, currentObject);
                currentObject = nextObject;
            }

            return currentObject;
        }

        /** Получить указатель на портотип. Нужно удалить объект после использваония */
        JSValue getPrototype(JSContext *ctx, std::string className) {
            JSValue global_obj = JS_GetGlobalObject(ctx);
            JSValue class_constructor = JS_GetPropertyStr(ctx, global_obj, className.c_str());

            if (JS_IsException(class_constructor)) {
                JS_FreeValue(ctx, global_obj);
                return assistant::js::getUndefined();
            }

            if (JS_IsUndefined(class_constructor)) {
                JS_FreeValue(ctx, global_obj);
                return assistant::js::getUndefined();
            }

            JSValue proto = JS_GetPropertyStr(ctx, class_constructor, "prototype");
            JS_FreeValue(ctx, class_constructor);
            JS_FreeValue(ctx, global_obj);

            if (JS_IsException(proto)) {
                return assistant::js::getUndefined();
            }

            if (JS_IsObject(proto)) {
                return proto;
            }

            JS_FreeValue(ctx, proto);
            return assistant::js::getUndefined();
        }

        /** Зарегистрировать функцию для оъекта */
        void registerFn(JSContext *ctx, JSValue instanceObject, std::string functionName, JSCFunction *func) {
            if (JS_IsObject(instanceObject)) {
                JSValue js_func = JS_NewCFunction(ctx, func, functionName.c_str(), 0);
                if (JS_IsFunction(ctx, js_func)) {
                    JS_SetPropertyStr(ctx, instanceObject, functionName.c_str(), js_func);
                    JS_FreeValue(ctx, instanceObject);
                }
            }
        }

        /** Зарегистрировать функцию для оъекта */
        void registerFn(JSContext *ctx, std::string objectName, std::string functionName, JSCFunction *func) {
            JSValue instanceObject = assistant::js::getOrCreateGlobalObject(ctx, objectName);
            registerFn(ctx, instanceObject, functionName, func);
        }

        /** Зарегистрировать портатип для класса */
        void registerPrototype(JSContext *ctx, std::string className, std::string functionName, JSCFunction *func) {
            JSValue instanceObject = assistant::js::getPrototype(ctx, className);
            registerFn(ctx, instanceObject, functionName, func);
        }

        /** Регистрация функции, которая возвращает промис */
        void registerPromiseFn(JSContext *ctx, JSValue instanceObject, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            auto smartFn = new std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>(std::move(fn));
            uintptr_t rawAddress = reinterpret_cast<uintptr_t>(smartFn);
            int64_t addressAsInt64 = static_cast<int64_t>(rawAddress);

            JSValue data = JS_NewInt64(ctx, addressAsInt64);

            JSValue js_func = JS_NewCFunctionData(ctx, assistant::js::pprivate::c_function_promise, 0, 0, 1, &data);

            JS_SetPropertyStr(ctx, instanceObject, functionName.c_str(), js_func);
            JS_FreeValue(ctx, instanceObject);
            JS_FreeValue(ctx, data);
        }

        /** Регистрация функции, которая возвращает промис, для класса ( портотип ) */
        void registerPromiseInClass(JSContext *ctx, std::string className, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            JSValue instanceObject = assistant::js::getPrototype(ctx, className);
            registerPromiseFn(ctx, instanceObject, functionName, fn);
        }

        /** Регистрация функции, которая возвращает промис, для объекта ( например app.otherObject.functionName ) */
        void registerPromiseInObject(JSContext *ctx, std::string objectName, std::string functionName, const std::function<JsVariant(JSContext*, JSValue, std::vector<JsVariant>)>& fn) {
            JSValue instanceObject = assistant::js::getOrCreateGlobalObject(ctx, objectName);
            registerPromiseFn(ctx, instanceObject, functionName, fn);
        }

        /** Послать событие обратно в JS и передать в событии ссылку на компонент */
        void emitEvent(wxEventControlJsData data, std::string eventName) {
            auto context = data.clientData->getContext();
            auto instance = data.clientData->getValue();
            auto changed = assistant::js::to_function(context, instance, eventName);
            if (changed.has_value()) {
                auto changedFn = changed.value();
                JSValue ret = assistant::js::call(context, changedFn, assistant::js::getUndefined(), instance);
                JS_FreeValue(context, ret);
                JS_FreeValue(context, changedFn);
            }
        }
    }
}

