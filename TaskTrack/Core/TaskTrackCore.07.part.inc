        out << "- **Project:** " << doc.project << "\n";
    out << "- **State:** " << TaskTrackStateName(doc.state) << "\n";
    out << "- **Updated:** " << doc.updated_at << "\n";
    out << "- **Progress:** " << TaskTrackAnsweredCount(doc) << "/" << doc.items.GetCount() << "\n\n";

    String last_category;
    for(const TaskTrackItem& item : doc.items) {
        if(item.category != last_category) {
            last_category = item.category;
            out << "## " << last_category << "\n\n";
        }
        out << "- [" << (item.answer.answered ? "x" : " ") << "] **"
            << MarkdownEscape(item.title) << "**";
        if(item.required)
            out << " *(required)*";
        out << "\n";
        if(!item.instruction.IsEmpty())
            out << "  - Question: " << MarkdownEscape(item.instruction) << "\n";
        out << "  - Type: `" << TaskTrackItemTypeName(item.type) << "`\n";
        if(!item.recommended.IsEmpty())
            out << "  - Agent recommendation: " << MarkdownEscape(item.recommended) << "\n";
        if(item.answer.answered) {
            if(!item.answer.status.IsEmpty())
                out << "  - Status: **" << MarkdownEscape(item.answer.status) << "**\n";
            String data = DataToDisplay(item.answer.data);
            if(!data.IsEmpty())
                out << "  - Answer: " << MarkdownEscape(data) << "\n";
            else if(!item.answer.value.IsEmpty())
                out << "  - Answer: " << MarkdownEscape(item.answer.value) << "\n";
            if(!item.answer.note.IsEmpty())
                out << "  - Note: " << MarkdownEscape(item.answer.note) << "\n";
            if(!item.answer.answered_at.IsEmpty())
                out << "  - Answered: " << item.answer.answered_at << "\n";
        }
    }
    return out;
}

Value TaskTrackStatusValue(const TaskTrackDocument& doc, const String& path, bool include_items)
{
    ValueMap root;
    root.Add("ok", true);
    root.Add("schema_version", 2);
    root.Add("task_id", doc.task_id);
    root.Add("task_path", NormalizePath(path));
    root.Add("project", doc.project);
    root.Add("title", doc.title);
    root.Add("subtitle", doc.subtitle);
    root.Add("state", TaskTrackStateName(doc.state));
    root.Add("created_at", doc.created_at);
    root.Add("updated_at", doc.updated_at);
    root.Add("reminder_minutes", doc.reminder_minutes);
    root.Add("remind_while_paused", doc.remind_while_paused);
    root.Add("nudge_on_agent_poll", doc.nudge_on_agent_poll);
    root.Add("reminder_count", doc.reminder_count);
    root.Add("answered", TaskTrackAnsweredCount(doc));
    root.Add("total", doc.items.GetCount());
    root.Add("required_answered", TaskTrackRequiredAnsweredCount(doc));
    root.Add("required_total", TaskTrackRequiredCount(doc));
    root.Add("ready_to_complete", TaskTrackCanComplete(doc));

    if(include_items) {
        Value serialized = TaskTrackToValue(doc);
        root.Add("items", serialized["items"]);
    }
    return Value(root);
}

bool TaskTrackList(const String& store_root, int limit, ValueArray& result, String& error)
{
    error.Clear();
    result.Clear();
    String root = EnsureStoreRoot(store_root);
    if(limit <= 0)
        limit = 20;
    limit = min(limit, 200);

    Vector<String> paths;
    FindFile ff(AppendFileName(root, "*.tasktrack.json"));
    while(ff) {
        if(ff.IsFile())
            paths.Add(AppendFileName(root, ff.GetName()));
        ff.Next();
    }
    Sort(paths);

    for(int i = paths.GetCount() - 1; i >= 0 && result.GetCount() < limit; --i) {
        TaskTrackDocument item;
        String load_error;
        if(!TaskTrackLoad(paths[i], item, load_error))
            continue;
        result.Add(TaskTrackStatusValue(item, paths[i], false));
    }
    return true;
}

void TaskTrackPruneHistory(const String& store_root, int history_limit)
{
    if(history_limit < 5)
        history_limit = 5;
    String root = EnsureStoreRoot(store_root);
    Vector<String> paths;
    FindFile ff(AppendFileName(root, "*.tasktrack.json"));
    while(ff) {
        if(ff.IsFile())
            paths.Add(AppendFileName(root, ff.GetName()));
        ff.Next();
    }
    if(paths.GetCount() <= history_limit)
        return;

    Sort(paths);
    int excess = paths.GetCount() - history_limit;
    for(int i = 0; i < paths.GetCount() && excess > 0; ++i) {
        TaskTrackDocument item;
        String error;
        if(!TaskTrackLoad(paths[i], item, error))
            continue;
        if(item.state != TaskTrackState::Completed && item.state != TaskTrackState::Closed)
            continue;
        FileDelete(paths[i] + ".bak");
        if(FileDelete(paths[i])) {
            String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), item.task_id + ".path");
            FileDelete(locator);
            FileDelete(locator + ".bak");
            --excess;
        }
    }
}

bool TaskTrackSetState(const String& path, TaskTrackState state, String& error)
{
    TaskTrackDocument doc;
    if(!TaskTrackLoad(path, doc, error))
        return false;
    doc.state = state;
    doc.updated_at = TaskTrackNowIso();
    doc.last_human_activity_at = doc.updated_at;
    doc.last_human_activity_epoch = GetSysTime().Get();
    return TaskTrackSave(path, doc, error);
}

String TaskTrackPollMarkerPath(const String& task_path)
{
    return NormalizePath(task_path) + ".poll";
}

bool TaskTrackTouchAgentPoll(const String& task_path)
{
    String marker = TaskTrackPollMarkerPath(task_path);
    String folder = GetFileFolder(marker);
    if(!RealizeDirectory(folder))
        return false;
    return SaveFile(marker, AsString((int64)GetSysTime().Get()));
}

int64 TaskTrackReadAgentPollEpoch(const String& task_path)
{
    String text = TrimBoth(LoadFile(TaskTrackPollMarkerPath(task_path)));
    if(IsNull(text) || text.IsEmpty())
        return 0;
    const char *begin = ~text;
    char *end = nullptr;
    long long value = strtoll(begin, &end, 10);
    if(end == begin)
        return 0;
    return (int64)value;
}

} // namespace Upp
