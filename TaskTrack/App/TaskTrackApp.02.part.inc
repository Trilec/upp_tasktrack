    exit_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    exit_button_.SetText("Exit").SetContentInset(DPI(4));

    header_layout_.Add(app_heading_).Fit().MinMain(DPI(176)).MinCross(DPI(32));
    header_layout_.Add(state_label_).Fit().MinMain(DPI(102));
    header_layout_.AddSpacer(1).Expand(1).MinMain(DPI(6));
    header_layout_.Add(pause_button_).Fixed(DPI(68)).MinCross(DPI(27));
    header_layout_.Add(reminder_dropdown_).Fit().MinMain(DPI(108)).MinCross(DPI(27));
    header_layout_.Add(paused_reminder_button_).Fit().MinMain(DPI(108)).MinCross(DPI(27));
    header_layout_.Add(agent_nudge_button_).Fit().MinMain(DPI(92)).MinCross(DPI(27));
    header_layout_.Add(exit_button_).Fixed(DPI(54)).MinCross(DPI(27));

    pause_button_.WhenAction = [=] { TogglePause(); };
    reminder_dropdown_.WhenSelect = [=](int) {
        if(!loaded_)
            return;
        document_.reminder_minutes = (int)reminder_dropdown_.GetSelectedData();
        SaveProgress(true);
        RefreshHeaderState();
    };
    paused_reminder_button_.WhenAction = [=] {
        if(!loaded_)
            return;
        document_.remind_while_paused = paused_reminder_button_.IsChecked();
        SaveProgress(true);
        RefreshHeaderState();
    };
    agent_nudge_button_.WhenAction = [=] {
        if(!loaded_)
            return;
        document_.nudge_on_agent_poll = agent_nudge_button_.IsChecked();
        last_seen_agent_poll_epoch_ = TaskTrackReadAgentPollEpoch(task_path_);
        SaveProgress(true);
        RefreshHeaderState();
    };
    exit_button_.WhenAction = [=] {
        if(loaded_)
            SaveProgress(false);
        Close();
    };
}

void TaskTrackWindow::BuildObjective()
{
    objective_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(8))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    objective_card_.SetCustomStyle(MakeTitleStyle(SansSerifZ(11).Bold(), SansSerifZ(9), UiRole::Standard));
    objective_card_.SetTitle("No task loaded").SetSubTitle("").SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

    objective_progress_.SetCustomStyle(CompactLabelStyle(UiRole::Accent));
    objective_progress_.SetText("0 / 0").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

    objective_layout_.Add(objective_card_).Expand(1).MinMain(DPI(360)).MinCross(DPI(44));
    objective_layout_.Add(objective_progress_).Fit().MinMain(DPI(104)).MinCross(DPI(26));
}

void TaskTrackWindow::BuildCategories()
{
    categories_group_.SetCustomStyle(MakeCategoryGroupStyle());
    categories_group_.SetTitle("Categories").SetSubTitle("").SetHeaderMode(UiGroupPanel::Inside);
    categories_group_.SetContent(categories_flow_);

    categories_flow_.SetDirection(UiDirection::H)
        .SetGap(DPI(4), DPI(4))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetFixedColumn(DPI(148))
        .SetAlignItems(UiCrossAlign::Center);
}

void TaskTrackWindow::BuildTaskArea()
{
    task_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    task_scroll_.Content().Add(task_flow_.SizePos());

    task_flow_.SetDirection(UiDirection::H)
        .SetGap(DPI(6), DPI(6))
        .SetInset(DPI(1))
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetFixedColumn(DPI(350));

    empty_label_.SetText("No questions in this category.");
    empty_label_.SetCustomStyle(CompactLabelStyle(UiRole::Subtle));
    empty_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    task_scroll_.Add(empty_label_.HCenterPosZ(0, DPI(300)).VCenterPosZ(0, DPI(22)));
}

