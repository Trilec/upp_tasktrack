#include "TaskTrackApp.h"

#include <stdio.h>

namespace Upp {

namespace {

static const int TASKTRACK_REMINDER_TIMER_ID = 7011;
static const int TASKTRACK_AUTOSAVE_TIMER_ID = 7012;

UiTitleCard::Style MakeCompactTitleStyle(UiRole role, Font title_font, Font subtitle_font)
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

UiPanel::Style MakeCardPanelStyle()
{
    UiPanel::Style style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    style.metrics.radius = DPI(7);
    style.metrics.frame_width = DPI(1);
    return style;
}

UiButton::Style MakeCompactButtonStyle(UiRole role)
{
    UiButton::Style style = UiTheme::ResolveButton(role);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    return style;
}

UiLabel::Style MakeCompactLabelStyle(UiRole role, int font_px = 9)
{
    UiLabel::Style style = UiTheme::ResolveLabel(role);
    style.font = SansSerifZ(font_px);
    return style;
}

UiDropdown::Style MakeCompactDropdownStyle(const UiDropdown::Style& source)
{
    UiDropdown::Style style = source;
    style.font = SansSerifZ(9);
    style.popup_item_style.font = SansSerifZ(9);
    return style;
}

UiBaseEdit::Style MakeCompactEditStyle(const UiBaseEdit::Style& source)
{
    UiBaseEdit::Style style = source;
    style.font = SansSerifZ(9);
    return style;
}

class TaskTrackReminderDialog : public TopWindow {
public:
    TaskTrackReminderDialog(bool paused, const String& task_title, bool agent_poll)
    {
        Title("TaskTrack Reminder");
        SetRect(0, 0, DPI(520), DPI(210));
        SetMinSize(Size(DPI(460), DPI(190)));

        Add(box_.SizePos());
        box_.SetDirection(UiDirection::V).SetGap(DPI(10)).SetInset(DPI(12));

        heading_.SetCustomStyle(MakeCompactTitleStyle(UiRole::Accent, SansSerifZ(11).Bold(), SansSerifZ(9)));
        heading_.SetTitle(paused ? "Task is paused" : "Still working on this task?");
        heading_.SetSubTitle(agent_poll ? "The agent checked this task while you were inactive."
                                         : "TaskTrack noticed a period without activity.");
        heading_.SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

        detail_.SetText(task_title);
        detail_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Subtle));
        detail_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

        buttons_.SetDirection(UiDirection::H).SetGap(DPI(8)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

        continue_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Accent));
        continue_.SetText(paused ? "Resume" : "Continue").SetContentInset(DPI(5));
        pause_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Subtle));
        pause_.SetText(paused ? "Keep paused" : "Pause").SetContentInset(DPI(5));
        close_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Alert));
        close_.SetText("Close task").SetContentInset(DPI(5));

        box_.Add(heading_).Fit().MinCross(DPI(48)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.Add(detail_).Fit().MinCross(DPI(24)).AlignSelf(UiBoxLayout::Align::Stretch);
        {
            auto spacer = box_.AddSpacer(1);
            spacer.Expand(1).MinMain(DPI(8));
        }
        box_.Add(buttons_).Fit().MinCross(DPI(34)).AlignSelf(UiBoxLayout::Align::Stretch);
        {
            auto spacer = buttons_.AddSpacer(1);
            spacer.Expand(1).MinMain(DPI(10));
        }
        buttons_.Add(continue_).Fixed(DPI(100)).MinCross(DPI(30));
        buttons_.Add(pause_).Fixed(DPI(110)).MinCross(DPI(30));
        buttons_.Add(close_).Fixed(DPI(100)).MinCross(DPI(30));

        continue_.WhenAction = [=] { Break(1); };
        pause_.WhenAction = [=] { Break(2); };
        close_.WhenAction = [=] { Break(3); };
    }

private:
    UiBoxLayout box_ { UiDirection::V };
    UiTitleCard heading_;
    UiLabel detail_;
    UiBoxLayout buttons_ { UiDirection::H };
    UiButton continue_;
    UiButton pause_;
    UiButton close_;
};

} // namespace

