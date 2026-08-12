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

TaskTrackRangeSelector& TaskTrackRangeSelector::SetRange(double mn, double mx)
{
    if(mx < mn)
        Swap(mn, mx);
    mn_ = mn;
    mx_ = mx;
    low_ = ClampSnap(low_);
    high_ = ClampSnap(high_);
    if(low_ > high_)
        low_ = high_;
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetStep(double step)
{
    step_ = max(step, 0.000001);
    low_ = ClampSnap(low_);
    high_ = ClampSnap(high_);
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetValues(double low, double high)
{
    low_ = ClampSnap(low);
    high_ = ClampSnap(high);
    if(low_ > high_)
        Swap(low_, high_);
    Refresh();
    return *this;
}

TaskTrackRangeSelector& TaskTrackRangeSelector::SetUnit(const String& unit)
{
    unit_ = unit;
    Refresh();
    return *this;
}

Size TaskTrackRangeSelector::GetMinSize() const
{
    return Size(DPI(230), DPI(48));
}

double TaskTrackRangeSelector::ClampSnap(double value) const
{
    value = minmax(value, mn_, mx_);
    if(step_ > 0.0) {
        double n = floor(((value - mn_) / step_) + 0.5);
        value = mn_ + n * step_;
    }
    return minmax(value, mn_, mx_);
}

int TaskTrackRangeSelector::XFromValue(double value) const
{
    Rect r = GetSize();
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    double span = max(0.000001, mx_ - mn_);
    double t = (value - mn_) / span;
    return left + (int)floor(t * (right - left) + 0.5);
}

double TaskTrackRangeSelector::ValueFromX(int x) const
{
    Rect r = GetSize();
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    double t = (double)(minmax(x, left, right) - left) / (double)(right - left);
    return ClampSnap(mn_ + t * (mx_ - mn_));
}

int TaskTrackRangeSelector::HitThumb(Point p) const
{
    int y = DPI(18);
    int r = DPI(9);
    int lx = XFromValue(low_);
    int hx = XFromValue(high_);
    int dl = abs(p.x - lx) + abs(p.y - y);
    int dh = abs(p.x - hx) + abs(p.y - y);
    if(dl <= r * 2 || dh <= r * 2)
        return dl <= dh ? 0 : 1;
    return abs(p.x - lx) <= abs(p.x - hx) ? 0 : 1;
}

void TaskTrackRangeSelector::SetDraggedValue(Point p, bool fire_changing)
{
    double value = ValueFromX(p.x);
    if(dragging_ == 0)
        low_ = min(value, high_);
    else if(dragging_ == 1)
        high_ = max(value, low_);
    Refresh();
    if(fire_changing)
        WhenChanging();
}

void TaskTrackRangeSelector::Paint(Draw& w)
{
    Rect r = GetSize();
    Color ink = SColorText();
    Color track = Blend(SColorShadow(), SColorFace(), 130);
    Color accent = Color(0, 120, 212);
    int y = DPI(18);
    int left = DPI(10);
    int right = max(left + 1, r.GetWidth() - DPI(10));
    int lx = XFromValue(low_);
    int hx = XFromValue(high_);

    w.DrawRect(left, y - DPI(2), right - left, DPI(4), track);
    w.DrawRect(lx, y - DPI(2), max(1, hx - lx), DPI(4), accent);

    Rect lr = RectC(lx - DPI(6), y - DPI(6), DPI(12), DPI(12));
    Rect hr = RectC(hx - DPI(6), y - DPI(6), DPI(12), DPI(12));
    w.DrawEllipse(lr, selected_ == 0 ? accent : SColorFace(), DPI(2), accent);
    w.DrawEllipse(hr, selected_ == 1 ? accent : SColorFace(), DPI(2), accent);

    Font f = SansSerifZ(9);
    String low_text = DisplayNumber(low_) + unit_;
    String high_text = DisplayNumber(high_) + unit_;
    w.DrawText(left, DPI(30), low_text, f, ink);
    Size hs = GetTextSize(high_text, f);
    w.DrawText(max(left, right - hs.cx), DPI(30), high_text, f, ink);
}

void TaskTrackRangeSelector::LeftDown(Point p, dword)
{
    SetFocus();
    selected_ = dragging_ = HitThumb(p);
    SetCapture();
    SetDraggedValue(p, true);
}

void TaskTrackRangeSelector::LeftUp(Point p, dword)
{
    if(dragging_ >= 0)
        SetDraggedValue(p, false);
    dragging_ = -1;
    ReleaseCapture();
    WhenAction();
}

void TaskTrackRangeSelector::MouseMove(Point p, dword)
{
    if(dragging_ >= 0)
        SetDraggedValue(p, true);
}

void TaskTrackRangeSelector::LostFocus()
{
    dragging_ = -1;
    ReleaseCapture();
    Refresh();
}

bool TaskTrackRangeSelector::Key(dword key, int count)
{
    if(key != K_LEFT && key != K_RIGHT)
        return Ctrl::Key(key, count);
    double delta = (key == K_LEFT ? -step_ : step_) * max(1, count);
    if(selected_ == 0)
        low_ = min(ClampSnap(low_ + delta), high_);
    else
        high_ = max(ClampSnap(high_ + delta), low_);
    Refresh();
    WhenChanging();
    WhenAction();
    return true;
}

// -----------------------------------------------------------------------------
// TaskTrackGradientSelector
// -----------------------------------------------------------------------------

TaskTrackGradientSelector::TaskTrackGradientSelector()
{
    WantFocus();
}

TaskTrackGradientSelector& TaskTrackGradientSelector::SetOptions(const Vector<TaskTrackGradientOption>& options)
{
    options_ = clone(options);
    Refresh();
    return *this;
}

TaskTrackGradientSelector& TaskTrackGradientSelector::SetValue(const String& value)
{
    value_ = value;
    Refresh();
    return *this;
}

Size TaskTrackGradientSelector::GetMinSize() const
{
    int count = max(1, options_.GetCount());
    int cols = min(3, count);
    int rows = (count + cols - 1) / cols;
    return Size(DPI(250), rows * DPI(42));
}

Rect TaskTrackGradientSelector::OptionRect(int index) const
{
    Rect r = GetSize();
    int count = max(1, options_.GetCount());
    int cols = min(3, count);
    int rows = (count + cols - 1) / cols;
    int gap = DPI(5);
    int cell_w = max(DPI(60), (r.GetWidth() - (cols - 1) * gap) / cols);
    int cell_h = max(DPI(36), (r.GetHeight() - (rows - 1) * gap) / rows);
    int row = index / cols;
    int col = index % cols;
    return RectC(col * (cell_w + gap), row * (cell_h + gap), cell_w, cell_h);
}

Color TaskTrackGradientSelector::ParseColor(const String& text, Color fallback) const
{
    String s = text;

    if(s.StartsWith("#"))
        s = s.Mid(1);
    unsigned r = 0, g = 0, b = 0;
    if(s.GetCount() >= 6 && sscanf(~s, "%02x%02x%02x", &r, &g, &b) == 3)
        return Color((int)r, (int)g, (int)b);
    return fallback;
}

void TaskTrackGradientSelector::PaintGradient(Draw& w, const Rect& r, Color a, Color b) const
{
    int strips = max(8, min(48, r.GetWidth()));
    for(int i = 0; i < strips; ++i) {
        int x1 = r.left + i * r.GetWidth() / strips;
        int x2 = r.left + (i + 1) * r.GetWidth() / strips;
        Color c = Blend(a, b, i * 255 / max(1, strips - 1));
        w.DrawRect(x1, r.top, max(1, x2 - x1), r.GetHeight(), c);
    }
}

void TaskTrackGradientSelector::Paint(Draw& w)
{
    w.DrawRect(Rect(GetSize()), SColorFace());
    for(int i = 0; i < options_.GetCount(); ++i) {
        Rect cell = OptionRect(i);
        Rect preview = cell;
        preview.bottom -= DPI(14);
        PaintGradient(w, preview, ParseColor(options_[i].from_color, Black()), ParseColor(options_[i].to_color, White()));
        Color frame = options_[i].id == value_ ? Color(0, 120, 212) : Blend(SColorShadow(), SColorFace(), 90);
        int thickness = options_[i].id == value_ ? DPI(2) : DPI(1);
        w.DrawRect(cell.left, cell.top, cell.GetWidth(), thickness, frame);
        w.DrawRect(cell.left, cell.bottom - thickness, cell.GetWidth(), thickness, frame);
        w.DrawRect(cell.left, cell.top, thickness, cell.GetHeight(), frame);
        w.DrawRect(cell.right - thickness, cell.top, thickness, cell.GetHeight(), frame);
        String label = options_[i].label.IsEmpty() ? options_[i].id : options_[i].label;
        w.DrawText(cell.left + DPI(4), cell.bottom - DPI(13), label, SansSerifZ(8), SColorText());
    }
}

int TaskTrackGradientSelector::HitOption(Point p) const
{
    for(int i = 0; i < options_.GetCount(); ++i)
        if(OptionRect(i).Contains(p))
            return i;
    return -1;
}

void TaskTrackGradientSelector::LeftDown(Point p, dword)
{
    int index = HitOption(p);
    if(index < 0)
        return;
    value_ = options_[index].id;
    Refresh();
    WhenAction();
}

// -----------------------------------------------------------------------------
// TaskTrackQuestionCtrl
// -----------------------------------------------------------------------------

TaskTrackQuestionCtrl::TaskTrackQuestionCtrl()
{
    SetCustomStyle(CompactQuestionStyle());
    SetHeaderMode(UiGroupPanel::Inside);
    SetContent(content_);
    content_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(DPI(8));
    response_.SetDirection(UiDirection::H)
        .SetGap(DPI(5), DPI(5))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);
    recommendation_.SetCustomStyle(CompactLabelStyle(UiRole::Accent, 8));
    recommendation_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    dropdown_.SetCustomStyle(CompactDropdownStyle());
    dropdown_.SetSizeMin(DPI(160), DPI(27));
    list_.SetCustomStyle(CompactListStyle());
    rank_.SetCustomStyle(CompactListStyle());
    tree_.SetCustomStyle(CompactTreeStyle());

    text_.SetPlaceholder("Enter response");
    notes_.SetPlaceholder("Optional note, exception, constraint, or instruction…");
    number_.ShowSpin(true);
    amount_.SetFieldWidth(DPI(66)).SetGap(DPI(5));
    range_.SetInset(0); // The question body owns the shared card inset.
    unit_label_.SetCustomStyle(CompactLabelStyle(UiRole::Subtle, 8));
    unit_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    rank_confirm_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
    rank_confirm_.SetText("Use this order").SetContentInset(DPI(4));
    curve_confirm_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
    curve_confirm_.SetText("Use curve").SetContentInset(DPI(4));

    custom_color_.SetLabelStyle(CompactLabelStyle(UiRole::Subtle, 8));
    custom_color_.SetValueStyle(CompactLabelStyle(UiRole::Subtle, 8));
    custom_color_.SetLabel("Custom").SetColorCount(1).ShowValue(true).SetLabelWidth(DPI(48)).SetValueWidth(DPI(64));

}

void TaskTrackQuestionCtrl::Bind(TaskTrackDocument& document, int item_index)
{
    document_ = &document;
    item_index_ = item_index;
    Configure();
    SyncFromModel();
}

void TaskTrackQuestionCtrl::ClearDynamicControls()
{
    content_.ClearItems();
    response_.ClearItems();
    radios_.Clear();
    checks_.Clear();
    choice_buttons_.Clear();
    color_buttons_.Clear();
    dropdown_.UseInternalModel().Clear();
    list_.GetInternalModel().Clear();
    rank_.GetInternalModel().Clear();
    tree_.GetInternalModel().Clear();
}

void TaskTrackQuestionCtrl::Configure()
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    ClearDynamicControls();
    TaskTrackItem& item = document_->items[item_index_];
    String title = item.title + (item.required ? "  *" : "");
    SetTitle(title);
    SetSubTitle(item.instruction);

    if(!item.recommended.IsEmpty()) {
        recommendation_.SetText("Agent suggests: " + item.recommended);
        content_.Add(recommendation_).Fit().MinCross(DPI(17)).AlignSelf(UiBoxLayout::Align::Stretch);
    }

    switch(item.type) {
    case TaskTrackItemType::Confirm:         BuildConfirm(); break;
    case TaskTrackItemType::SingleChoice:    BuildSingleChoice(); break;
    case TaskTrackItemType::MultiChoice:     BuildMultiChoice(); break;
    case TaskTrackItemType::Select:          BuildSelect(); break;
    case TaskTrackItemType::ListSelect:      BuildListSelect(); break;
    case TaskTrackItemType::Text:            BuildText(); break;
    case TaskTrackItemType::Notes:           BuildNotes(); break;
    case TaskTrackItemType::Number:          BuildNumber(); break;
    case TaskTrackItemType::Amount:          BuildAmount(); break;
    case TaskTrackItemType::Range:           BuildRange(); break;
    case TaskTrackItemType::Rating:          BuildRating(); break;
    case TaskTrackItemType::Color:           BuildColor(); break;
    case TaskTrackItemType::Gradient:        BuildGradient(); break;
    case TaskTrackItemType::Position:        BuildPosition(); break;
    case TaskTrackItemType::Direction:       BuildDirection(); break;
    case TaskTrackItemType::RankOrder:       BuildRankOrder(); break;
    case TaskTrackItemType::HierarchySelect: BuildHierarchy(); break;
    case TaskTrackItemType::Curve:           BuildCurve(); break;
    }

    content_.RefreshLayout();
    RefreshLayout();
}

