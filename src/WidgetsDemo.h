#pragma once

#include <deki/Engine.h>
#include <deki/reflection/ObjectRef.h>

#include "ArcComponent.h"
#include "BarComponent.h"
#include "ButtonComponent.h"
#include "ChartComponent.h"
#include "LedComponent.h"
#include "MsgboxComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "TextComponent.h"

// Wires the widgets scene together: the slider drives the bar, the switch
// lights the LED, the chart plots a wave as it goes, and a button opens the
// dialog, whose answer shows in the status line.
DEKI_CATEGORY("Demo")
DEKI_DESCRIPTION("Connects the widgets scene: slider to bar, switch to LED, a "
                 "live chart and a dialog.")
class WidgetsDemo : public Deki::Component
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("Moves the bar.")
    Deki::ObjectRef<Deki2D::SliderComponent> slider;

    DEKI_EXPORT
    DEKI_TOOLTIP("Follows the slider.")
    Deki::ObjectRef<Deki2D::BarComponent> bar;

    DEKI_EXPORT
    DEKI_TOOLTIP("Turns the LED on and off.")
    Deki::ObjectRef<Deki2D::SwitchComponent> lightSwitch;

    DEKI_EXPORT
    DEKI_TOOLTIP("Lit by the switch, as bright as the arc is turned.")
    Deki::ObjectRef<Deki2D::LedComponent> led;

    DEKI_EXPORT
    DEKI_TOOLTIP("Sets the LED's brightness.")
    Deki::ObjectRef<Deki2D::ArcComponent> arc;

    DEKI_EXPORT
    DEKI_TOOLTIP("Plots a wave, a point every few frames.")
    Deki::ObjectRef<Deki2D::ChartComponent> chart;

    DEKI_EXPORT
    DEKI_TOOLTIP("Opens the dialog.")
    Deki::ObjectRef<Deki2D::ButtonComponent> dialogButton;

    DEKI_EXPORT
    DEKI_TOOLTIP("The dialog the button opens.")
    Deki::ObjectRef<Deki2D::MsgboxComponent> dialog;

    DEKI_EXPORT
    DEKI_TOOLTIP("Says what the dialog answered.")
    Deki::ObjectRef<Deki2D::TextComponent> status;

    DEKI_EXPORT
    DEKI_TOOLTIP("Seconds between chart points.")
    float chartInterval = 0.15f;

    WidgetsDemo() = default;

    void Start() override;
    void Update() override;

private:
    float m_SinceChart = 0.0f;
    float m_Wave = 0.0f;
};
