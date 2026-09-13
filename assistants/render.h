#include <wx/wx.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include "wxFrameData.h"

extern "C"
{
#include <quickjs.h>
}

namespace render
{
    /** Получить растяжения */
    std::optional<int> getGrowable(std::optional<std::string> text)
    {
        if (!text.has_value())
        {
            return std::nullopt;
        }
        auto textValue = text.value();
        if (textValue == "*")
        {
            return 1;
        }
        else
        {
            auto growable = assistant::core::replaceAll(textValue, "*", "");
            if (growable.size() != textValue.size())
            {
                auto growableInt = assistant::core::to_int(growable);
                if (growableInt.has_value())
                {
                    return growableInt.value();
                }
            }
        }

        return std::nullopt;
    }

    namespace base
    {
        wxSizer* apply(wxSizer *sizer, JSContext *ctx, JSValue instance)
        {
            auto clientData = new JSValueClientData(ctx, instance);
            sizer->SetClientData(clientData);

            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(sizer);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, instance, "__wx_ptr", ptr_value);


            return sizer;
        }

        JSValue getValue(wxWindow* control)
        {
            if (!control)
            {
                return assistant::js::getUndefined();
            }

            wxClientData* clientDataVoid = control->GetClientObject();
            JSValueClientData* clientData = dynamic_cast<JSValueClientData*>(clientDataVoid);
            if (!clientData)
            {
                return assistant::js::getUndefined();
            }

            return clientData->getValue();
        }
        