TaskTrackItemCtrl::TaskTrackItemCtrl()
{
    SetCustomStyle(MakeCardPanelStyle());
    Add(body_.SizePos());
    body_.SetDirection(UiDirection::V).SetGap(DPI(7)).SetInset(DPI(9));

    heading_.SetCustomStyle(MakeCompactTitleStyle(UiRole::Standard, SansSerifZ(10).Bold(), SansSerifZ(9)));
    heading_.SetContentInset(0).ShowTitleLine(false).ShowCardLine(false).SetSelectable(false);

    expected_label_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Subtle));
    expected_label_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    response_row_.SetDirection(UiDirection::H)
        .SetGap(DPI(6), DPI(6))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    UiCheckBox::Style check_style = check_button_.GetStyle();
    check_style.font = SansSerifZ(9);
    check_button_.SetCustomStyle(check_style);
    check_button_.SetText("Confirm");

    status_dropdown_.SetCustomStyle(MakeCompactDropdownStyle(status_dropdown_.GetStyle()));
    status_dropdown_.SetSizeMin(DPI(160), DPI(28));
    value_edit_.SetCustomStyle(MakeCompactEditStyle(value_edit_.GetStyle()));
    multiline_edit_.SetCustomStyle(MakeCompactEditStyle(multiline_edit_.GetStyle()));
    note_edit_.SetCustomStyle(MakeCompactEditStyle(note_edit_.GetStyle()));
    value_edit_.SetPlaceholder("Enter response");
    multiline_edit_.SetPlaceholder("Enter response");
    note_edit_.SetPlaceholder("Notes (optional)");

    expected_color_.SetLabelStyle(MakeCompactLabelStyle(UiRole::Subtle));
    expected_color_.SetValueStyle(MakeCompactLabelStyle(UiRole::Subtle));
    expected_color_.SetLabel("Expected").SetColorCount(1).ShowValue(true);
    expected_color_.Disable();

    body_.Add(heading_).Fit().MinCross(DPI(48)).AlignSelf(UiBoxLayout::Align::Stretch);
    body_.Add(expected_label_).Fit().MinCross(DPI(20)).AlignSelf(UiBoxLayout::Align::Stretch);
    body_.Add(expected_color_).Fit().MinCross(DPI(30)).AlignSelf(UiBoxLayout::Align::Stretch);
    body_.Add(response_row_).Fit().MinCross(DPI(30)).AlignSelf(UiBoxLayout::Align::Stretch);
    response_row_.Add(check_button_).Fit().MinMain(DPI(130)).MinCross(DPI(28));
    response_row_.Add(status_dropdown_).Fit().MinMain(DPI(170)).MinCross(DPI(28));
    response_row_.Add(value_edit_).Expand(1).MinMain(DPI(190)).MinCross(DPI(28));
    body_.Add(multiline_edit_).Fit().MinMain(DPI(78)).MinCross(DPI(78)).AlignSelf(UiBoxLayout::Align::Stretch);
    body_.Add(note_edit_).Fit().MinCross(DPI(28)).AlignSelf(UiBoxLayout::Align::Stretch);

    check_button_.WhenAction = [=] { Commit(); };
    status_dropdown_.WhenSelect = [=](int) { Commit(); };
    value_edit_.WhenChange = [=] { if(!syncing_) Commit(); };
    value_edit_.WhenAction = [=] { if(!syncing_) Commit(); };
    multiline_edit_.WhenChange = [=] { if(!syncing_) Commit(); };
    multiline_edit_.WhenAction = [=] { if(!syncing_) Commit(); };
    note_edit_.WhenChange = [=] { if(!syncing_) Commit(); };
    note_edit_.WhenAction = [=] { if(!syncing_) Commit(); };
}

void TaskTrackItemCtrl::Bind(TaskTrackDocument& document, int item_index)
{
    document_ = &document;
    item_index_ = item_index;
    Configure();
    SyncFromModel();
}

bool TaskTrackItemCtrl::ParseExpectedColor(Color& color) const
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return false;
    String s = document_->items[item_index_].expected_color;
    if(s.StartsWith("#"))
        s = s.Mid(1);
    if(s.GetCount() < 6)
        return false;
    unsigned r = 0, g = 0, b = 0;
    if(sscanf(~s, "%02x%02x%02x", &r, &g, &b) != 3)
        return false;
    color = Color((int)r, (int)g, (int)b);
    return true;
}

