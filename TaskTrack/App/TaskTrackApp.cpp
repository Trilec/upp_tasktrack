#include "TaskTrackApp.h"

namespace Upp {

namespace {

static const int TASKTRACK_REMINDER_TIMER_ID = 1;
static const int TASKTRACK_AUTOSAVE_TIMER_ID = 2;

UiTitleCard::Style CompactTitleStyle(UiRole role, Font title_font, Font subtitle_font)
{
    UiTitleCard::Style style = UiTheme::ResolveTitleCard(role);
    style.title_font = title_font;
    style.subtitle_font = subtitle_font;
    style.metrics.face_enabled = false;
    style.metrics.frame_enabled = false;
    style.metrics.radius = 0;
    style.title_line = false;
    style.card_line = false;
    return style;
}

UiButton::Style CompactButtonStyle(UiRole role)
{
    UiButton::Style style = UiTheme::ResolveButton(role);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    return style;
}

UiButton::Style TaskTrackAnsweredButtonStyle()
{
    UiButton::Style style = CompactButtonStyle(UiRole::Standard);
    Color green = Color(45, 142, 77);
    style.metrics.face_enabled = true;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = DPI(2);
    for(int st = 0; st < 4; ++st) {
        style.palette.face[st] = UiFill::Solid(Blend(SColorPaper(), green, st == ST_DISABLED ? 16 : 28));
        style.palette.frame[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 100) : green;
        style.palette.ink[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 120) : green;
    }
    return style;
}

UiLabel::Style CompactLabelStyle(UiRole role, int px = 9)
{
    UiLabel::Style style = UiTheme::ResolveLabel(role);
    style.font = SansSerifZ(px);
    return style;
}

UiDropdown::Style CompactDropdownStyle()
{
    UiDropdown::Style style = UiTheme::ResolveDropdown(UiRole::Standard);
    style.font = SansSerifZ(9);
    style.popup_item_style.font = SansSerifZ(9);
    style.popup_item_height = DPI(24);
    return style;
}

UiCheckBox::Style CompactCheckStyle()
{
    UiCheckBox::Style style = UiTheme::ResolveCheckBox(UiRole::Standard);
    style.font = SansSerifZ(9);
    style.indicator_size = DPI(15);
    style.indicator_gap = DPI(6);
    return style;
}

class TaskTrackReminderDialog : public TopWindow {
public:
    TaskTrackReminderDialog(bool paused, const String& task_title, bool agent_poll)
    {
        Title("TaskTrack Reminder");
        SetRect(0, 0, DPI(560), DPI(224));
        SetMinSize(Size(DPI(520), DPI(208)));

        Add(box_.SizePos());
        box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(11));

        heading_.SetCustomStyle(CompactTitleStyle(UiRole::Accent, SansSerifZ(11).Bold(), SansSerifZ(9)));
        heading_.SetTitle(paused ? "Task is paused" : "Still working on this task?");
        heading_.SetSubTitle(agent_poll ? "The agent checked this task while you were inactive."
                                         : "TaskTrack noticed a period without activity.");
        heading_.SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

        detail_.SetText(task_title);
        detail_.SetCustomStyle(CompactLabelStyle(UiRole::Subtle));
        detail_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        keep_reminding_.SetCustomStyle(CompactCheckStyle());
        keep_reminding_.SetText("Keep reminding me about this task");
        keep_reminding_.SetChecked(true);

