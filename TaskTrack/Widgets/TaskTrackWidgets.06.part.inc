
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
