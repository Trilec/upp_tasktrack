    Vector<String> categories = TaskTrackCategories(document_);
    bool show_categories = categories.GetCount() > 1;
    categories_group_.Show(show_categories);
    if(show_categories)
        categories_item_.Fit().MinMain(DPI(50));
    else
        categories_item_.Fixed(0).MinMain(0);

    auto add_button = [&](const String& category) {
        int total = 0;
        int answered = 0;
        for(const TaskTrackItem& item : document_.items) {
            if(category != "All" && item.category != category)
                continue;
            ++total;
            if(item.answer.answered)
                ++answered;
        }

        UiButton& button = category_buttons_.Add();
        button.SetCustomStyle(MakeCategoryButtonStyle(category == selected_category_));
        button.SetText(Format("%s  %d/%d", category, answered, total));
        button.SetContentInset(DPI(3));
        String selected = category;
        button.WhenAction = [=] { SelectCategory(selected); };
        categories_flow_.Add(button).Fit().MinMain(DPI(148)).MinCross(DPI(26));
    };

    add_button("All");
    for(const String& category : categories)
        add_button(category);

    rebuilding_ = false;
    categories_flow_.RefreshLayout();
    categories_group_.RefreshLayout();
}

void TaskTrackWindow::RebuildItems()
{
    rebuilding_ = true;
    task_flow_.ClearItems();
    question_controls_.Clear();

    int shown = 0;
    for(int i = 0; i < document_.items.GetCount(); ++i) {
        const TaskTrackItem& item = document_.items[i];
        if(selected_category_ != "All" && item.category != selected_category_)
            continue;

        TaskTrackQuestionCtrl& ctrl = question_controls_.Add();
        ctrl.Bind(document_, i);
        ctrl.WhenChanged = [=] { OnItemChanged(); };
        bool terminal = document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed;
        ctrl.Enable(!terminal);
        task_flow_.Add(ctrl).Fit().MinMain(DPI(310)).MaxMain(DPI(350)).MinCross(DPI(78));
        ++shown;
    }

    empty_label_.Show(shown == 0);
    rebuilding_ = false;
    task_flow_.RefreshLayout();
    task_scroll_.RefreshLayout();
}

void TaskTrackWindow::SelectCategory(const String& category)
{
    if(category == selected_category_)
        return;
    selected_category_ = category;
    RebuildCategories();
    RebuildItems();
}

void TaskTrackWindow::TouchHumanActivity()
{
    document_.last_human_activity_at = TaskTrackNowIso();
    document_.last_human_activity_epoch = GetSysTime().Get();
}

bool TaskTrackWindow::SaveProgress(bool touch_human)
{
    if(!loaded_)
        return false;
    KillTimeCallback(TASKTRACK_AUTOSAVE_TIMER_ID);
    if(touch_human)
        TouchHumanActivity();
    document_.updated_at = TaskTrackNowIso();
    String error;
    if(!TaskTrackSave(task_path_, document_, error)) {
        Exclamation("TaskTrack could not save progress.\n" + error);
        return false;
    }
    RefreshProgress();
    return true;
}

void TaskTrackWindow::ScheduleAutosave()
{
    KillTimeCallback(TASKTRACK_AUTOSAVE_TIMER_ID);
    SetTimeCallback(350, [=] { SaveProgress(false); }, TASKTRACK_AUTOSAVE_TIMER_ID);
}

void TaskTrackWindow::OnItemChanged()
{
    if(rebuilding_ || !loaded_)
        return;
    if(document_.state == TaskTrackState::Paused)
        document_.state = TaskTrackState::InProgress;
    TouchHumanActivity();
    RefreshHeaderState();
    RefreshProgress();
    RebuildCategories();
    ScheduleAutosave();
}

void TaskTrackWindow::TogglePause()
{
    if(!loaded_ || document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed)
        return;
    document_.state = document_.state == TaskTrackState::Paused ? TaskTrackState::InProgress : TaskTrackState::Paused;
    TouchHumanActivity();
    SaveProgress(false);
    RefreshHeaderState();
}

void TaskTrackWindow::CompleteTask()
{
    if(!loaded_)
        return;
    Vector<String> missing;
    if(!TaskTrackCanComplete(document_, &missing)) {
        Exclamation("Required questions are still unanswered:\n\n" + Join(missing, "\n"));
        return;
    }
    document_.state = TaskTrackState::Completed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;
    RebuildItems();
    RefreshHeaderState();
    PromptOK("TaskTrack answers saved. The agent can retrieve the completed task.");
}

void TaskTrackWindow::CloseTask()
{
    if(!loaded_ || document_.state == TaskTrackState::Completed)
        return;
    document_.state = TaskTrackState::Closed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;
    RebuildItems();
    RefreshHeaderState();
}

void TaskTrackWindow::ExportMarkdown()
{
    if(!loaded_)
        return;
    FileSel fs;
    fs.Type("Markdown", "*.md");
    fs.Set(AppendFileName(GetFileFolder(task_path_), document_.task_id + ".md"));
    if(!fs.ExecuteSaveAs("Export TaskTrack Markdown"))
        return;
    if(!SaveFile(~fs, TaskTrackExportMarkdown(document_)))
        Exclamation("Unable to export Markdown.");
}

void TaskTrackWindow::ExportJson()
{
    if(!loaded_)
        return;
    FileSel fs;
    fs.Type("JSON", "*.json");
    fs.Set(AppendFileName(GetFileFolder(task_path_), document_.task_id + ".json"));
    if(!fs.ExecuteSaveAs("Export TaskTrack JSON"))
        return;
    if(!SaveFile(~fs, TaskTrackToJson(document_, true)))
        Exclamation("Unable to export JSON.");
}

void TaskTrackWindow::SaveCopy()
{
    if(!loaded_)
        return;
    FileSel fs;
    fs.Type("TaskTrack task", "*.tasktrack.json");
    fs.Set(AppendFileName(GetFileFolder(task_path_), document_.task_id + ".copy.tasktrack.json"));
    if(!fs.ExecuteSaveAs("Save TaskTrack Copy"))
        return;
    if(!SaveFile(~fs, TaskTrackToJson(document_, true)))
        Exclamation("Unable to save TaskTrack copy.");
}

void TaskTrackWindow::ArmReminderTimer()
{
    KillTimeCallback(TASKTRACK_REMINDER_TIMER_ID);
    SetTimeCallback(-30000, [=] { CheckReminder(); }, TASKTRACK_REMINDER_TIMER_ID);
}

int TaskTrackWindow::RunReminderPrompt(bool agent_poll_triggered)
{
    TaskTrackReminderDialog dialog(document_.state == TaskTrackState::Paused, document_.title, agent_poll_triggered);
    return dialog.Run();
}

void TaskTrackWindow::CheckReminder()
{
    if(!loaded_ || document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed)
        return;

    int64 now = GetSysTime().Get();
    int64 idle_seconds = max<int64>(0, now - document_.last_human_activity_epoch);
    bool periodic_due = document_.reminder_minutes > 0 &&
                        idle_seconds >= (int64)document_.reminder_minutes * 60 &&
                        (document_.state != TaskTrackState::Paused || document_.remind_while_paused);

    int64 poll_epoch = TaskTrackReadAgentPollEpoch(task_path_);
    bool agent_due = document_.nudge_on_agent_poll &&
                     poll_epoch > last_seen_agent_poll_epoch_ &&