void TaskTrackQuestionCtrl::BuildConfirm()
{
    TaskTrackItem& item = document_->items[item_index_];
    int group = 1000 + item_index_;
    String labels[2] = { "Yes", "No" };
    if(item.choices.GetCount() == 2) {
        labels[0] = item.choices[0];
        labels[1] = item.choices[1];
    }
    const char *values[] = { "yes", "no" };
    bool pass_fail = TaskTrackIsPassFailConfirm(item);
    for(int i = 0; i < 2; ++i) {
        UiRadioButton& radio = radios_.Add();
        radio.SetText(labels[i]).SetGroup(group).SetVisual(UIRADIOVIS_PILLS).SetIndicatorSide(UiAlign::LEFT);
        if(pass_fail) {
            bool pass = TaskTrackConfirmLabelIsPass(labels[i]);
            Color c = pass ? AnsweredGreen() : EscalatedRed();
            UiRadioButton::Style st = radio.GetStyle();
            st.font = SansSerifZ(9);
            st.metrics.frame_enabled = true;
            st.metrics.frame_width = DPI(1);
            for(int k = 0; k < 4; ++k) {
                st.palette.frame[k] = c;
                st.palette.face[k] = UiFill::Solid(Blend(SColorPaper(), c, 8));
                st.palette.ink[k] = c;
            }
            radio.SetCustomStyle(st);
        }
        else {
            UiRadioButton::Style style = radio.GetStyle();
            style.font = SansSerifZ(9);
            radio.SetCustomStyle(style);
        }
        String label = labels[i];
        String value = pass_fail ? labels[i] : String(values[i]);
        Value data = pass_fail ? Value(TaskTrackConfirmLabelIsPass(labels[i])) : Value(i == 0);
        radio.WhenAction = [=] { CommitSimpleValue(label, value, data); };
        response_.Add(radio).Fit().MinMain(DPI(pass_fail ? 74 : 84)).MinCross(DPI(26));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
    if(pass_fail)
        BuildVerdictNote();
    (void)item;
}

void TaskTrackQuestionCtrl::BuildVerdictNote()
{
    verdict_note_label_.SetText("Note (optional)");
    UiLabel::Style ls = UiTheme::ResolveLabel(UiTheme::GetContext(), UiRole::Subtle, UiTextSize::Body);
    ls.font = SansSerifZ(8);
    verdict_note_label_.SetCustomStyle(ls);
    verdict_note_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    UiBaseEdit::Style es = UiTheme::ResolveEdit(UiTheme::GetContext(), UiRole::Standard);
    es.font = SansSerifZ(9);
    verdict_note_.SetCustomStyle(es);

    verdict_note_.WhenChange = [=] { CommitNote(); };
    verdict_note_row_.SetDirection(UiDirection::H).SetGap(DPI(4)).SetAlignItems(UiCrossAlign::Center);
    verdict_note_row_.Add(verdict_note_label_).Fit().MinCross(DPI(20));
    verdict_note_row_.Add(verdict_note_).Expand(1).MinCross(DPI(22)).MinMain(DPI(90));
    content_.Add(verdict_note_row_).Fit().MinCross(DPI(28)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::CommitNote()
{
    if(syncing_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;
    String text = verdict_note_.GetTextUtf8();
    TaskTrackAnswer& answer = document_->items[item_index_].answer;
    if(answer.note != text) {
        answer.note = text;
        WhenChanged();
    }
}

void TaskTrackQuestionCtrl::BuildSingleChoice()
{
    TaskTrackItem& item = document_->items[item_index_];
    if(item.choices.GetCount() <= 5) {
        int group = 2000 + item_index_;
        for(int i = 0; i < item.choices.GetCount(); ++i) {
            UiRadioButton& radio = radios_.Add();
            radio.SetText(item.choices[i]).SetGroup(group).SetVisual(UIRADIOVIS_PILLS);
            UiRadioButton::Style style = radio.GetStyle();
            style.font = SansSerifZ(9);
            radio.SetCustomStyle(style);
            String choice = item.choices[i];
            radio.WhenAction = [=] { CommitSimpleValue("selected", choice, choice); };
            response_.Add(radio).Fit().MinMain(DPI(78)).MinCross(DPI(26));
        }
    }
    else {
        for(const String& choice : item.choices)
            dropdown_.Add(choice, choice);
        dropdown_.WhenSelect = [=](int) { CommitCurrent(); };
        response_.Add(dropdown_).Expand(1).MinMain(DPI(170)).MinCross(DPI(27));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildMultiChoice()
{
    TaskTrackItem& item = document_->items[item_index_];
    for(int i = 0; i < item.choices.GetCount(); ++i) {
        UiCheckBox& check = checks_.Add();
        UiCheckBox::Style style = check.GetStyle();
        style.font = SansSerifZ(9);

        check.SetCustomStyle(style);
        check.SetText(item.choices[i]);
        check.WhenAction = [=] { CommitCurrent(); };
        response_.Add(check).Fit().MinMain(DPI(82)).MinCross(DPI(25));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildSelect()
{
    TaskTrackItem& item = document_->items[item_index_];
    for(const String& choice : item.choices)
        dropdown_.Add(choice, choice);
    dropdown_.WhenSelect = [=](int) { CommitCurrent(); };
    response_.Add(dropdown_).Expand(1).MinMain(DPI(180)).MinCross(DPI(27));
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::PopulateList(UiList& list, const Vector<String>& choices)
{
    UiListModel& model = list.GetInternalModel();
    model.Clear();
    for(const String& choice : choices)
        model.Add(choice, choice);
}

void TaskTrackQuestionCtrl::BuildListSelect()
{
    TaskTrackItem& item = document_->items[item_index_];
    PopulateList(list_, item.choices);
    list_.SetSelectionMode(item.allow_multiple ? UILISTSEL_MULTI : UILISTSEL_SINGLE);
    list_.EnableRenameOnDblClick(false).EnableDragReorder(false);
    list_.WhenSelection = [=] { CommitCurrent(); };
    content_.Add(list_).Fit().MinMain(DPI(92)).MaxMain(DPI(118)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildText()
{
    text_.SetPlaceholder("Enter response");
    text_.WhenChange = [=] { if(!syncing_) CommitCurrent(); };
    text_.WhenAction = [=] { if(!syncing_) CommitCurrent(); };
    response_.Add(text_).Expand(1).MinMain(DPI(180)).MinCross(DPI(27));
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildNotes()
{
    notes_.WhenChange = [=] { if(!syncing_) CommitCurrent(); };
    notes_.WhenAction = [=] { if(!syncing_) CommitCurrent(); };
    content_.Add(notes_).Fit().MinMain(DPI(68)).MaxMain(DPI(96)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildNumber()
{
    TaskTrackItem& item = document_->items[item_index_];
    if(item.has_min) number_.Min(item.min_value);
    if(item.has_max) number_.Max(item.max_value);
    number_.Step(item.step_value);
    number_.Precision(item.step_value >= 1.0 ? 0 : 2);
    number_.WhenChange = [=] { if(!syncing_) CommitCurrent(); };
    number_.WhenAction = [=] { if(!syncing_) CommitCurrent(); };
    response_.Add(number_).Fit().MinMain(DPI(120)).MinCross(DPI(27));
    if(!item.unit.IsEmpty()) {
        unit_label_.SetText(item.unit);
        response_.Add(unit_label_).Fit().MinMain(DPI(36)).MinCross(DPI(24));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildAmount()
{
    TaskTrackItem& item = document_->items[item_index_];
    double mn = item.has_min ? item.min_value : 0.0;
    double mx = item.has_max ? item.max_value : 100.0;
    amount_.SetRange(mn, mx).SetStep(item.step_value).SetFieldWidth(DPI(64));
    amount_.WhenChanging = [=] { if(!syncing_) CommitCurrent(); };
    amount_.WhenAction = [=] { if(!syncing_) CommitCurrent(); };
    response_.Add(amount_).Expand(1).MinMain(DPI(210)).MinCross(DPI(28));
    if(!item.unit.IsEmpty()) {
        unit_label_.SetText(item.unit);
        response_.Add(unit_label_).Fit().MinMain(DPI(32)).MinCross(DPI(24));
    }
    content_.Add(response_).Fit().MinCross(DPI(30)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildRange()
{
    TaskTrackItem& item = document_->items[item_index_];
    double mn = item.has_min ? item.min_value : 0.0;
    double mx = item.has_max ? item.max_value : 100.0;
    range_.SetRange(mn, mx).SetStep(item.step_value).SetUnit(item.unit);
    double low = mn + (mx - mn) * 0.25;
    double high = mn + (mx - mn) * 0.75;
    if(item.default_value.Is<ValueMap>()) {
        ValueMap def = item.default_value;
        if(IsNumber(def["low"])) low = (double)def["low"];
        if(IsNumber(def["high"])) high = (double)def["high"];
    }
    range_.SetValues(low, high);
    range_.WhenChanging = [=] { if(!syncing_) CommitCurrent(); };
    range_.WhenAction = [=] { if(!syncing_) CommitCurrent(); };
    content_.Add(range_).Fit().MinMain(DPI(48)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildRating()
{
    TaskTrackItem& item = document_->items[item_index_];
    int first = (int)floor(item.min_value + 0.5);
    int last = (int)floor(item.max_value + 0.5);
    for(int value = first; value <= last; ++value) {
        UiButton& button = choice_buttons_.Add();
        button.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        button.SetText(AsString(value)).SetCheckable(true).SetContentInset(DPI(2));
        button.WhenAction = [=] { CommitSimpleValue("rated", AsString(value), value); SyncFromModel(); };
        response_.Add(button).Fixed(DPI(30)).MinCross(DPI(27));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

Color TaskTrackQuestionCtrl::ParseColor(const String& text, Color fallback) const
{
    String s = text;
    if(s.StartsWith("#"))
        s = s.Mid(1);
    unsigned r = 0, g = 0, b = 0;
    if(s.GetCount() >= 6 && sscanf(~s, "%02x%02x%02x", &r, &g, &b) == 3)
        return Color((int)r, (int)g, (int)b);
    return fallback;
}

String TaskTrackQuestionCtrl::ColorToText(Color color) const
{
    return Format("#%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
}

void TaskTrackQuestionCtrl::BuildColor()
{
    TaskTrackItem& item = document_->items[item_index_];
    for(int i = 0; i < item.colors.GetCount(); ++i) {
        Color color = ParseColor(item.colors[i], Gray());
        UiButton& button = color_buttons_.Add();
        UiButton::Style style = CompactButtonStyle(UiRole::Subtle);
        style.palette.face[ST_NORMAL] = UiFill::Solid(color);
        style.palette.face[ST_HOT] = UiFill::Solid(Blend(color, White(), 25));
        style.palette.face[ST_PRESSED] = UiFill::Solid(Blend(color, Black(), 20));
        style.palette.ink[ST_NORMAL] = IsDarkColor(color) ? White() : Black();
        style.palette.ink[ST_HOT] = style.palette.ink[ST_NORMAL];
        style.palette.ink[ST_PRESSED] = style.palette.ink[ST_NORMAL];
        button.SetCustomStyle(style);
        button.SetText("").SetCheckable(true).SetContentInset(0);
        String value = item.colors[i];
        button.WhenAction = [=] { CommitSimpleValue("selected", value, value); SyncFromModel(); };
        response_.Add(button).Fixed(DPI(30)).MinCross(DPI(28));
    }

    String initial = item.expected_color;
    if(initial.IsEmpty() && !item.colors.IsEmpty())
        initial = item.colors[0];
    custom_color_.SetColor(0, ParseColor(initial, Color(47, 111, 237))).SetValueText(initial);
    custom_color_.WhenAction = [=] {
        String value = ColorToText(custom_color_.GetColor(0));
        custom_color_.SetValueText(value);
        CommitSimpleValue("selected", value, value);
        SyncFromModel();
    };
    response_.Add(custom_color_).Expand(1).MinMain(DPI(150)).MinCross(DPI(28));
    content_.Add(response_).Fit().MinCross(DPI(30)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildGradient()
{
    TaskTrackItem& item = document_->items[item_index_];
    gradient_.SetOptions(item.gradients);
    gradient_.WhenAction = [=] { CommitCurrent(); };
    content_.Add(gradient_).Fit().MinMain(DPI(42)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildPosition()
{
    position_.WhenAction = [=] { CommitCurrent(); };
    response_.Add(position_).Fit().MinMain(DPI(96)).MinCross(DPI(96));
    content_.Add(response_).Fit().MinCross(DPI(96)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildDirection()
{
    direction_.WhenAction = [=] { CommitCurrent(); };
    response_.Add(direction_).Fit().MinMain(DPI(106)).MinCross(DPI(96));
    content_.Add(response_).Fit().MinCross(DPI(96)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildRankOrder()
{
    TaskTrackItem& item = document_->items[item_index_];
    PopulateList(rank_, item.choices);
    rank_.SetSelectionMode(UILISTSEL_SINGLE);
    rank_.EnableRenameOnDblClick(false).EnableDragReorder(true).EnableInternalMutation(true).ShowDragHandle(true);
    rank_.WhenReordered = [=](int, int) { CommitRankOrder(true); };
    rank_confirm_.WhenAction = [=] { CommitRankOrder(true); };
    content_.Add(rank_).Fit().MinMain(DPI(100)).MaxMain(DPI(126)).AlignSelf(UiBoxLayout::Align::Stretch);
    response_.Add(rank_confirm_).Fit().MinMain(DPI(110)).MinCross(DPI(27));
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::AddHierarchyChildren(const String& parent_id, UiTreeNodeRef parent)
{
    TaskTrackItem& item = document_->items[item_index_];
    for(const TaskTrackHierarchyNode& node : item.hierarchy) {
        if(node.parent_id != parent_id)
            continue;
        UiModelItem model_item(node.label, node.id, true);
        UiTreeNodeRef ref = tree_.GetInternalModel().AddChild(parent, model_item);
        AddHierarchyChildren(node.id, ref);
        if(tree_.GetInternalModel().GetChildCount(ref) > 0)
            tree_.Expand(ref, true);
    }
}

void TaskTrackQuestionCtrl::PopulateHierarchy()
{

    UiTreeModel& model = tree_.GetInternalModel();
    model.Clear();
    AddHierarchyChildren(String(), model.Root());
}

void TaskTrackQuestionCtrl::BuildHierarchy()
{
    TaskTrackItem& item = document_->items[item_index_];
    PopulateHierarchy();
    tree_.SetRootVisible(false)
        .SetSelectionMode(item.allow_multiple ? UITREESEL_MULTI : UITREESEL_SINGLE)
        .EnableDragDrop(false)
        .EnableRenameOnDblClick(false);
    tree_.WhenSelection = [=] { CommitHierarchy(); };
    content_.Add(tree_).Fit().MinMain(DPI(112)).MaxMain(DPI(145)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::BuildCurve()
{
    TaskTrackItem& item = document_->items[item_index_];
    curve_.SetShowFormula(false).SetShowCopy(false).SetEditable(true).SetFormulaSelectable(false);
    if(!IsNull(item.default_value))
        curve_.SetData(item.default_value);
    curve_.WhenChanging = [=] { if(!syncing_) CommitCurve(true); };
    curve_.WhenAction = [=] { if(!syncing_) CommitCurve(true); };
    curve_confirm_.WhenAction = [=] { CommitCurve(true); };
    content_.Add(curve_).Fit().MinMain(DPI(100)).MaxMain(DPI(124)).AlignSelf(UiBoxLayout::Align::Stretch);
    response_.Add(curve_confirm_).Fit().MinMain(DPI(92)).MinCross(DPI(27));
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
}

void TaskTrackQuestionCtrl::TouchAnswer(TaskTrackAnswer& answer, bool answered)
{
    answer.answered = answered;
    if(answered)
        answer.answered_at = TaskTrackNowIso();
    else {
        answer.status.Clear();
        answer.value.Clear();
        answer.data = Value();
        answer.answered_at.Clear();
    }
}

void TaskTrackQuestionCtrl::CommitSimpleValue(const String& status, const String& value, const Value& data)
{
    if(syncing_ || !document_ || item_index_ < 0)
        return;
    TaskTrackAnswer& answer = document_->items[item_index_].answer;
    answer.status = status;
    answer.value = value;
    answer.data = data;
    TouchAnswer(answer, true);
    WhenChanged();
}

void TaskTrackQuestionCtrl::CommitCurrent()
{
    if(syncing_ || !document_ || item_index_ < 0)
        return;
    TaskTrackItem& item = document_->items[item_index_];
    TaskTrackAnswer& answer = item.answer;

    switch(item.type) {
    case TaskTrackItemType::Confirm:
    case TaskTrackItemType::SingleChoice:
        if(!radios_.IsEmpty()) {
            for(int i = 0; i < radios_.GetCount(); ++i) {
                if(radios_[i].IsChecked()) {
                    String value = item.type == TaskTrackItemType::Confirm
                                 ? (i == 0 ? String("yes") : String("no"))
                                 : item.choices[i];
                    Value data = item.type == TaskTrackItemType::Confirm ? Value(i == 0) : Value(value);
                    CommitSimpleValue("selected", value, data);
                    return;
                }
            }
        }
        else if(dropdown_.HasSelection())
            CommitSimpleValue("selected", AsString(dropdown_.GetSelectedData()), dropdown_.GetSelectedData());
        return;

    case TaskTrackItemType::MultiChoice: {
        ValueArray values;
        for(int i = 0; i < checks_.GetCount(); ++i)
            if(checks_[i].IsChecked())
                values.Add(item.choices[i]);
        answer.status = values.IsEmpty() ? String() : String("selected");
        answer.value = AsJSON(values, false);
        answer.data = values;
        TouchAnswer(answer, !values.IsEmpty());
        WhenChanged();
        return;
    }

    case TaskTrackItemType::Select:
        if(dropdown_.HasSelection())
            CommitSimpleValue("selected", AsString(dropdown_.GetSelectedData()), dropdown_.GetSelectedData());
        return;

    case TaskTrackItemType::ListSelect: {
        Value data = list_.GetData();
        bool answered = !IsNull(data);
        if(data.Is<ValueArray>())
            answered = !((ValueArray)data).IsEmpty();
        answer.status = answered ? "selected" : "";
        answer.data = data;
        answer.value = answered ? (data.Is<ValueArray>() ? AsJSON(data, false) : AsString(data)) : String();
        TouchAnswer(answer, answered);
        WhenChanged();
        return;
    }

    case TaskTrackItemType::Text: {
        String value = TrimBoth(text_.GetTextUtf8());
        answer.status = value.IsEmpty() ? "" : "recorded";
        answer.value = value;
        answer.data = value;
        TouchAnswer(answer, !value.IsEmpty());
        WhenChanged();
        return;
    }

    case TaskTrackItemType::Notes: {
        String value = TrimBoth(notes_.GetTextUtf8());
        answer.status = value.IsEmpty() ? "" : "recorded";
        answer.value = value;
        answer.data = value;
        answer.note.Clear();
        TouchAnswer(answer, !value.IsEmpty());
        WhenChanged();
        return;
    }

    case TaskTrackItemType::Number: {
        Value data = number_.GetData();
        bool answered = !IsNull(data);
        answer.status = answered ? "recorded" : "";
        answer.data = data;
        answer.value = answered ? AsString(data) : String();
        TouchAnswer(answer, answered);
        WhenChanged();
        return;
    }

    case TaskTrackItemType::Amount: {
        double value = amount_.GetValue();
        CommitSimpleValue("selected", DisplayNumber(value) + item.unit, value);
        return;
    }

    case TaskTrackItemType::Range: {
        ValueMap data;
        data.Add("low", range_.GetLow());
        data.Add("high", range_.GetHigh());
        CommitSimpleValue("selected", DisplayNumber(range_.GetLow()) + "–" + DisplayNumber(range_.GetHigh()) + item.unit, Value(data));
        return;
    }

    case TaskTrackItemType::Gradient:
        if(!gradient_.GetValue().IsEmpty())
            CommitSimpleValue("selected", gradient_.GetValue(), gradient_.GetValue());
        return;

    case TaskTrackItemType::Position:
        if(!position_.GetValue().IsEmpty())
            CommitSimpleValue("selected", position_.GetValue(), position_.GetValue());
        return;

    case TaskTrackItemType::Direction:
        if(!direction_.GetValue().IsEmpty())
            CommitSimpleValue("selected", direction_.GetValue(), direction_.GetValue());
        return;

    case TaskTrackItemType::RankOrder:
        CommitRankOrder(true);
        return;
    case TaskTrackItemType::HierarchySelect:
        CommitHierarchy();
        return;
    case TaskTrackItemType::Curve:
        CommitCurve(true);
        return;
    case TaskTrackItemType::Rating:
    case TaskTrackItemType::Color:
        return; // committed directly by their button/swatch callbacks.
    }
}

void TaskTrackQuestionCtrl::CommitRankOrder(bool confirmed)
{
    if(syncing_ || !document_ || item_index_ < 0)
        return;
    ValueArray order;
    const UiListModel& model = rank_.GetModel();
    for(int i = 0; i < model.GetCount(); ++i)
        order.Add(model.Get(i).data);
    TaskTrackAnswer& answer = document_->items[item_index_].answer;
    answer.status = confirmed ? "ordered" : answer.status;
    answer.value = AsJSON(order, false);
    answer.data = order;
    if(confirmed)
        TouchAnswer(answer, true);
    WhenChanged();
}

void TaskTrackQuestionCtrl::CommitHierarchy()
{
    if(syncing_ || !document_ || item_index_ < 0)
        return;
    TaskTrackItem& item = document_->items[item_index_];
    Value data = tree_.GetData();
    bool answered = !IsNull(data);
    if(data.Is<ValueArray>())
        answered = !((ValueArray)data).IsEmpty();
    TaskTrackAnswer& answer = item.answer;
    answer.status = answered ? "selected" : "";
    answer.data = data;
    answer.value = answered ? (data.Is<ValueArray>() ? AsJSON(data, false) : AsString(data)) : String();
    TouchAnswer(answer, answered);
    WhenChanged();
}


void TaskTrackQuestionCtrl::CommitCurve(bool confirmed)
{
    if(syncing_ || !document_ || item_index_ < 0)
        return;
    Value data = curve_.GetData();
    TaskTrackAnswer& answer = document_->items[item_index_].answer;
    answer.data = data;
    answer.value = AsJSON(data, false);
    answer.status = confirmed ? "selected" : answer.status;
    if(confirmed)
        TouchAnswer(answer, true);
    WhenChanged();
}

void TaskTrackQuestionCtrl::SyncFromModel()
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    syncing_ = true;
    TaskTrackItem& item = document_->items[item_index_];
    const TaskTrackAnswer& answer = item.answer;

    if(item.type == TaskTrackItemType::Confirm) {
        bool yes = answer.answered && (answer.value == "yes" || answer.value == "true" || (answer.data.Is<bool>() && (bool)answer.data));
        bool no = answer.answered && !yes;
        if(radios_.GetCount() >= 2) {
            radios_[0].SetChecked(yes);
            radios_[1].SetChecked(no);
        }
        if(verdict_note_.GetParent())
            verdict_note_.SetTextUtf8(answer.note);
    }
    else if(item.type == TaskTrackItemType::SingleChoice) {
        if(!radios_.IsEmpty()) {
            for(int i = 0; i < radios_.GetCount() && i < item.choices.GetCount(); ++i)
                radios_[i].SetChecked(answer.answered && item.choices[i] == answer.value);
        }
        else if(answer.answered)
            dropdown_.SelectByData(IsNull(answer.data) ? Value(answer.value) : answer.data);
    }
    else if(item.type == TaskTrackItemType::MultiChoice) {
        Index<String> selected;
        if(answer.data.Is<ValueArray>()) {
            ValueArray values = answer.data;
            for(int i = 0; i < values.GetCount(); ++i)
                selected.Add(AsString(values[i]));
        }
        for(int i = 0; i < checks_.GetCount() && i < item.choices.GetCount(); ++i)
            checks_[i].SetChecked(selected.Find(item.choices[i]) >= 0);
    }
    else if(item.type == TaskTrackItemType::Select && answer.answered)
        dropdown_.SelectByData(IsNull(answer.data) ? Value(answer.value) : answer.data);
    else if(item.type == TaskTrackItemType::ListSelect && answer.answered)
        list_.SetData(answer.data);
    else if(item.type == TaskTrackItemType::Text)
        text_.SetTextUtf8(answer.value);
    else if(item.type == TaskTrackItemType::Notes)
        notes_.SetTextUtf8(answer.value);
    else if(item.type == TaskTrackItemType::Number) {
        if(answer.answered) number_.SetData(answer.data);
        else if(IsNumber(item.default_value)) number_.SetData(item.default_value);
    }
    else if(item.type == TaskTrackItemType::Amount) {
        if(answer.answered && IsNumber(answer.data)) amount_.SetValue((double)answer.data);
        else if(IsNumber(item.default_value)) amount_.SetValue((double)item.default_value);
    }
    else if(item.type == TaskTrackItemType::Range && answer.data.Is<ValueMap>()) {
        ValueMap data = answer.data;
        if(IsNumber(data["low"]) && IsNumber(data["high"]))
            range_.SetValues((double)data["low"], (double)data["high"]);
    }
    else if(item.type == TaskTrackItemType::Rating) {
        int value = answer.answered ? atoi(~answer.value) : INT_MIN;
        int first = (int)floor(item.min_value + 0.5);
        for(int i = 0; i < choice_buttons_.GetCount(); ++i)
            choice_buttons_[i].SetChecked(first + i == value);
    }
    else if(item.type == TaskTrackItemType::Color) {
        String selected = answer.answered ? answer.value : String();
        for(int i = 0; i < color_buttons_.GetCount() && i < item.colors.GetCount(); ++i)
            color_buttons_[i].SetChecked(ToUpper(item.colors[i]) == ToUpper(selected));
        if(answer.answered) {
            custom_color_.SetColor(0, ParseColor(selected, custom_color_.GetColor(0)));
            custom_color_.SetValueText(selected);
        }
    }
    else if(item.type == TaskTrackItemType::Gradient) {
        if(answer.answered) gradient_.SetValue(answer.value);
        else if(item.default_value.Is<String>()) gradient_.SetValue(AsString(item.default_value));
    }
    else if(item.type == TaskTrackItemType::Position) {
        if(answer.answered) position_.SetValue(answer.value);
        else if(item.default_value.Is<String>()) position_.SetValue(AsString(item.default_value));
    }
    else if(item.type == TaskTrackItemType::Direction) {
        if(answer.answered) direction_.SetValue(answer.value);
        else if(item.default_value.Is<String>()) direction_.SetValue(AsString(item.default_value));
    }
    else if(item.type == TaskTrackItemType::RankOrder && answer.data.Is<ValueArray>()) {
        ValueArray order = answer.data;
        Vector<String> ordered;
        for(int i = 0; i < order.GetCount(); ++i)
            ordered.Add(AsString(order[i]));
        if(!ordered.IsEmpty())
            PopulateList(rank_, ordered);
    }
    else if(item.type == TaskTrackItemType::HierarchySelect && answer.answered)
        tree_.SetData(answer.data);
    else if(item.type == TaskTrackItemType::Curve) {
        if(answer.answered && answer.data.Is<ValueArray>())
            curve_.SetData(answer.data);
        else if(!IsNull(item.default_value))
            curve_.SetData(item.default_value);
    }

    syncing_ = false;
}

} // namespace Upp
