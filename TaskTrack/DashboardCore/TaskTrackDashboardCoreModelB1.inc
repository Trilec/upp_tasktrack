String TaskTrackDashboardToJson(const TaskTrackDashboardDocument& doc, bool pretty)
{
    return AsJSON(TaskTrackDashboardToValue(doc), pretty);
}

bool TaskTrackDashboardFromValue(const Value& value, TaskTrackDashboardDocument& doc, String& error)
{
    error.Clear();
    if(!value.Is<ValueMap>()) {
        error = "TaskTrack dashboard must be a JSON object.";
        return false;
    }

    const char *string_fields[] = {
        "dashboard_version", "dashboard_id", "project", "title", "subtitle", "phase",
        "created_at", "updated_at", "updated_by", "source_head"
    };
    for(const char *key : string_fields)
        if(!ExpectOptionalString(value, key, error))
            return false;
    const char *number_fields[] = { "schema_version", "overall_progress", "confidence", "revision" };
    for(const char *key : number_fields)
        if(!ExpectOptionalNumber(value, key, error))
            return false;
    if(!ExpectOptionalArray(value, "panels", error))
        return false;

    int schema = ReadInt(value, "schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);
    if(schema != TASKTRACK_DASHBOARD_SCHEMA_VERSION) {
        error = Format("Unsupported TaskTrack dashboard schema version %d.", schema);
        return false;
    }

    TaskTrackDashboardDocument parsed;
    parsed.schema_version = TASKTRACK_DASHBOARD_SCHEMA_VERSION;
    parsed.dashboard_id = TrimBoth(ReadString(value, "dashboard_id"));
    parsed.project = TrimBoth(ReadString(value, "project"));
    parsed.title = TrimBoth(ReadString(value, "title"));
    parsed.subtitle = ReadString(value, "subtitle");
    parsed.phase = TrimBoth(ReadString(value, "phase"));
    parsed.overall_progress = ReadDouble(value, "overall_progress", -1.0);
    parsed.confidence = ReadDouble(value, "confidence", -1.0);
    parsed.revision = ReadInt(value, "revision", 0);
    parsed.created_at = ReadString(value, "created_at");
    parsed.updated_at = ReadString(value, "updated_at");
    parsed.updated_by = TrimBoth(ReadString(value, "updated_by"));
    parsed.source_head = TrimBoth(ReadString(value, "source_head"));

    Value raw_panels = value["panels"];
    if(IsNull(raw_panels)) raw_panels = Value(ValueArray());
    ValueArray panels = raw_panels;
    if(panels.GetCount() > TASKTRACK_DASHBOARD_MAX_PANELS) {
        error = Format("TaskTrack dashboard exceeds the %d-panel safety limit.", TASKTRACK_DASHBOARD_MAX_PANELS);
        return false;
    }
    for(int i = 0; i < panels.GetCount(); ++i) {
        TaskTrackDashboardPanel panel;
        if(!ParsePanel(panels[i], panel, Format("panels[%d].", i), error))
            return false;
        parsed.panels.Add(pick(panel));
    }
    if(!TaskTrackDashboardValidate(parsed, error))
        return false;
    doc = pick(parsed);
    return true;
}
