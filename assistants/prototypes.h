namespace prototypes 
{
    namespace text 
    {
        template<typename T>
        T* getInstance(JSContext *ctx, JSValue value)
        {
            auto address = assistant::js::to_int_64(ctx, value, "__wx_ptr");
            if (!address.has_value())
            {
                return nullptr;
            }
            T* control = reinterpret_cast<T*>(address.value());
            return control;
        }
        
        JSValue get_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) 
        {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (!text) 
            {
                return assistant::js::getUndefined();
            }

            std::string value = text->GetValue().ToStdString(wxConvUTF8);
            return JS_NewString(ctx, value.c_str());
        }

        JSValue set_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) 
        {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (text && argc > 0) 
            {
                auto next_text = assistant::js::to_string(ctx, argv[0]);
                if (next_text.has_value())
                {
                    text->SetValue(wxString::FromUTF8(next_text.value()));
                }
            }
            return assistant::js::getUndefined();
        }
    }
}