void TaskTrackItemCtrl::FillStatusOptions()
{
    status_dropdown_.UseInternalModel().Clear();
    if(!document_ || item_index_ < 0)
        return;

    const TaskTrackItem& item = document_->items[item_index_];
    switch(item.type) {
    case TaskTrackItemType::PassFail:
        status_dropdown_.Add("Pass", "pass").Add("Fail", "fail").Add("Blocked", "blocked").Add("Not applicable", "not_applicable");
        break;
    case TaskTrackItemType::Choice:
        for(const String& choice : item.choices)
            status_dropdown_.Add(choice, choice);
        break;
    case TaskTrackItemType::Color:
    case TaskTrackItemType::VisualCompare:
        status_dropdown_.Add("Match", "match").Add("Different", "different").Add("Unsure", "unsure");
        break;
    case TaskTrackItemType::File:
        status_dropdown_.Add("Found", "found").Add("Missing", "missing").Add("Wrong output", "wrong_output").Add("Unsure", "unsure");
        break;
    case TaskTrackItemType::Interaction:
        status_dropdown_.Add("Pass", "pass").Add("Fail", "fail").Add("Partial", "partial").Add("Blocked", "blocked");
        break;
    default:
        break;
    }
}

void TaskTrackItemCtrl::Configure()
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    const TaskTrackItem& item = document_->items[item_index_];
    String subtitle = item.instruction;
    if(item.required)
        subtitle << (subtitle.IsEmpty() ? "" : "  ") << "Required";
    heading_.SetTitle(item.title).SetSubTitle(subtitle);

    expected_label_.Hide();
    expected_color_.Hide();
    check_button_.Hide();
    status_dropdown_.Hide();
    value_edit_.Hide();
    multiline_edit_.Hide();
    note_edit_.Hide();

    if(!item.expected_value.IsEmpty()) {
        expected_label_.SetText("Expected: " + item.expected_value);
        expected_label_.Show();
    }

    FillStatusOptions();

    switch(item.type) {
    case TaskTrackItemType::Check:
        check_button_.Show();
        break;
    case TaskTrackItemType::PassFail:
    case TaskTrackItemType::Choice:
    case TaskTrackItemType::File:
    case TaskTrackItemType::Interaction:
    case TaskTrackItemType::VisualCompare:
        status_dropdown_.Show();
        break;
    case TaskTrackItemType::Text:
    case TaskTrackItemType::Number:
        value_edit_.Show();
        break;
    case TaskTrackItemType::Multiline:
        multiline_edit_.Show();
        break;
    case TaskTrackItemType::Color:
        expected_color_.Show();
        status_dropdown_.Show();
        break;
    }
    note_edit_.Show();
    RefreshLayout();
}

void TaskTrackItemCtrl::SyncCheckButton()
{
    if(!document_ || item_index_ < 0)
        return;
    const TaskTrackItem& item = document_->items[item_index_];
    check_button_.SetChecked(item.answer.answered && item.answer.status == "confirmed");
}

void TaskTrackItemCtrl::SyncFromModel()
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    syncing_ = true;
    const TaskTrackItem& item = document_->items[item_index_];

    SyncCheckButton();
    status_dropdown_.ClearSelection();
    if(!item.answer.value.IsEmpty())
        status_dropdown_.SelectByData(item.answer.value);
    value_edit_.SetTextUtf8(item.answer.value);
    multiline_edit_.SetTextUtf8(item.answer.value);
    note_edit_.SetTextUtf8(item.answer.note);

    Color expected;
    if(ParseExpectedColor(expected)) {
        expected_color_.SetColor(0, expected);
        expected_color_.SetValueText(item.expected_color);
    }

    syncing_ = false;
}

