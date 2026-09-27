#pragma once
#include <wx/clntdata.h>
#include "JsValue.h"

extern "C"
{
#include <quickjs.h>
}

class JSValueClientData : public wxClientData {
private:
    JSContext *ctx = nullptr;
    JSValue val = JS_UNDEFINED;

public:
    JSValueClientData(JSContext *ctx, JSValue val) {
        if (JS_IsUndefined(val)) {
            return;
        }
        this->val = val;
        this->ctx = ctx;
    }

    ~JSValueClientData() {
        if (ctx) {
            JS_FreeValue(ctx, val);
        }
        ctx = nullptr;
    }

    std::unique_ptr<JsValue> getJsValue(bool disableFree = false) {
        return std::make_unique<JsValue>(this->ctx, this->val, disableFree);
    }

    JSValue getValue() {
        return val;
    }

    JSContext *getContext() {
        return ctx;
    }
};