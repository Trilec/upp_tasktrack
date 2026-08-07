        error = prefix + "answer must be an object.";
        return false;
    }
    String field_error;
    if(!ExpectOptionalBool(raw, "answered", field_error)) {
        error = prefix + "answer." + field_error;
        return false;
    }
    const char *strings[] = { "status", "value", "note", "answered_at" };
    for(const char *key : strings) {
        if(!ExpectOptionalString(raw, key, field_error)) {
            error = prefix + "answer." + field_error;
            return false;
        }
    }
    answer.answered = ReadBool(raw, "answered", false);
    answer.status = ReadString(raw, "status");
    answer.value = ReadString(raw, "value");
    answer.data = raw["data"];
    if(IsNull(answer.data) && answer.answered && !answer.value.IsEmpty())
        answer.data = answer.value;
    answer.note = ReadString(raw, "note");
    answer.answered_at = ReadString(raw, "answered_at");
    return true;
}

bool ParseStringArray(const Value& raw, const char *field, Vector<String>& out,
                      const String& prefix, String& error)
{
    out.Clear();
    Value value = raw[field];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + field + " must be an array.";
        return false;
    }
    const ValueArray& array = value;
    for(int i = 0; i < array.GetCount(); ++i) {
        if(!array[i].Is<String>()) {
            error = Format("%s%s[%d] must be a string.", prefix, field, i);
            return false;
        }
        String text = TrimBoth(AsString(array[i]));
        if(!text.IsEmpty())
            out.Add(text);
    }
    return true;
}

bool ParseGradients(const Value& raw, Vector<TaskTrackGradientOption>& out,
                    const String& prefix, String& error)
{
    out.Clear();
    Value value = raw["gradients"];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + "gradients must be an array.";
        return false;
    }
    const ValueArray& array = value;
    Index<String> ids;
    for(int i = 0; i < array.GetCount(); ++i) {
        const Value& entry = array[i];
        if(!entry.Is<ValueMap>()) {
            error = Format("%sgradients[%d] must be an object.", prefix, i);
            return false;
        }
        const char *fields[] = { "id", "label", "from", "to" };
        for(const char *key : fields) {
            String field_error;
            if(!ExpectOptionalString(entry, key, field_error)) {
                error = Format("%sgradients[%d].%s", prefix, i, field_error);
                return false;
            }
        }
        TaskTrackGradientOption option;
        option.id = TrimBoth(ReadString(entry, "id"));
        option.label = TrimBoth(ReadString(entry, "label", option.id));
        option.from_color = TrimBoth(ReadString(entry, "from"));
        option.to_color = TrimBoth(ReadString(entry, "to"));
        if(option.id.IsEmpty() || ids.Find(option.id) >= 0) {
            error = Format("%sgradients[%d] requires a unique non-empty id.", prefix, i);
            return false;
        }
        if(!ValidateColorText(option.from_color) || option.from_color.IsEmpty() ||
           !ValidateColorText(option.to_color) || option.to_color.IsEmpty()) {
            error = Format("%sgradients[%d] requires valid #RRGGBB colours.", prefix, i);
            return false;
        }
        ids.Add(option.id);
        out.Add(pick(option));
    }
    return true;
}

bool ValidateHierarchy(const Vector<TaskTrackHierarchyNode>& nodes, const String& prefix, String& error)
{
    Index<String> ids;
    for(const TaskTrackHierarchyNode& node : nodes) {
        if(node.id.IsEmpty() || ids.Find(node.id) >= 0) {
            error = prefix + "hierarchy node ids must be unique and non-empty.";
            return false;
        }
        ids.Add(node.id);
    }
    for(int i = 0; i < nodes.GetCount(); ++i) {
        const TaskTrackHierarchyNode& node = nodes[i];
        if(!node.parent_id.IsEmpty() && ids.Find(node.parent_id) < 0) {
            error = prefix + "hierarchy node " + node.id + " references missing parent " + node.parent_id + ".";
            return false;
        }
        String parent = node.parent_id;
        Index<String> chain;
        chain.Add(node.id);
        for(int guard = 0; !parent.IsEmpty() && guard <= nodes.GetCount(); ++guard) {
            if(chain.Find(parent) >= 0) {
                error = prefix + "hierarchy contains a cycle at " + node.id + ".";
                return false;
            }
            chain.Add(parent);
            int p = ids.Find(parent);
            if(p < 0)
                break;
            parent = nodes[p].parent_id;
        }
    }
    return true;
}