void TaskTrackItemCtrl::Commit()
{
    if(syncing_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    TaskTrackItem& item = document_->items[item_index_];
    TaskTrackAnswer answer = item.answer;

    switch(item.type) {
    case TaskTrackItemType::Check:
        answer.answered = check_button_.IsChecked();
        answer.status = check_button_.IsChecked() ? "confirmed" : "";
        answer.value = check_button_.IsChecked() ? "true" : "";
        break;
    case TaskTrackItemType::PassFail:
    case TaskTrackItemType::Choice:
    case TaskTrackItemType::Color:
    case TaskTrackItemType::File:
    case TaskTrackItemType::Interaction:
    case TaskTrackItemType::VisualCompare:
        if(status_dropdown_.HasSelection()) {
            answer.answered = true;
            answer.status = status_dropdown_.GetSelectedText();
            answer.value = AsString(status_dropdown_.GetSelectedData());
        }
        else {
            answer.answered = false;
            answer.status.Clear();
            answer.value.Clear();
        }
        break;
    case TaskTrackItemType::Text:
    case TaskTrackItemType::Number:
        answer.value = TrimBoth(value_edit_.GetTextUtf8());
        answer.status = answer.value.IsEmpty() ? "" : "recorded";
        answer.answered = !answer.value.IsEmpty();
        break;
    case TaskTrackItemType::Multiline:
        answer.value = TrimBoth(multiline_edit_.GetTextUtf8());
        answer.status = answer.value.IsEmpty() ? "" : "recorded";
        answer.answered = !answer.value.IsEmpty();
        break;
    }

    answer.note = TrimBoth(note_edit_.GetTextUtf8());
    if(answer.answered)
        answer.answered_at = TaskTrackNowIso();
    else
        answer.answered_at.Clear();
    item.answer = pick(answer);

    if(WhenChanged)
        WhenChanged();
}

TaskTrackWindow::TaskTrackWindow()
{
    Title("TaskTrack");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1180), DPI(780));
    SetMinSize(Size(DPI(820), DPI(600)));
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
    return MakeCompactTitleStyle(role, title_font, subtitle_font);
}

UiButton::Style TaskTrackWindow::MakeCategoryButtonStyle(bool selected) const
{
    UiButton::Style style = UiTheme::ResolveButton(selected ? UiRole::Accent : UiRole::Subtle);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    style.metrics.radius = DPI(6);
    style.metrics.frame_width = DPI(1);
    return style;
}

void TaskTrackWindow::BuildUi()
{
    Add(main_box_.SizePos());
    main_box_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(8));

    BuildHeader();
    BuildObjective();
    BuildCategories();
    BuildTaskArea();
    BuildFooter();

    main_box_.Add(header_layout_).Fit().MinCross(DPI(38)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(objective_panel_).Fit().MinMain(DPI(76)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(categories_panel_).Fixed(DPI(112)).MinMain(DPI(92)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(task_panel_).Expand(1).MinMain(DPI(300)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(footer_layout_).Fit().MinCross(DPI(36)).AlignSelf(UiBoxLayout::Align::Stretch);

    ArmReminderTimer();
}

void TaskTrackWindow::BuildHeader()
{
    header_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(7))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    app_heading_.SetCustomStyle(MakeTitleStyle(SansSerifZ(11).Bold(), SansSerifZ(9), UiRole::Accent));
    app_heading_.SetTitle("TaskTrack")
        .SetSubTitle("Human verification")
        .SetContentInset(DPI(1))
        .SetMediaReserve(DPI(24))
        .SetMediaMin(DPI(16))
        .SetMediaGap(DPI(6))
        .SetMediaAutoFit(false)
        .SetMediaSide(UiAlign::LEFT);
    app_heading_.SetMedia(ICON_DESIGN_CHECK_SMALL_48(), Size(DPI(17), DPI(17)));

    version_label_.SetText("v" + TaskTrackVersion());
    version_label_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Subtle));
    state_label_.SetText("No task");
    state_label_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Accent));

    pause_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Subtle));
    pause_button_.SetText("Pause").SetContentInset(DPI(5));

    reminder_dropdown_.SetCustomStyle(MakeCompactDropdownStyle(reminder_dropdown_.GetStyle()));
    reminder_dropdown_.SetSizeMin(DPI(116), DPI(28));
    reminder_dropdown_.UseInternalModel().Clear()
        .Add("Reminder off", 0)
        .Add("1 minute", 1)
        .Add("15 minutes", 15)
        .Add("30 minutes", 30)
        .Add("1 hour", 60)
        .Add("2 hours", 120)
        .Add("4 hours", 240);
    reminder_dropdown_.SelectByData(60);

    paused_reminder_button_.SetCheckable(true);
    paused_reminder_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Subtle));
    paused_reminder_button_.SetText("Paused reminders").SetContentInset(DPI(5));

    agent_nudge_button_.SetCheckable(true);
    agent_nudge_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Subtle));
    agent_nudge_button_.SetText("Agent nudge").SetContentInset(DPI(5));

    exit_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Subtle));
    exit_button_.SetText("Exit").SetContentInset(DPI(5));

    header_layout_.Add(app_heading_).Fit().MinMain(DPI(210)).MinCross(DPI(34));
    header_layout_.Add(version_label_).Fit().MinMain(DPI(62));
    header_layout_.Add(state_label_).Fit().MinMain(DPI(112));
    {
        auto spacer = header_layout_.AddSpacer(1);
        spacer.Expand(1).MinMain(DPI(8));
    }
    header_layout_.Add(pause_button_).Fixed(DPI(76)).MinCross(DPI(28));
    header_layout_.Add(reminder_dropdown_).Fit().MinMain(DPI(116)).MinCross(DPI(28));
    header_layout_.Add(paused_reminder_button_).Fit().MinMain(DPI(126)).MinCross(DPI(28));
    header_layout_.Add(agent_nudge_button_).Fit().MinMain(DPI(96)).MinCross(DPI(28));
    header_layout_.Add(exit_button_).Fixed(DPI(62)).MinCross(DPI(28));

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
    objective_panel_.Add(objective_layout_.SizePos());
    objective_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(10))
        .SetInset(DPI(10))
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    objective_card_.SetCustomStyle(MakeTitleStyle(SansSerifZ(11).Bold(), SansSerifZ(9), UiRole::Standard));
    objective_card_.SetTitle("No task loaded").SetSubTitle("").SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

    objective_progress_.SetText("0 / 0");
    objective_progress_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Accent));
    objective_progress_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

    objective_layout_.Add(objective_card_).Expand(1).MinMain(DPI(360)).MinCross(DPI(48));
    objective_layout_.Add(objective_progress_).Fit().MinMain(DPI(100)).MinCross(DPI(28));
}