        wxWindow* apply(wxWindow *control, JSContext *ctx, JSValue instance)
        {
            if (!control)
            {
                return control;
            }

            auto widthInt = assistant::js::to_int(ctx, instance, "width");
            auto heightInt = assistant::js::to_int(ctx, instance, "height");
            if (widthInt.has_value() || heightInt.has_value())
            {
                control->SetSize(
                    widthInt.value_or(control->GetSize().GetWidth()),
                    heightInt.value_or(control->GetSize().GetHeight())
                );
            }

            auto name = assistant::js::to_string(ctx, instance, "name");
            if (name.has_value())
            {
                control->SetName(wxString::FromUTF8(name.value()));
            }
            auto id = assistant::js::to_int(ctx, instance, "id");
            if (id.has_value())
            {
                control->SetId(id.value());
            }

            auto clientData = new JSValueClientData(ctx, instance);
            control->SetClientObject(clientData);


            // Устанавливаем адрес ссылки в объект, в служебное поле
            int64_t address = reinterpret_cast<int64_t>(control);
            JSValue ptr_value = JS_NewInt64(ctx, address);
            JS_SetPropertyStr(ctx, instance, "__wx_ptr", ptr_value);

            return control;
        }
    }

    namespace comboBox
    {
        wxComboBox *create(wxWindow* parent)
        {
            return new wxComboBox(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
        }

        void setItems(wxComboBox* control, JSContext *ctx, JSValue items)
        {
            control->Clear();
            if (!JS_IsArray(ctx, items))
            {
                return;
            }

            auto length = assistant::js::to_int(ctx, items, "length").value_or(0);

            for (int i = 0; i < length; i++)
            {
                JSValue item = JS_GetPropertyUint32(ctx, items, i);
                auto text = assistant::js::to_string(ctx, item, "text").value_or("");
                auto value = assistant::js::to_string(ctx, item, "value").value_or("");
                control->Append(wxString::FromUTF8(text.c_str()), new wxStringClientData(wxString::FromUTF8(value.c_str())));
                JS_FreeValue(ctx, item);
            }
        }

        void setSelectedIndex(wxComboBox* control, JSContext *ctx, JSValue selectedIndexValue)
        {
            auto selectedIndex = assistant::js::to_int(ctx, selectedIndexValue).value_or(0);
            if (control->GetCount() > 0 && selectedIndex < control->GetCount())
            {
                control->SetSelection(selectedIndex);
            }
        }

        void apply(wxComboBox* control, JSContext *ctx, JSValue instance)
        {
            auto items = assistant::js::getValue(ctx, instance, "items");
            render::comboBox::setItems(control, ctx, items);
            JS_FreeValue(ctx, items);

            auto selectedIndex = assistant::js::getValue(ctx, instance, "selectedIndex");
            render::comboBox::setSelectedIndex(control, ctx, selectedIndex);
            JS_FreeValue(ctx, selectedIndex);
        }
    }

    namespace label
    {
        wxStaticText *create(wxWindow *parent)
        {
            return new wxStaticText(parent, wxID_ANY, "");
        }

        void apply(wxStaticText *label, JSContext *ctx, JSValue instance)
        {
            auto text = assistant::js::to_string(ctx, instance, "text").value_or("");
            label->SetLabel(wxString::FromUTF8(text));
        }
    }

    namespace text
    {
        wxTextCtrl *create(wxWindow* parent, bool isMultiline = false)
        {
            return !isMultiline 
                ? new wxTextCtrl(parent, wxID_ANY, "")
                : new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,  wxTE_MULTILINE | wxTE_WORDWRAP);
        }

        void apply(wxTextCtrl *textControl, JSContext *ctx, JSValue instance)
        {
            auto text = assistant::js::to_string(ctx, instance, "text").value_or("");
            textControl->SetValue(wxString::FromUTF8(text));

            auto placeholder = assistant::js::to_string(ctx, instance, "placeholder").value_or("");
            if (!placeholder.empty()) 
            {
                textControl->SetValue(wxString::FromUTF8(placeholder));
            }
        }
    }

    namespace button
    {
        wxButton *create(wxWindow *parent)
        {
            return new wxButton(parent, wxID_ANY);
        }

        void apply(wxButton *button, JSContext *ctx, JSValue instance)
        {
            auto text = assistant::js::to_string(ctx, instance, "text").value_or("");
            button->SetLabel(wxString::FromUTF8(text));

            auto click = assistant::js::getValue(ctx, instance, "click");
            if (JS_IsFunction(ctx, click))
            {
                button->Bind(wxEVT_BUTTON, [](wxCommandEvent &event)
                {
                    wxWindow* control = dynamic_cast<wxWindow*>(event.GetEventObject());
                    if (control) 
                    {
                        wxClientData* clientDataVoid = control->GetClientObject();
                        JSValueClientData* clientData = dynamic_cast<JSValueClientData*>(clientDataVoid);
                        if (clientData)
                        {
                            auto instance = clientData->getValue();
                            auto context = clientData->getContext();
                            JSValue argv[1] = 
                            { 
                                instance
                            };
                            auto click = assistant::js::getValue(context, instance, "click");
                            if (JS_IsFunction(context, click))
                            {
                                JSValue ret = assistant::js::call(context, click, assistant::js::getUndefined(), 1, argv);
                                JS_FreeValue(context, ret);
                            }
                            
                            JS_FreeValue(context, click);
                        }
                    }

                    event.Skip(); 
                });
            }

            JS_FreeValue(ctx, click);
        }
    }

    namespace gridBag
    {
        wxGridBagSizer *create(int gap = 5)
        {
            return new wxGridBagSizer(gap, gap);
        }

        void apply(wxGridBagSizer *grid, JSContext *ctx, JSValue items, wxWindow *parent)
        {
            if (!JS_IsArray(ctx, items) || !grid)
            {
                return;
            }

            auto length = assistant::js::to_int(ctx, items, "length").value_or(0);
            for (int i = 0; i < length; i++)
            {
                JSValue item = JS_GetPropertyUint32(ctx, items, i);
                auto className = assistant::js::getClassName(ctx, item);

                wxWindow *control = nullptr;
                wxSizer *sizer = nullptr;

                auto row = assistant::js::to_int(ctx, item, "row").value_or(0);
                auto rowSpan = assistant::js::to_int(ctx, item, "rowSpan").value_or(1);
                auto column = assistant::js::to_int(ctx, item, "column").value_or(0);
                auto columnSpan = assistant::js::to_int(ctx, item, "columnSpan").value_or(1);

                auto widthString = assistant::js::to_string(ctx, item, "width");
                auto heightString = assistant::js::to_string(ctx, item, "height");

                if (className == "Button")
                {
                    auto button = render::button::create(parent);
                    control = render::base::apply(button, ctx, item);
                    render::button::apply(button, ctx, item);
                }
                if (className == "Label")
                {
                    auto label = render::label::create(parent);
                    control = render::base::apply(label, ctx, item);
                    render::label::apply(label, ctx, item);
                }
                if (className == "Text")
                {
                    auto multiline = assistant::js::to_bool(ctx, item, "multiline").value_or(false);
                    auto text = render::text::create(parent, multiline);
                    control = render::base::apply(text, ctx, item);
                    render::text::apply(text, ctx, item);
                }
                if (className == "ComboBox")
                {
                    auto comboBox = render::comboBox::create(parent);
                    control = render::base::apply(comboBox, ctx, item);
                    render::comboBox::apply(comboBox, ctx, item);
                }
                if (className == "GridBagLayout")
                {
                    auto gridBag = render::gridBag::create();
                    sizer = render::base::apply(gridBag, ctx, item);
                    render::gridBag::apply(gridBag, ctx, item, parent);
                }

                if (control)
                {
                    grid->Add(control, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                }
                else if (sizer)
                {
                    grid->Add(sizer, wxGBPosition(row, column), wxGBSpan(rowSpan, columnSpan), wxEXPAND | wxALL, 0);
                }
                else 
                {
                    JS_FreeValue(ctx, item);
                }

                if (columnSpan == 1)
                {
                    auto growable = render::getGrowable(widthString);
                    if (growable.has_value())
                    {
                        if (!grid->IsColGrowable(column)) 
                        {
                            grid->AddGrowableCol(column, growable.value());
                        }
                    }
                }
                if (rowSpan == 1)
                {
                    auto growable = render::getGrowable(heightString);
                    if (growable.has_value())
                    {
                        if (!grid->IsRowGrowable(column)) 
                        {
                            grid->AddGrowableRow(row, growable.value());
                        }
                    }
                }
            }
        }
    }

    namespace frame
    {
        wxFrameData *getFrameData(wxFrame *frame)
        {
            if (!frame)
            {
                return nullptr;
            }

            auto boxSizer = reinterpret_cast<wxBoxSizer *>(frame->GetSizer());
            if (!boxSizer)
            {
                return nullptr;
            }

            auto count = boxSizer->GetChildren().GetCount();
            if (count < 1)
            {
                return nullptr;
            }

            wxPanel *panel = nullptr;
            auto items = boxSizer->GetChildren();
            auto node = items.Item(0);
            wxSizerItem* item = node ? node->GetData() : nullptr;
            if (item && item->IsWindow())
            {
                panel = dynamic_cast<wxPanel *>(item->GetWindow());
            }

            if (!panel)
            {
                return nullptr;
            }
            auto data = reinterpret_cast<wxFrameData *>(panel->GetClientObject());
            return data;
        }

        void apply(wxFrame *frame, JSContext *ctx, JSValue instance)
        {
            if (!JS_IsObject(instance) || !frame)
            {
                return;
            }
            auto title = assistant::js::to_string(ctx, instance, "title");
            if (title.has_value())
            {
                frame->SetTitle(wxString::FromUTF8(title.value()));
            }

            auto name = assistant::js::to_string(ctx, instance, "name");
            if (name.has_value())
            {
                frame->SetName(wxString::FromUTF8(name.value()));
            }

            auto width = assistant::js::to_int(ctx, instance, "width");
            auto height = assistant::js::to_int(ctx, instance, "height");
            if (width.has_value() || height.has_value())
            {
                auto widthValue = width.value_or(frame->GetSize().GetWidth());
                auto heightValue = height.value_or(frame->GetSize().GetHeight());
                frame->SetSize(widthValue, heightValue);
            }

            wxPanel* panel = new wxPanel(frame, wxID_ANY);
            auto frameData = new wxFrameData();
            panel->SetClientObject(frameData);
            
            wxBoxSizer* frame_sizer = new wxBoxSizer(wxVERTICAL);
            frame_sizer->Add(panel, 1, wxEXPAND, 0);
            frame->SetSizer(frame_sizer);

            wxBoxSizer* panel_sizer = new wxBoxSizer(wxVERTICAL);
            panel->SetSizer(panel_sizer);
            wxGauge* progressBar = new wxGauge(panel, wxID_ANY, 100,  wxDefaultPosition, wxSize(-1, 3),  wxGA_HORIZONTAL | wxGA_SMOOTH);
            panel_sizer->Add(progressBar, 0, wxEXPAND, 0);
            panel_sizer->Detach(progressBar);

            auto grid = render::gridBag::create();
            auto items = assistant::js::getValue(ctx, instance, "items");
            render::gridBag::apply(grid, ctx, items, panel);
            JS_FreeValue(ctx, items);
            panel_sizer->Add(grid, 1, wxEXPAND, 0);
            auto debugPanel = new wxDebugPanel(panel, wxID_ANY);
            panel_sizer->Add(debugPanel, 0, wxEXPAND, 0);
            
            
            frame->Layout();
            
            frameData->setProgressBar(progressBar);
            frameData->setPanelSizer(panel_sizer);
            frameData->setPanel(panel);
            frameData->setDebugPanel(debugPanel);
        }
    }
}