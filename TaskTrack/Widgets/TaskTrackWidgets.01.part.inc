#include "TaskTrackWidgets.h"

#include <stdio.h>
#include <math.h>

namespace Upp {

namespace {

UiButton::Style CompactButtonStyle(UiRole role = UiRole::Standard)
{
    UiButton::Style style = UiTheme::ResolveButton(role);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    return style;
}

UiLabel::Style CompactLabelStyle(UiRole role = UiRole::Standard, int px = 9)
{
    UiLabel::Style style = UiTheme::ResolveLabel(role);
    style.font = SansSerifZ(px);
    return style;
}

UiDropdown::Style CompactDropdownStyle()
{
    UiDropdown::Style style = UiTheme::ResolveDropdown(UiRole::Standard);
    style.font = SansSerifZ(9);
    style.popup_item_style.font = SansSerifZ(9);
    style.popup_item_height = DPI(24);
    return style;
}

UiList::Style CompactListStyle()
{
    UiList::Style style = UiTheme::ResolveList(UiRole::Standard);
    style.font = SansSerifZ(9);
    style.row_height = DPI(23);
    style.h_padding = DPI(6);
    style.v_padding = DPI(3);
    style.row_radius = DPI(3);
    return style;
}

UiTree::Style CompactTreeStyle()
{
    UiTree::Style style = UiTheme::ResolveTree();
    style.font = SansSerifZ(9);
    style.row_height = DPI(23);
    style.h_padding = DPI(6);
    style.v_padding = DPI(3);
    style.row_radius = DPI(3);
    return style;
}

UiGroupPanel::Style CompactQuestionStyle()
{
    UiGroupPanel::Style style = UiTheme::ResolveGroupPanel(UiRole::Standard);
    style.title_font = SansSerifZ(10).Bold();
    style.subtitle_font = SansSerifZ(8);
    style.metrics.radius = DPI(6);
    style.metrics.frame_width = DPI(1);
    style.header_inset = Rect(DPI(7), DPI(4), DPI(7), DPI(3));
    style.inset = Rect(DPI(7), DPI(6), DPI(7), DPI(7));
    style.header_gap = DPI(2);
    style.title_subtitle_gap = DPI(1);
    style.header_band_enabled = false;
    style.line_enabled = false;
    return style;
}

String DisplayNumber(double value)
{
    if(fabs(value - floor(value + 0.5)) < 0.000001)
        return Format("%.0f", value);
    return Format("%.2f", value);
}

bool IsDarkColor(Color c)
{
    return (c.GetR() * 299 + c.GetG() * 587 + c.GetB() * 114) < 128000;
}

} // namespace

// -----------------------------------------------------------------------------
// TaskTrackPosition9
// -----------------------------------------------------------------------------

TaskTrackPosition9::TaskTrackPosition9()
{
    static const char *labels[] = { "↖", "↑", "↗", "←", "•", "→", "↙", "↓", "↘" };
    for(int i = 0; i < 9; ++i) {
        UiButton& button = buttons_.Add();
        Add(button);
        button.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        button.SetText(labels[i]).SetCheckable(true).SetContentInset(DPI(2));
        button.WhenAction = [=] {
            value_ = ValueAt(i);
            Sync();
            WhenAction();
        };
    }
}

String TaskTrackPosition9::ValueAt(int index) const
{
    static const char *values[] = {
        "top_left", "top", "top_right",
        "left", "center", "right",
        "bottom_left", "bottom", "bottom_right"
    };
    return index >= 0 && index < 9 ? String(values[index]) : String();
}

TaskTrackPosition9& TaskTrackPosition9::SetValue(const String& value)
{
    value_ = value;
    Sync();
    return *this;
}

void TaskTrackPosition9::Sync()
{
    for(int i = 0; i < buttons_.GetCount(); ++i)
        buttons_[i].SetChecked(ValueAt(i) == value_);
}

Size TaskTrackPosition9::GetMinSize() const
{
    return Size(DPI(94), DPI(94));
}

void TaskTrackPosition9::Layout()
{
    Rect r = GetSize();
    int gap = DPI(3);
    int cell = min((r.GetWidth() - 2 * gap) / 3, (r.GetHeight() - 2 * gap) / 3);
    cell = max(cell, DPI(24));
    int total = 3 * cell + 2 * gap;
    int ox = max(0, (r.GetWidth() - total) / 2);
    int oy = max(0, (r.GetHeight() - total) / 2);
    for(int i = 0; i < 9; ++i)
        buttons_[i].SetRect(ox + (i % 3) * (cell + gap), oy + (i / 3) * (cell + gap), cell, cell);
}

// -----------------------------------------------------------------------------
// TaskTrackDirection8
// -----------------------------------------------------------------------------

TaskTrackDirection8::TaskTrackDirection8()
{
    static const char *labels[] = { "NW", "N", "NE", "W", "•", "E", "SW", "S", "SE" };
    for(int i = 0; i < 9; ++i) {
        UiButton& button = buttons_.Add();
        Add(button);
        button.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        button.SetText(labels[i]).SetCheckable(i != 4).SetContentInset(DPI(2));
        if(i == 4)
            button.Disable();
        else
            button.WhenAction = [=] {
                value_ = ValueAt(i);
                Sync();
                WhenAction();
            };
    }
}

String TaskTrackDirection8::ValueAt(int index) const
{
    static const char *values[] = {
        "north_west", "north", "north_east",
        "west", "", "east",
        "south_west", "south", "south_east"
    };
    return index >= 0 && index < 9 ? String(values[index]) : String();
}

TaskTrackDirection8& TaskTrackDirection8::SetValue(const String& value)
{
    value_ = value;
    Sync();
    return *this;
}

void TaskTrackDirection8::Sync()
{
    for(int i = 0; i < buttons_.GetCount(); ++i)
        if(i != 4)
            buttons_[i].SetChecked(ValueAt(i) == value_);
}

Size TaskTrackDirection8::GetMinSize() const
{
    return Size(DPI(104), DPI(94));
}

void TaskTrackDirection8::Layout()
{
    Rect r = GetSize();
    int gap = DPI(3);
    int cell = min((r.GetWidth() - 2 * gap) / 3, (r.GetHeight() - 2 * gap) / 3);
    cell = max(cell, DPI(24));
    int total = 3 * cell + 2 * gap;
    int ox = max(0, (r.GetWidth() - total) / 2);
    int oy = max(0, (r.GetHeight() - total) / 2);
    for(int i = 0; i < 9; ++i)
        buttons_[i].SetRect(ox + (i % 3) * (cell + gap), oy + (i / 3) * (cell + gap), cell, cell);
}

// -----------------------------------------------------------------------------
// TaskTrackRangeSelector
// -----------------------------------------------------------------------------

TaskTrackRangeSelector::TaskTrackRangeSelector()
{
    WantFocus();
}
