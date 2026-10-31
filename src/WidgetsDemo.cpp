#include "WidgetsDemo.h"

#include <deki/Time.h>

#include <cmath>
#include <cstdio>

void WidgetsDemo::Start()
{
    if (auto* s = slider.Get())
    {
        s->AddOnValueChangedCallback(
            [this](float value)
            {
                if (auto* b = bar.Get())
                {
                    b->SetValue(value);
                }
            });
        if (auto* b = bar.Get())
        {
            b->SetValue(s->value);
        }
    }
    if (auto* toggle = lightSwitch.Get())
    {
        toggle->AddOnToggledCallback(
            [this](bool on)
            {
                if (auto* l = led.Get())
                {
                    l->SetOn(on);
                }
            });
        if (auto* l = led.Get())
        {
            l->SetOn(toggle->isOn);
        }
    }
    if (auto* a = arc.Get())
    {
        a->AddOnValueChangedCallback(
            [this](float value)
            {
                if (auto* l = led.Get())
                {
                    l->SetBrightness(value / 100.0f);
                }
            });
    }
    if (auto* button = dialogButton.Get())
    {
        button->AddOnClickCallback(
            [this]()
            {
                if (auto* d = dialog.Get())
                {
                    d->Open();
                }
            });
    }
    if (auto* d = dialog.Get())
    {
        d->AddOnClosedCallback(
            [this](int32_t index)
            {
                auto* text = status.Get();
                auto* d2 = dialog.Get();
                if (!text || !d2)
                {
                    return;
                }
                char line[64];
                if (index >= 0 && index < static_cast<int32_t>(d2->buttons.size()))
                {
                    std::snprintf(line, sizeof(line), "You chose %s", d2->buttons[index].c_str());
                }
                else
                {
                    std::snprintf(line, sizeof(line), "Cancelled");
                }
                text->SetText(line);
            });
    }
}

void WidgetsDemo::Update()
{
    auto* c = chart.Get();
    if (!c)
    {
        return;
    }
    // Hidden behind another tab, the chart still keeps time, so it shows a
    // full trace when its tab opens.
    m_SinceChart += Deki::Time::GetDeltaTimeF() / 1000.0f;
    while (m_SinceChart >= chartInterval && chartInterval > 0.0f)
    {
        m_SinceChart -= chartInterval;
        m_Wave += 0.35f;
        c->AddPoint(50.0f + 30.0f * std::sin(m_Wave) + 12.0f * std::sin(m_Wave * 2.7f));
    }
}
