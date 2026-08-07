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
