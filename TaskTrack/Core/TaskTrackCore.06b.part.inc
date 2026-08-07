    }

    Index<String> ids;
    const ValueArray& array = raw_items;
    for(int i = 0; i < array.GetCount(); ++i) {
        TaskTrackItem item;
        String prefix = Format("items[%d].", i);
        if(!ParseItem(array[i], item, prefix, error))
            return false;
        if(item.id.IsEmpty())
            item.id = Format("item-%d", i + 1);
        if(ids.Find(item.id) >= 0) {
            error = "Duplicate item id: " + item.id;
            return false;
        }
        ids.Add(item.id);
        created.items.Add(pick(item));
    }

    if(created.items.IsEmpty()) {
        error = "create_task requires at least one item.";
        return false;
    }

    String root = EnsureStoreRoot(ReadString(args, "store_root"));
    path = TaskTrackMakeTaskPath(root, created.task_id);
    if(FileExists(path) || FileExists(path + ".bak")) {
        error = "TaskTrack task already exists: " + created.task_id;
        return false;
    }

    if(!TaskTrackSave(path, created, error))
        return false;

    String registry_error;
    if(!TaskTrackRegisterTask(created.task_id, path, registry_error)) {
        FileDelete(path);
        error = "Task created but registry update failed; rolled back task file. " + registry_error;
        return false;
    }

    TaskTrackPruneHistory(root, created.history_limit);
    doc = pick(created);
    return true;
}

Vector<String> TaskTrackCategories(const TaskTrackDocument& doc)
{
    Index<String> seen;
    Vector<String> out;
    for(const TaskTrackItem& item : doc.items) {
        String category = item.category.IsEmpty() ? String("General") : item.category;
        if(seen.Find(category) < 0) {
            seen.Add(category);
            out.Add(category);
        }
    }
    return out;
}

int TaskTrackAnsweredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.answer.answered)
            ++count;
    return count;
}

int TaskTrackRequiredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.required)
            ++count;
    return count;
}

int TaskTrackRequiredAnsweredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.required && item.answer.answered)
            ++count;
    return count;
}

bool TaskTrackCanComplete(const TaskTrackDocument& doc, Vector<String>* missing_ids)
{
    if(missing_ids)
        missing_ids->Clear();
    bool ok = true;
    for(const TaskTrackItem& item : doc.items) {
        if(item.required && !item.answer.answered) {
            ok = false;
            if(missing_ids)
                missing_ids->Add(item.id);
        }
    }
    return ok;
}

String TaskTrackExportMarkdown(const TaskTrackDocument& doc)
{
    String out;
    out << "# " << doc.title << "\n\n";
    if(!doc.subtitle.IsEmpty())
        out << doc.subtitle << "\n\n";
    out << "- **Task:** `" << doc.task_id << "`\n";
    if(!doc.project.IsEmpty())