        buttons_.SetDirection(UiDirection::H).SetGap(DPI(7)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

        continue_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
        continue_.SetText(paused ? "Resume" : "Keep working").SetContentInset(DPI(4));
        accept_.SetCustomStyle(CompactButtonStyle(UiRole::Standard));
        accept_.SetText("Accept suggestions").SetContentInset(DPI(4));
        pause_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        pause_.SetText(paused ? "Keep paused" : "Pause for now").SetContentInset(DPI(4));
        close_.SetCustomStyle(CompactButtonStyle(UiRole::Alert));
        close_.SetText("Close task").SetContentInset(DPI(4));

        box_.Add(heading_).Fit().MinCross(DPI(46)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.Add(detail_).Fit().MinCross(DPI(22)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.Add(keep_reminding_).Fit().MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.AddSpacer(1).Expand(1).MinMain(DPI(6));
        box_.Add(buttons_).Fit().MinCross(DPI(32)).AlignSelf(UiBoxLayout::Align::Stretch);
        buttons_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        buttons_.Add(continue_).Fixed(DPI(100)).MinCross(DPI(28));
        buttons_.Add(accept_).Fixed(DPI(118)).MinCross(DPI(28));
        buttons_.Add(pause_).Fixed(DPI(104)).MinCross(DPI(28));
        buttons_.Add(close_).Fixed(DPI(96)).MinCross(DPI(28));

        continue_.WhenAction = [=] { Break(1); };
        accept_.WhenAction = [=] { Break(4); };
        pause_.WhenAction = [=] { Break(2); };
        close_.WhenAction = [=] { Break(3); };
    }

    bool KeepReminding() const { return keep_reminding_.IsChecked(); }

private:
    UiBoxLayout box_ { UiDirection::V };
    UiTitleCard heading_;
    UiLabel detail_;
    UiCheckBox keep_reminding_;
    UiBoxLayout buttons_ { UiDirection::H };
    UiButton continue_;
    UiButton accept_;
    UiButton pause_;
    UiButton close_;
};

class TaskTrackExitDialog : public TopWindow {
public:
    TaskTrackExitDialog(const String& task_title)
    {
        Title("Exit TaskTrack");
        SetRect(0, 0, DPI(520), DPI(196));
        SetMinSize(Size(DPI(460), DPI(180)));

        Add(box_.SizePos());
        box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(11));

        heading_.SetCustomStyle(CompactTitleStyle(UiRole::Accent, SansSerifZ(11).Bold(), SansSerifZ(9)));
        heading_.SetTitle("Task is not complete yet");
        heading_.SetSubTitle("Your answers are safe. The agent will see the task is not yet accepted.");
        heading_.SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

        detail_.SetText(task_title);
        detail_.SetCustomStyle(CompactLabelStyle(UiRole::Subtle));
        detail_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        buttons_.SetDirection(UiDirection::H).SetGap(DPI(7)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

        accept_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
        accept_.SetText("Accept suggestions & finish").SetContentInset(DPI(4));
        leave_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        leave_.SetText("Exit without accepting").SetContentInset(DPI(4));
        keep_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        keep_.SetText("Keep working").SetContentInset(DPI(4));

        box_.Add(heading_).Fit().MinCross(DPI(46)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.Add(detail_).Fit().MinCross(DPI(22)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.AddSpacer(1).Expand(1).MinMain(DPI(6));
        box_.Add(buttons_).Fit().MinCross(DPI(32)).AlignSelf(UiBoxLayout::Align::Stretch);
        buttons_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        buttons_.Add(accept_).Fixed(DPI(158)).MinCross(DPI(28));
        buttons_.Add(leave_).Fixed(DPI(150)).MinCross(DPI(28));
        buttons_.Add(keep_).Fixed(DPI(96)).MinCross(DPI(28));

        accept_.WhenAction = [=] { Break(1); };
        leave_.WhenAction = [=] { Break(2); };
        keep_.WhenAction = [=] { Break(0); };
    }

private:
    UiBoxLayout box_ { UiDirection::V };
    UiTitleCard heading_;
    UiLabel detail_;
    UiBoxLayout buttons_ { UiDirection::H };
    UiButton accept_;
    UiButton leave_;
    UiButton keep_;
};

} // namespace

TaskTrackWindow::TaskTrackWindow()
{
    Title("TaskTrack");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1180), DPI(780));
    SetMinSize(Size(DPI(760), DPI(540)));
    WhenClose = [=] { RequestExit(); };
    BuildUi();
}

TaskTrackWindow::~TaskTrackWindow()
{
    KillTimeCallback(TASKTRACK_AUTOSAVE_TIMER_ID);
    KillTimeCallback(TASKTRACK_REMINDER_TIMER_ID);
    if(loaded_) {
        String ignored;
        document_.updated_at = TaskTrackNowIso();
        TaskTrackSave(task_path_, document_, ignored);
    }
}

UiTitleCard::Style TaskTrackWindow::MakeTitleStyle(Font title_font, Font subtitle_font, UiRole role) const
{
    return CompactTitleStyle(role, title_font, subtitle_font);
}

UiButton::Style TaskTrackWindow::MakeCategoryButtonStyle(bool selected, bool needs_attention) const
{
    UiRole role = needs_attention ? UiRole::Alert : (selected ? UiRole::Accent : UiRole::Subtle);
    UiButton::Style style = UiTheme::ResolveButton(role);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    style.metrics.radius = DPI(5);
    style.metrics.frame_width = needs_attention ? DPI(2) : DPI(1);
    return style;
}

UiGroupPanel::Style TaskTrackWindow::MakeCategoryGroupStyle() const
{
    UiGroupPanel::Style style = UiTheme::ResolveGroupPanel(UiRole::Standard);
    style.title_font = SansSerifZ(9).Bold();
    style.subtitle_font = SansSerifZ(8);
    style.metrics.radius = DPI(6);
    style.metrics.frame_width = DPI(1);
    style.inset = Rect(DPI(6), DPI(5), DPI(6), DPI(10));
    style.header_inset = Rect(DPI(6), DPI(3), DPI(6), DPI(2));
    style.header_gap = DPI(2);
    style.title_subtitle_gap = 0;
    style.header_band_enabled = false;
    style.line_enabled = true;
    return style;
}

void TaskTrackWindow::BuildUi()
{
    Add(main_box_.SizePos());
    main_box_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7));