void TaskTrackWindow::BuildFooter()
{
    footer_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(7))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    progress_label_.SetCustomStyle(CompactLabelStyle(UiRole::Subtle));
    progress_label_.SetText("No task loaded");

    save_button_.SetCustomStyle(CompactButtonStyle(UiRole::Standard));
    save_button_.SetText("Save");
    save_button_.SetContentInset(DPI(4));
    save_button_.SetSplitWidth(DPI(26)).SetPopupMinWidth(DPI(180));
    save_button_.ClearItems()
        .Add("Export Markdown", "markdown")
        .Add("Export JSON", "json")
        .Add("Save Copy...", "copy");

    complete_button_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
    complete_button_.SetText("Submit answers").SetContentInset(DPI(4));

    footer_layout_.Add(progress_label_).Expand(1).MinMain(DPI(300));
    footer_layout_.Add(save_button_).Fixed(DPI(88)).MinCross(DPI(28));
    footer_layout_.Add(complete_button_).Fixed(DPI(122)).MinCross(DPI(28));

    save_button_.WhenAction = [=] { if(loaded_) SaveProgress(true); };
    save_button_.WhenSelect = [=](int, const Value& data) {
        String action = AsString(data);
        if(action == "markdown") ExportMarkdown();
        else if(action == "json") ExportJson();
        else if(action == "copy") SaveCopy();
    };
    complete_button_.WhenAction = [=] { CompleteTask(); };
}

bool TaskTrackWindow::LoadTask(const String& path, String& error)
{
    TaskTrackDocument loaded;
    if(!TaskTrackLoad(path, loaded, error))
        return false;

    document_ = pick(loaded);
    task_path_ = NormalizePath(path);
    loaded_ = true;
    selected_category_ = "All";
    last_seen_agent_poll_epoch_ = TaskTrackReadAgentPollEpoch(task_path_);

    if(document_.state == TaskTrackState::AwaitingHuman) {
        document_.state = TaskTrackState::InProgress;
        TouchHumanActivity();
        if(!TaskTrackSave(task_path_, document_, error)) {
            loaded_ = false;
            return false;
        }
    }

    RefreshAll();
    return true;
}

void TaskTrackWindow::RefreshAll()
{
    if(!loaded_)
        return;
    objective_card_.SetTitle(document_.title).SetSubTitle(document_.subtitle);
    app_heading_.SetSubTitle(document_.project.IsEmpty() ? String("Human input") : document_.project);
    reminder_dropdown_.SelectByData(document_.reminder_minutes);
    paused_reminder_button_.SetChecked(document_.remind_while_paused);
    agent_nudge_button_.SetChecked(document_.nudge_on_agent_poll);
    RebuildCategories();
    RebuildItems();
    RefreshHeaderState();
    RefreshProgress();
}

void TaskTrackWindow::RefreshHeaderState()
{
    if(!loaded_)
        return;

    state_label_.SetText(TaskTrackStateName(document_.state));
    UiRole role = UiRole::Accent;
    if(document_.state == TaskTrackState::Paused)
        role = UiRole::Subtle;
    else if(document_.state == TaskTrackState::Closed)
        role = UiRole::Alert;
    state_label_.SetCustomStyle(CompactLabelStyle(role));

    bool terminal = document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed;
    pause_button_.Enable(!terminal);
    pause_button_.SetText(document_.state == TaskTrackState::Paused ? "Resume" : "Pause");
    complete_button_.SetText(document_.state == TaskTrackState::Completed ? "Submitted" : "Submit answers");
    complete_button_.Enable(!terminal);
    save_button_.Enable(true);
    reminder_dropdown_.Enable(!terminal);
    paused_reminder_button_.Enable(!terminal);
    agent_nudge_button_.Enable(!terminal);
}

void TaskTrackWindow::RefreshProgress()
{
    if(!loaded_)
        return;

    int answered = TaskTrackAnsweredCount(document_);
    int total = document_.items.GetCount();
    int req_answered = TaskTrackRequiredAnsweredCount(document_);
    int req_total = TaskTrackRequiredCount(document_);

    objective_progress_.SetText(Format("%d / %d answered", answered, total));
    progress_label_.SetText(Format("%d/%d questions · %d/%d required · %s",
                                   answered, total, req_answered, req_total,
                                   TaskTrackStateName(document_.state)));
    complete_button_.Enable(document_.state != TaskTrackState::Completed &&
                            document_.state != TaskTrackState::Closed);
}

void TaskTrackWindow::RebuildCategories()
{
    rebuilding_ = true;
    categories_flow_.ClearItems();
    category_buttons_.Clear();

