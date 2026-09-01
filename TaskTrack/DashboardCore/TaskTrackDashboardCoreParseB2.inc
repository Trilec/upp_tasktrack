bool ParsePanel(const Value& value, TaskTrackDashboardPanel& panel,
                const String& prefix, String& error)
{
    if(!value.Is<ValueMap>()) {
        error = prefix + "must be an object.";
        return false;
    }

    const char *string_fields[] = { "id", "type", "title", "subtitle", "category", "density" };
    for(const char *key : string_fields) {
        String field_error;
        if(!ExpectOptionalString(value, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }
    String field_error;
    if(!ExpectOptionalNumber(value, "preview_limit", field_error)) {
        error = prefix + field_error;
        return false;
    }
    if(!ExpectOptionalBool(value, "contributes_to_progress", field_error)) {
        error = prefix + field_error;
        return false;
    }
    if(!ExpectOptionalArray(value, "entries", field_error)) {
        error = prefix + field_error;
        return false;
    }

    panel.id = TrimBoth(ReadString(value, "id"));
    panel.title = TrimBoth(ReadString(value, "title"));
    panel.subtitle = ReadString(value, "subtitle");
    panel.category = TrimBoth(ReadString(value, "category"));
    panel.preview_limit = ReadInt(value, "preview_limit", 6);

    if(!TaskTrackDashboardParsePanelType(ReadString(value, "type", "records"), panel.type)) {
        error = prefix + "type is invalid.";
        return false;
    }
    if(!TaskTrackDashboardParseDensity(ReadString(value, "density", "standard"), panel.density)) {
        error = prefix + "density is invalid.";
        return false;
    }

    bool progress_default = panel.type == TaskTrackDashboardPanelType::Timeline ||
                            panel.type == TaskTrackDashboardPanelType::ProgressList;
    panel.contributes_to_progress = ReadBool(value, "contributes_to_progress", progress_default);

    if(panel.id.IsEmpty()) {
        error = prefix + "id is required.";
        return false;
    }
    if(panel.title.IsEmpty()) {
        error = prefix + "title is required.";
        return false;
    }
    if(panel.preview_limit < 1 || panel.preview_limit > 100) {
        error = prefix + "preview_limit must be 1..100.";
        return false;
    }

    Value raw_entries = value["entries"];
    if(IsNull(raw_entries))
        raw_entries = Value(ValueArray());
    ValueArray entries = raw_entries;
    if(entries.GetCount() > TASKTRACK_DASHBOARD_MAX_ENTRIES_PER_PANEL) {
        error = prefix + Format("entries exceeds the %d-entry safety limit.",
                                TASKTRACK_DASHBOARD_MAX_ENTRIES_PER_PANEL);
        return false;
    }

    Index<String> ids;
    for(int i = 0; i < entries.GetCount(); ++i) {
        TaskTrackDashboardEntry entry;
        if(!ParseEntry(entries[i], entry, prefix + Format("entries[%d].", i), error))
            return false;
        if(ids.Find(entry.id) >= 0) {
            error = prefix + "entry ids must be unique within a panel.";
            return false;
        }
        ids.Add(entry.id);
        panel.entries.Add(pick(entry));
    }
    return true;
}

Value EmptyProgressValue(double progress)
{
    return progress < 0.0 ? Value() : Value(progress);
}

} // namespace