bool ParseHierarchy(const Value& raw, Vector<TaskTrackHierarchyNode>& out,
                    const String& prefix, String& error)
{
    out.Clear();
    Value value = raw["hierarchy"];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + "hierarchy must be an array.";
        return false;
    }
    const ValueArray& array = value;
    for(int i = 0; i < array.GetCount(); ++i) {
        const Value& entry = array[i];
        if(!entry.Is<ValueMap>()) {
            error = Format("%shierarchy[%d] must be an object.", prefix, i);
            return false;
        }
        const char *fields[] = { "id", "parent_id", "label" };
        for(const char *key : fields) {
            String field_error;
            if(!ExpectOptionalString(entry, key, field_error)) {
                error = Format("%shierarchy[%d].%s", prefix, i, field_error);
                return false;
            }
        }
        TaskTrackHierarchyNode node;
        node.id = TrimBoth(ReadString(entry, "id"));
        node.parent_id = TrimBoth(ReadString(entry, "parent_id"));
        node.label = TrimBoth(ReadString(entry, "label", node.id));
        if(node.label.IsEmpty())
            node.label = node.id;
        out.Add(pick(node));
    }
    return ValidateHierarchy(out, prefix, error);
}


void NormalizeLegacyAnswer(const String& raw_type, TaskTrackItem& item)
{
    if(!item.answer.answered)
        return;
    String value = ToLower(TrimBoth(item.answer.value));
    String normalized;
    if(raw_type == "pass_fail") {
        if(value == "pass") normalized = "Pass";
        else if(value == "fail") normalized = "Fail";
        else if(value == "blocked") normalized = "Blocked";
        else if(value == "not_applicable") normalized = "Not applicable";
    }
    else if(raw_type == "file") {
        if(value == "found") normalized = "Found";
        else if(value == "missing") normalized = "Missing";
        else if(value == "wrong_output") normalized = "Wrong output";
        else if(value == "unsure") normalized = "Unsure";
    }
    else if(raw_type == "interaction") {
        if(value == "pass") normalized = "Pass";
        else if(value == "fail") normalized = "Fail";
        else if(value == "partial") normalized = "Partial";
        else if(value == "blocked") normalized = "Blocked";
    }
    else if(raw_type == "visual_compare" || raw_type == "legacy_color_witness") {
        if(value == "match") normalized = "Match";
        else if(value == "different") normalized = "Different";
        else if(value == "unsure") normalized = "Unsure";
    }
    if(!normalized.IsEmpty()) {
        item.answer.value = normalized;
        item.answer.data = normalized;
    }
}

void AddLegacyChoices(const String& raw_type, TaskTrackItem& item)
{
    if(!item.choices.IsEmpty())
        return;
    if(raw_type == "pass_fail") {
        item.choices.Add("Pass"); item.choices.Add("Fail"); item.choices.Add("Blocked"); item.choices.Add("Not applicable");
    }
    else if(raw_type == "file") {
        item.choices.Add("Found"); item.choices.Add("Missing"); item.choices.Add("Wrong output"); item.choices.Add("Unsure");
    }
    else if(raw_type == "interaction") {
        item.choices.Add("Pass"); item.choices.Add("Fail"); item.choices.Add("Partial"); item.choices.Add("Blocked");
    }
    else if(raw_type == "visual_compare" || raw_type == "legacy_color_witness") {
        item.choices.Add("Match"); item.choices.Add("Different"); item.choices.Add("Unsure");
    }
