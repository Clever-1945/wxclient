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

    static JSValue c_function_promise(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data)
    {
        int64_t ptr_val;
        if (JS_ToInt64(ctx, &ptr_val, data[0]) < 0)
        {
            return JS_ThrowTypeError(ctx, "Failed to retrieve function pointer");
        }

        auto *func = reinterpret_cast<std::function<JsVariant(std::vector<JsVariant>)> *>(ptr_val);

        JSValue resolving_funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, resolving_funcs);

        JSValue resolve_func = JS_DupValue(ctx, resolving_funcs[0]);
        JSValue reject_func = JS_DupValue(ctx, resolving_funcs[1]);

        JS_FreeValue(ctx, resolving_funcs[0]);
        JS_FreeValue(ctx, resolving_funcs[1]);

        std::vector<JsVariant> args;
        for (int i = 0; i < argc; i++) 
        {
            args.push_back(assistant::js::to_variant(ctx, argv[i]));
        }

        std::thread worker([reject_func, resolve_func, args, func, ctx]()
        {
            JsVariant result = JsVariant(std::monostate{});
            bool isSucess = true;
            try 
            {
                if (func)
                {
                    result = (*func)(args);
                }
            }
            catch (const std::exception& e) 
            {
                isSucess = false;
            }
            // delete func;
        
            wxTheApp->CallAfter([resolve_func, reject_func, result, ctx, isSucess]() 
            {
                auto js_result = assistant::js::to_js_value(ctx, result);
                JSValue argv[1] = 
                { 
                    js_result
                };
                JSValue ret = isSucess
                    ? assistant::js::call(ctx, resolve_func, assistant::js::getUndefined(), 1, argv)
                    : assistant::js::call(ctx, reject_func, assistant::js::getUndefined(), 1, argv);
                JS_FreeValue(ctx, js_result);
                JS_FreeValue(ctx, ret);
                JS_FreeValue(ctx, resolve_func);
                JS_FreeValue(ctx, reject_func);

                JSContext *pctx;
                while (JS_ExecutePendingJob(JS_GetRuntime(ctx), &pctx) > 0) {
                
                }
            }); 
        });
        worker.detach();
        return promise;
    }

public:
    /*** Инициализировать движок */
    bool init()
    {
        if (isInit.has_value())
        {
            return isInit.value();
        }

        rt = JS_NewRuntime();
        if (!rt)
        {
            errorInit = "Ошибка: Не удалось создать JSRuntime";
            isInit = false;
            return false;
        }

        ctx = JS_NewContext(rt);
        if (!ctx)
        {
            JS_FreeRuntime(rt);
            errorInit = "Ошибка: Не удалось создать JSContext";
            isInit = false;
            return false;
        }

        isInit = true;
        return true;
    }

    /** Получить текст ошибки при инициализации */
    std::string getErrorInit()
    {
        return errorInit;
    }

    /*** Освободить ресурсы */
    void free()
    {
        if (ctx)
        {
            JS_FreeContext(ctx);
        }
        ctx = nullptr;
        
        if (rt)
        {
            JS_FreeRuntime(rt);
        }
        
        rt = nullptr;
    }

    /** Выполнить скрипт из файла */
    void evalFile(std::string fileName)
    {
        assistant::js::evalFile(ctx, fileName);
    }

    /** Функция гарантирует получение или создание глобального объекта */
    JSValue getOrCreateGlobalObject(std::string name, JSValue parentObject)
    {
        JSValue target_obj = JS_GetPropertyStr(ctx, parentObject, name.c_str());
        if (JS_IsObject(target_obj))
        {
            return target_obj;
        }

        JS_FreeValue(ctx, target_obj);

        target_obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, parentObject, name.c_str(), JS_DupValue(ctx, target_obj));
        return target_obj;
    }

    /** Функция гарантирует получение или создание глобального объекта */
    JSValue getOrCreateGlobalObject(std::string name)
    {
        auto segments = assistant::core::split(name, ".");
        JSValue currentObject = JS_GetGlobalObject(ctx);
        for (const auto& s : segments)
        {
            auto segment = assistant::core::trim(s);
            if (segment.empty())
            {
                continue;
            }

            JSValue nextObject = getOrCreateGlobalObject(segment, currentObject);
            JS_FreeValue(ctx, currentObject);
            currentObject = nextObject;
        }

        return currentObject;
    }

    void registerFn(std::string objectName, std::string functionName, JSCFunction *func)
    {
        JSValue native_app = getOrCreateGlobalObject(objectName);
        JSValue js_func = JS_NewCFunction(ctx, func, functionName.c_str(), 1);
        JS_SetPropertyStr(ctx, native_app, functionName.c_str(), js_func);
        JS_FreeValue(ctx, native_app);
    }

    /** Регистрация функции, которая возвращает промис */
    void registerPromiseFn(std::string objectName, std::string functionName, std::function<JsVariant(std::vector<JsVariant>)> fn)
    {
        auto smartFn = new std::function<JsVariant(std::vector<JsVariant>)>(std::move(fn));
        uintptr_t rawAddress = reinterpret_cast<uintptr_t>(smartFn);
        int64_t addressAsInt64 = static_cast<int64_t>(rawAddress);

        JSValue data = JS_NewInt64(ctx, addressAsInt64);
        JSValue js_func = JS_NewCFunctionData(ctx, c_function_promise, 0, 0, 1, &data);
    
        JSValue native_app = getOrCreateGlobalObject(objectName);
        JS_SetPropertyStr(ctx, native_app, functionName.c_str(), js_func);
        JS_FreeValue(ctx, native_app);
        JS_FreeValue(ctx, data); 
    }

    void registerPrototype(std::string className, std::string functionName, JSCFunction *func)
    {        
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue class_constructor = JS_GetPropertyStr(ctx, global_obj, className.c_str());

        if (JS_IsException(class_constructor))
        {
            JS_FreeValue(ctx, global_obj);
            return;
        }

        if (JS_IsUndefined(class_constructor))
        {
            JS_FreeValue(ctx, global_obj);
            return;
        }

        JSValue proto = JS_GetPropertyStr(ctx, class_constructor, "prototype");

        if (!JS_IsException(proto) && JS_IsObject(proto))
        {
            JSValue c_func = JS_NewCFunction(ctx, func, functionName.c_str(), 0);
            JS_SetPropertyStr(ctx, proto, functionName.c_str(), c_func);
        }

        JS_FreeValue(ctx, proto);
        JS_FreeValue(ctx, class_constructor);
        JS_FreeValue(ctx, global_obj);
    }

    /** Установить данные для контекста */
    void setContextOpaque(void *opaque)
    {
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
    std::optional<std::string> to_string(JSValue value)
    {
        return assistant::js::to_string(ctx, value);
    }

    /** Получить целочисленное значение */
    std::optional<int> to_int(JSValue value)
    {
        return assistant::js::to_int(ctx, value);
    }

    /** Получить булевное значение */
    std::optional<bool> to_bool(JSValue value)
    {
        return assistant::js::to_bool(ctx, value);
    }

    /** Получить имя класса */
    std::string getClassName(JSValue value)
    {
        return assistant::js::getClassName(ctx, value);
    }
};
