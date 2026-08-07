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