    BuildHeader();
    BuildCategories();
    BuildTaskArea();
    BuildFooter();

    main_box_.Add(header_layout_).Fit().MinCross(DPI(44)).AlignSelf(UiBoxLayout::Align::Stretch);
    categories_item_ = main_box_.Add(categories_group_);
    categories_item_.Fit().MinMain(DPI(64)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(task_scroll_).Expand(1).MinMain(DPI(300)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(footer_layout_).Fit().MinCross(DPI(32)).AlignSelf(UiBoxLayout::Align::Stretch);

    ArmReminderTimer();
}

void TaskTrackWindow::BuildHeader()
{
    header_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(6))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    app_heading_.SetCustomStyle(MakeTitleStyle(SansSerifZ(12).Bold(), SansSerifZ(8), UiRole::Accent));
    app_heading_.SetTitle("TaskTrack")
        .SetSubTitle("Human input")
        .SetContentInset(0)
        .SetMediaReserve(DPI(24))
        .SetMediaMin(DPI(18))
        .SetMediaGap(DPI(5))
        .SetMediaAutoFit(false)
        .SetMediaSide(UiAlign::LEFT);
    app_heading_.SetMedia(ICON_DESIGN_CHECK_SMALL_48(), Size(DPI(18), DPI(18)));

    objective_card_.SetCustomStyle(MakeTitleStyle(SansSerifZ(10).Bold(), SansSerifZ(8), UiRole::Standard));
    objective_card_.SetTitle("No task loaded")
        .SetSubTitle("")
        .SetContentInset(0)
        .ShowTitleLine(false)
        .ShowCardLine(false);

    state_label_.SetCustomStyle(CompactLabelStyle(UiRole::Accent));
    state_label_.SetText("No task").SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    objective_progress_.SetCustomStyle(CompactLabelStyle(UiRole::Accent));
    objective_progress_.SetText("0 / 0 answered").SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    pause_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    pause_button_.SetText("Pause").SetContentInset(DPI(4));

    reminder_dropdown_.SetCustomStyle(CompactDropdownStyle());
    reminder_dropdown_.SetSizeMin(DPI(108), DPI(27));
    reminder_dropdown_.UseInternalModel().Clear()
        .Add("Reminder off", 0)
        .Add("1 minute", 1)
        .Add("10 minutes", 10)
        .Add("15 minutes", 15)
        .Add("30 minutes", 30)
        .Add("1 hour", 60)
        .Add("2 hours", 120)
        .Add("4 hours", 240);
    reminder_dropdown_.SelectByData(10);

    paused_reminder_button_.SetCheckable(true);
    paused_reminder_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    paused_reminder_button_.SetText("Paused remind").SetContentInset(DPI(4));

    agent_nudge_button_.SetCheckable(true);
    agent_nudge_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    agent_nudge_button_.SetText("Agent nudge").SetContentInset(DPI(4));

    exit_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    exit_button_.SetText("Exit").SetContentInset(DPI(4));

    header_layout_.Add(app_heading_).Fit().MinMain(DPI(150)).MinCross(DPI(40));
    header_layout_.Add(objective_card_).Expand(1).MinMain(DPI(240)).MinCross(DPI(40));
    header_layout_.Add(state_label_).Fit().MinMain(DPI(74)).MinCross(DPI(27));
    header_layout_.Add(objective_progress_).Fit().MinMain(DPI(90)).MinCross(DPI(27));
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
    exit_button_.WhenAction = [=] { RequestExit(); };
}

void TaskTrackWindow::BuildCategories()
{
    categories_group_.SetCustomStyle(MakeCategoryGroupStyle());
    categories_group_.SetTitle("Categories").SetSubTitle("").SetHeaderMode(UiGroupPanel::Inside);
    categories_group_.SetContent(categories_flow_);

    categories_flow_.SetDirection(UiDirection::H)
        .SetGap(DPI(6), DPI(6))
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
        .SetGap(DPI(10), DPI(10))
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

    accept_recommendations_button_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
    accept_recommendations_button_.SetText("Accept suggestions").SetContentInset(DPI(4));

    complete_button_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
    complete_button_.SetText("Submit answers").SetContentInset(DPI(4));

    footer_layout_.Add(progress_label_).Expand(1).MinMain(DPI(260));
    footer_layout_.Add(save_button_).Fixed(DPI(88)).MinCross(DPI(28));
    footer_layout_.Add(accept_recommendations_button_).Fixed(DPI(150)).MinCross(DPI(28));
    footer_layout_.Add(complete_button_).Fixed(DPI(122)).MinCross(DPI(28));

    save_button_.WhenAction = [=] { if(loaded_) SaveProgress(true); };
    save_button_.WhenSelect = [=](int, const Value& data) {
        String action = AsString(data);
        if(action == "markdown") ExportMarkdown();
        else if(action == "json") ExportJson();
        else if(action == "copy") SaveCopy();
    };
    accept_recommendations_button_.WhenAction = [=] { AcceptRecommendations(); };
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
    review_required_ = false;
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

int TaskTrackWindow::CountMissingRequired(const String& category) const
{
    int count = 0;
    for(const TaskTrackItem& item : document_.items) {
        String item_category = item.category.IsEmpty() ? String("General") : item.category;
        if(!category.IsEmpty() && category != "All" && item_category != category)
            continue;
        if(item.required && !item.answer.answered)
            ++count;
    }
    return count;
}

String TaskTrackWindow::FirstMissingRequiredCategory() const
{
    for(const TaskTrackItem& item : document_.items) {
        if(item.required && !item.answer.answered)
            return item.category.IsEmpty() ? String("General") : item.category;
    }
    return String();
}

void TaskTrackWindow::ActivateRequiredReview()
{
    review_required_ = CountMissingRequired() > 0;
    if(review_required_) {
        String category = FirstMissingRequiredCategory();
        if(!category.IsEmpty())
            selected_category_ = category;
    }
}

void TaskTrackWindow::RefreshQuestionVisualStates()
{
    for(TaskTrackQuestionCtrl& ctrl : question_controls_) {
        int index = ctrl.GetItemIndex();
        if(index < 0 || index >= document_.items.GetCount())
            continue;
        const TaskTrackItem& item = document_.items[index];
        ctrl.SetNeedsAttention(review_required_ && item.required && !item.answer.answered);
        ctrl.RefreshVisualState();
    }
}

void TaskTrackWindow::RefreshProgress()
{
    if(!loaded_)
        return;

    int answered = TaskTrackAnsweredCount(document_);
    int total = document_.items.GetCount();
    int req_answered = TaskTrackRequiredAnsweredCount(document_);
    int req_total = TaskTrackRequiredCount(document_);
    int missing_required = req_total - req_answered;

    bool has_any_recommendation = false;
    bool has_unanswered_recommendation = false;
    for(const TaskTrackItem& item : document_.items) {
        if(item.recommended.IsEmpty())
            continue;
        has_any_recommendation = true;
        if(!item.answer.answered)
            has_unanswered_recommendation = true;
    }

    objective_progress_.SetText(Format("%d / %d answered", answered, total));
    progress_label_.SetText(Format("%d/%d questions · %d/%d required · %s",
                                   answered, total, req_answered, req_total,
                                   TaskTrackStateName(document_.state)));

    bool terminal = document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed;
    complete_button_.Enable(!terminal);

    if(terminal) {
        accept_recommendations_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        accept_recommendations_button_.SetText("Suggestions closed").Disable();
    }
    else if(review_required_ && missing_required > 0) {
        accept_recommendations_button_.SetCustomStyle(CompactButtonStyle(UiRole::Alert));
        accept_recommendations_button_.SetText(Format("Review %d required", missing_required)).Enable();
    }
    else if(has_unanswered_recommendation) {
        accept_recommendations_button_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
        accept_recommendations_button_.SetText("Accept suggestions").Enable();
    }
    else if(has_any_recommendation) {
        accept_recommendations_button_.SetCustomStyle(TaskTrackAnsweredButtonStyle());
        accept_recommendations_button_.SetText("Suggestions applied").Disable();
    }
    else {
        accept_recommendations_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        accept_recommendations_button_.SetText("No suggestions").Disable();
    }
}

void TaskTrackWindow::RebuildCategories()
{
    rebuilding_ = true;
    categories_flow_.ClearItems();
    category_buttons_.Clear();

    Vector<String> categories = TaskTrackCategories(document_);
    bool show_categories = categories.GetCount() > 1;
    categories_group_.Show(show_categories);
    if(show_categories)
        categories_item_.Fit().MinMain(DPI(64));
    else
        categories_item_.Fixed(0).MinMain(0);

    auto add_button = [&](const String& category) {
        int total = 0;
        int answered = 0;
        int missing_required = 0;
        for(const TaskTrackItem& item : document_.items) {
            String item_category = item.category.IsEmpty() ? String("General") : item.category;
            if(category != "All" && item_category != category)
                continue;
            ++total;
            if(item.answer.answered)
                ++answered;
            else if(item.required)
                ++missing_required;
        }

        bool needs_attention = review_required_ && missing_required > 0;
        UiButton& button = category_buttons_.Add();
        button.SetCustomStyle(MakeCategoryButtonStyle(category == selected_category_, needs_attention));
        String text = Format("%s  %d/%d", category, answered, total);
        if(needs_attention)
            text << Format(" · %d needed", missing_required);
        button.SetText(text);
        button.SetContentInset(DPI(3));
        String selected = category;
        button.WhenAction = [=] { SelectCategory(selected); };
        categories_flow_.Add(button).Fit().MinMain(DPI(148)).MinCross(DPI(30));
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
        String item_category = item.category.IsEmpty() ? String("General") : item.category;
        if(selected_category_ != "All" && item_category != selected_category_)
            continue;

        TaskTrackQuestionCtrl& ctrl = question_controls_.Add();
        ctrl.Bind(document_, i);
        ctrl.SetNeedsAttention(review_required_ && item.required && !item.answer.answered);
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
    if(review_required_ && CountMissingRequired() == 0)
        review_required_ = false;
    TouchHumanActivity();
    RefreshQuestionVisualStates();
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

void TaskTrackWindow::AcceptRecommendations()
{
    if(!loaded_ || document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed)
        return;

    int applied = 0;
    for(TaskTrackItem& item : document_.items)
        if(TaskTrackApplyRecommendation(item))
            ++applied;

    ActivateRequiredReview();

    if(applied == 0 && !review_required_) {
        RefreshProgress();
        PromptOK("No unanswered agent suggestions are available.");
        return;
    }

    if(document_.state == TaskTrackState::Paused)
        document_.state = TaskTrackState::InProgress;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;
    RebuildCategories();
    RebuildItems();
    RefreshHeaderState();
    RefreshProgress();
}

void TaskTrackWindow::CompleteTask()
{
    if(!loaded_)
        return;
    Vector<String> missing;
    if(!TaskTrackCanComplete(document_, &missing)) {
        ActivateRequiredReview();
        RebuildCategories();
        RebuildItems();
        RefreshProgress();
        Exclamation(Format("%d required question%s still need your input. They are highlighted in red.",
                           missing.GetCount(), missing.GetCount() == 1 ? "" : "s"));
        return;
    }
    review_required_ = false;
    document_.state = TaskTrackState::Completed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;
    RebuildItems();
    RefreshHeaderState();
    RefreshProgress();
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
    RefreshProgress();
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
    int result = dialog.Run();
    if(result > 0 && !dialog.KeepReminding()) {
        document_.reminder_minutes = 0;
        reminder_dropdown_.SelectByData(0);
    }
    return result;
}

bool TaskTrackWindow::ApplyRecommendation(TaskTrackItem& item)
{
    return TaskTrackApplyRecommendation(item);
}

int TaskTrackWindow::TaskTrackExitPrompt()
{
    TaskTrackExitDialog dialog(document_.title);
    return dialog.Run();
}

void TaskTrackWindow::RequestExit()
{
    if(closing_)
        return;
    if(!loaded_ || document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed) {
        if(loaded_)
            SaveProgress(false);
        closing_ = true;
        Close();
        return;
    }

    int result = TaskTrackExitPrompt();
    if(result == EXIT_ACCEPT) {
        for(TaskTrackItem& item : document_.items)
            ApplyRecommendation(item);
        ActivateRequiredReview();
        Vector<String> missing;
        if(TaskTrackCanComplete(document_, &missing)) {
            review_required_ = false;
            document_.state = TaskTrackState::Completed;
            TouchHumanActivity();
            if(SaveProgress(false)) {
                RebuildItems();
                RefreshHeaderState();
                RefreshProgress();
                closing_ = true;
                Close();
            }
        }
        else {
            SaveProgress(false);
            RebuildCategories();
            RebuildItems();
            RefreshHeaderState();
            RefreshProgress();
            Exclamation(Format("%d required question%s still need your input. They are highlighted in red.",
                               missing.GetCount(), missing.GetCount() == 1 ? "" : "s"));
        }
    }
    else if(result == EXIT_LEAVE) {
        if(!SaveProgress(false))
            return;
        RebuildItems();
        RefreshHeaderState();
        RefreshProgress();
        closing_ = true;
        Close();
    }
}

void TaskTrackWindow::CheckReminder()
{
    if(reminder_showing_ || !loaded_ || document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed)
        return;

    int64 now = GetSysTime().Get();
    int64 idle_seconds = max<int64>(0, now - document_.last_human_activity_epoch);
    bool periodic_due = document_.reminder_minutes > 0 &&
                        idle_seconds >= (int64)document_.reminder_minutes * 60 &&
                        (document_.state != TaskTrackState::Paused || document_.remind_while_paused);

    int64 poll_epoch = TaskTrackReadAgentPollEpoch(task_path_);
    bool agent_due = document_.nudge_on_agent_poll &&
                     poll_epoch > last_seen_agent_poll_epoch_ &&
                     idle_seconds >= 60;

    if(!periodic_due && !agent_due)
        return;

    last_seen_agent_poll_epoch_ = max(last_seen_agent_poll_epoch_, poll_epoch);
    reminder_showing_ = true;
    int result = RunReminderPrompt(agent_due);
    reminder_showing_ = false;
    ++document_.reminder_count;
    TouchHumanActivity();

    if(result == REMINDER_CONTINUE)
        document_.state = TaskTrackState::InProgress;
    else if(result == REMINDER_PAUSE)
        document_.state = TaskTrackState::Paused;
    else if(result == REMINDER_CLOSE)
        document_.state = TaskTrackState::Closed;
    else if(result == REMINDER_ACCEPT) {
        for(TaskTrackItem& item : document_.items)
            ApplyRecommendation(item);
        ActivateRequiredReview();
        Vector<String> missing;
        if(TaskTrackCanComplete(document_, &missing)) {
            review_required_ = false;
            document_.state = TaskTrackState::Completed;
            PromptOK("All available agent suggestions accepted. The task is now completed.");
        }
        else
            Exclamation(Format("%d required question%s still need your input. They are highlighted in red.",
                               missing.GetCount(), missing.GetCount() == 1 ? "" : "s"));
    }

    SaveProgress(false);
    if(result == REMINDER_CLOSE || result == REMINDER_ACCEPT) {
        RebuildCategories();
        RebuildItems();
    }
    RefreshHeaderState();
    RefreshProgress();
}

} // namespace Upp
