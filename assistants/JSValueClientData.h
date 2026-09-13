#include <wx/clntdata.h>

extern "C"
{
#include <quickjs.h>
}

class JSValueClientData : public wxClientData {
private:
    JSContext* ctx = nullptr;
    JSValue val = JS_UNDEFINED;

public:
    JSValueClientData(JSContext* ctx, JSValue val) {
        // m_val = JS_DupValue(ctx, val); 
        this->val = val;
        // this->val = JS_DupValue(ctx, val);
        this->ctx = ctx;
    }

    ~JSValueClientData() 
    {
        if (ctx) 
        {
            JS_FreeValue(ctx, val);
        }
        ctx = nullptr;
    }

    JSValue getValue() 
    { 
        return val; 
    }

    JSContext* getContext() 
    { 
        return ctx; 
    }
};