void TaskTrackWindow::BuildCategories()
{
    categories_panel_.Add(categories_base_.SizePos());
    categories_base_.SetDirection(UiDirection::V).SetGap(DPI(5)).SetInset(DPI(7));

    categories_heading_.SetCustomStyle(MakeTitleStyle(SansSerifZ(10).Bold(), SansSerifZ(9), UiRole::Accent));
    categories_heading_.SetTitle("Categories").SetSubTitle("Choose a verification group").SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

    categories_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    categories_scroll_.Content().Add(categories_flow_.SizePos());
    categories_flow_.SetDirection(UiDirection::H)
        .SetGap(DPI(5), DPI(5))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetFixedColumn(DPI(190));

    categories_base_.Add(categories_heading_).Fit().MinCross(DPI(38));
    categories_base_.Add(categories_scroll_).Expand(1).MinMain(DPI(48));
}

void TaskTrackWindow::BuildTaskArea()
{
    task_panel_.Add(task_base_.SizePos());
    task_base_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7));

    task_heading_.SetCustomStyle(MakeTitleStyle(SansSerifZ(10).Bold(), SansSerifZ(9), UiRole::Standard));
    task_heading_.SetTitle("Verification").SetSubTitle("Human evidence requested by the agent").SetContentInset(0).ShowTitleLine(false).ShowCardLine(false);

    task_scroll_.SetScrollMode(UIPANELSCROLL_VERTICAL);
    task_scroll_.Content().Add(task_flow_.SizePos());
    task_flow_.SetDirection(UiDirection::H)
        .SetGap(DPI(7), DPI(7))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetFixedColumn(DPI(410));

    empty_label_.SetText("No checks in this category.");
    empty_label_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Subtle));
    empty_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);
    task_scroll_.Add(empty_label_.HCenterPosZ(0, DPI(320)).VCenterPosZ(0, DPI(24)));

    task_base_.Add(task_heading_).Fit().MinCross(DPI(38));
    task_base_.Add(task_scroll_).Expand(1).MinMain(DPI(240));
}

