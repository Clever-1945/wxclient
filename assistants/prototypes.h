#include <wx/wx.h>

namespace prototypes
{
    namespace text 
    {
        template<typename T>
        T *getInstance(JSContext *ctx, JSValue value) {
            auto address = assistant::js::to_int_64(ctx, value, "__wx_ptr");
            if (!address.has_value()) {
                return nullptr;
            }
            T *control = reinterpret_cast<T *>(address.value());
            return control;
        }

        JSValue get_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (text) {
                return assistant::js::to_value(ctx, text->GetValue().ToStdString(wxConvUTF8));
            }

            return assistant::js::getUndefined();
        }

        JSValue set_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto text = prototypes::text::getInstance<wxTextCtrl>(ctx, this_val);
            if (text && argc > 0) {
                auto next_text = assistant::js::to_string(ctx, argv[0]);
                if (next_text.has_value()) {
                    text->SetValue(wxString::FromUTF8(next_text.value()));
                }
            }
            return assistant::js::getUndefined();
        }
    }

    namespace checkBox
    {
        /** Поулчить значение */
        JSValue get_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                return assistant::js::to_value(ctx, control->GetValue());
            }
            return assistant::js::getUndefined();
        }

        /** Получить подсказку */
        JSValue get_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control) {
                return assistant::js::to_value(ctx, control->GetLabel().ToStdString(wxConvUTF8));
            }
            return assistant::js::getUndefined();
        }

        /** Установить значение  */
        JSValue set_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control && argc > 0) {
                control->SetValue(assistant::js::to_bool(ctx, argv[0]).value_or(false));
            }
            return assistant::js::getUndefined();
        }

        /** Установить подсказку */
        JSValue set_label(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxCheckBox>(ctx, this_val);
            if (control && argc > 0) {
                control->SetLabel(assistant::js::to_string(ctx, argv[0]).value_or(""));
            }

            return assistant::js::getUndefined();
        }
    }

    namespace comboBox
    {
        /** Установить элементы в выпадающий список */
        void set_items(wxComboBox *control, JSContext *ctx, JSValue items) {
            if (!control) {
                return;
            }

            control->Clear();
            if (!JS_IsArray(ctx, items)) {
                return;
            }

            auto length = assistant::js::to_int(ctx, items, "length").value_or(0);

            for (int i = 0; i < length; i++) {
                JSValue item = JS_GetPropertyUint32(ctx, items, i);
                auto text = assistant::js::to_string(ctx, item, "text").value_or("");
                auto value = assistant::js::to_string(ctx, item, "value").value_or("");
                control->Append(wxString::FromUTF8(text.c_str()), new wxStringClientData(wxString::FromUTF8(value.c_str())));
                JS_FreeValue(ctx, item);
            }
        }

        /** Установить элементы в выпадающий список */
        JSValue set_items(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (control && argc > 0) {
                JSValue items = argv[0];
                prototypes::comboBox::set_items(control, ctx, items);
            }

            return assistant::js::getUndefined();
        }

        /** Вернуть выбранное значение в списке */
        JSValue get_selected_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (!control) {
                return assistant::js::getUndefined();
            }
            int selectedIndex = control->GetSelection();

            if (selectedIndex != wxNOT_FOUND)
            {
                wxClientData* rawData = control->GetClientObject(selectedIndex);

                if (rawData)
                {
                    wxStringClientData* stringData = dynamic_cast<wxStringClientData*>(rawData);
                    if (stringData)
                    {
                        std::string value = stringData->GetData().ToStdString();
                        return assistant::js::to_value(ctx, value);
                    }
                }
            }

            return assistant::js::getUndefined();
        }

        /** Установить значение в список */
        JSValue set_selected_value(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            auto control = prototypes::text::getInstance<wxComboBox>(ctx, this_val);
            if (!control) {
                return assistant::js::getUndefined();
            }
            if (argc < 1) {
                return assistant::js::getUndefined();
            }

            auto value = assistant::js::to_string(ctx, argv[0]);
            if (!value.has_value()) {
                return assistant::js::getUndefined();
            }
            auto count = control->GetCount();
            for (int i = 0; i < count; i++) {
                wxClientData *rawData = control->GetClientObject(i);
                if (rawData) {
                    wxStringClientData *stringData = dynamic_cast<wxStringClientData *>(rawData);
                    if (stringData) {
                        std::string value_index = stringData->GetData().ToStdString();
                        control->SetSelection(i);
                        return assistant::js::getUndefined();
                    }
                }
            }

            return assistant::js::getUndefined();
        }
    }
}