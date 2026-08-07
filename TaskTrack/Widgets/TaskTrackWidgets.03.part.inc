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
    content_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(0);
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
    for(int i = 0; i < 2; ++i) {
        UiRadioButton& radio = radios_.Add();
        radio.SetText(labels[i]).SetGroup(group).SetVisual(UIRADIOVIS_PILLS).SetIndicatorSide(UiAlign::LEFT);
        UiRadioButton::Style style = radio.GetStyle();
        style.font = SansSerifZ(9);
        radio.SetCustomStyle(style);
        String label = labels[i];
        String value = values[i];
        bool affirmative = i == 0;
        radio.WhenAction = [=] { CommitSimpleValue(label, value, affirmative); };
        response_.Add(radio).Fit().MinMain(DPI(84)).MinCross(DPI(26));
    }
    content_.Add(response_).Fit().MinCross(DPI(29)).AlignSelf(UiBoxLayout::Align::Stretch);
    (void)item;
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