void TaskTrackWindow::BuildFooter()
{
    footer_layout_.SetDirection(UiDirection::H)
        .SetGap(DPI(8))
        .SetInset(0)
        .SetWrap(UiBoxWrap::Flow)
        .SetWrapAutoResize(true)
        .SetAlignItems(UiCrossAlign::Center);

    progress_label_.SetText("No task loaded");
    progress_label_.SetCustomStyle(MakeCompactLabelStyle(UiRole::Subtle));

    save_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Standard));
    save_button_.SetText("Save");
    save_button_.SetContentInset(DPI(5));
    save_button_.SetSplitWidth(DPI(28)).SetPopupMinWidth(DPI(190));
    save_button_.ClearItems()
        .Add("Export Markdown", "markdown")
        .Add("Export JSON", "json")
        .Add("Save Copy...", "copy");

    complete_button_.SetCustomStyle(MakeCompactButtonStyle(UiRole::Accent));
    complete_button_.SetText("Send Result").SetContentInset(DPI(5));

    footer_layout_.Add(progress_label_).Expand(1).MinMain(DPI(320));
    footer_layout_.Add(save_button_).Fixed(DPI(92)).MinCross(DPI(30));
    footer_layout_.Add(complete_button_).Fixed(DPI(116)).MinCross(DPI(30));

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
    app_heading_.SetSubTitle(document_.project.IsEmpty() ? String("Human verification") : document_.project);
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
    UiRole state_role = UiRole::Accent;
    if(document_.state == TaskTrackState::Paused)
        state_role = UiRole::Subtle;
    else if(document_.state == TaskTrackState::Closed)
        state_role = UiRole::Alert;
    state_label_.SetCustomStyle(MakeCompactLabelStyle(state_role));

    bool terminal = document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed;
    pause_button_.Enable(!terminal);
    pause_button_.SetText(document_.state == TaskTrackState::Paused ? "Resume" : "Pause");
    complete_button_.SetText(document_.state == TaskTrackState::Completed ? "Result Sent" : "Send Result");
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
    progress_label_.SetText(Format("%d/%d checks · %d/%d required · %s",
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
        button.SetContentInset(DPI(4));
        button.WhenAction = [=] { SelectCategory(category); };
        categories_flow_.Add(button).Fit().MinMain(DPI(190)).MinCross(DPI(28));
    };

    add_button("All");
    for(const String& category : TaskTrackCategories(document_))
        add_button(category);

    rebuilding_ = false;
    categories_flow_.RefreshLayout();
    categories_scroll_.RefreshLayout();
}

void TaskTrackWindow::RebuildItems()
{
    rebuilding_ = true;
    task_flow_.ClearItems();
    item_controls_.Clear();

    int shown = 0;
    for(int i = 0; i < document_.items.GetCount(); ++i) {
        const TaskTrackItem& item = document_.items[i];
        if(selected_category_ != "All" && item.category != selected_category_)
            continue;

        TaskTrackItemCtrl& ctrl = item_controls_.Add();
        ctrl.Bind(document_, i);
        ctrl.WhenChanged = [=] { OnItemChanged(); };
        bool terminal = document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed;
        ctrl.Enable(!terminal);
        task_flow_.Add(ctrl).Fit().MinMain(DPI(390)).MinCross(DPI(150));
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
        Exclamation("Required checks are still unanswered:\n\n" + Join(missing, "\n"));
        return;
    }
    document_.state = TaskTrackState::Completed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;
    RebuildItems();
    RefreshHeaderState();
    PromptOK("TaskTrack result saved. The agent can now retrieve the completed task.");
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
                     idle_seconds >= 60;

    if(!periodic_due && !agent_due)
        return;

    last_seen_agent_poll_epoch_ = max(last_seen_agent_poll_epoch_, poll_epoch);
    int result = RunReminderPrompt(agent_due);
    ++document_.reminder_count;
    TouchHumanActivity();

    if(result == REMINDER_CONTINUE)
        document_.state = TaskTrackState::InProgress;
    else if(result == REMINDER_PAUSE)
        document_.state = TaskTrackState::Paused;
    else if(result == REMINDER_CLOSE)
        document_.state = TaskTrackState::Closed;

    SaveProgress(false);
    if(result == REMINDER_CLOSE)
        RebuildItems();
    RefreshHeaderState();
}

} // namespace Upp
