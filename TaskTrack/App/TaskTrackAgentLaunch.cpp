#include "TaskTrackApp.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

namespace Upp {

namespace {

static const int TASKTRACK_AGENT_REMINDER_TIMER_ID = 1;
static const int TASKTRACK_AGENT_FOREGROUND_TIMER_ID = 3;
static const int TASKTRACK_AGENT_COMPLETE_TIMER_ID = 4;

UiLabel::Style AgentProgressStyle()
{
    UiLabel::Style style = UiTheme::ResolveLabel(UiTheme::GetContext(), UiRole::Accent, UiTextSize::Body);
    style.font = SansSerifZ(9).Bold();
    return style;
}

int AgentItemEstimatedHeight(const TaskTrackItem& item)
{
    int h = DPI(100);
    switch(item.type) {
    case TaskTrackItemType::Confirm:
    case TaskTrackItemType::Text:
    case TaskTrackItemType::Select:
    case TaskTrackItemType::Number:
    case TaskTrackItemType::Amount:
    case TaskTrackItemType::Rating:
        h = DPI(100);
        break;
    case TaskTrackItemType::SingleChoice:
    case TaskTrackItemType::MultiChoice: {
        int rows = max(1, (item.choices.GetCount() + 3) / 4);
        h = DPI(88 + min(rows, 5) * 22);
        break;
    }
    case TaskTrackItemType::Notes:
        h = DPI(132);
        break;
    case TaskTrackItemType::Range:
        h = DPI(135);
        break;
    case TaskTrackItemType::Color:
    case TaskTrackItemType::Gradient:
    case TaskTrackItemType::Position:
    case TaskTrackItemType::Direction:
        h = DPI(155);
        break;
    case TaskTrackItemType::ListSelect:
    case TaskTrackItemType::RankOrder:
    case TaskTrackItemType::HierarchySelect:
        h = DPI(190);
        break;
    case TaskTrackItemType::Curve:
        h = DPI(210);
        break;
    }

    if(item.instruction.GetCount() > 120)
        h += DPI(18);
    return h;
}

int AgentItemEstimatedWidth(const TaskTrackItem& item)
{
    int w = DPI(620);
    if(item.title.GetCount() > 72 || item.instruction.GetCount() > 100)
        w = DPI(680);

    switch(item.type) {
    case TaskTrackItemType::Notes:
    case TaskTrackItemType::ListSelect:
    case TaskTrackItemType::Range:
    case TaskTrackItemType::Color:
    case TaskTrackItemType::Gradient:
    case TaskTrackItemType::Position:
    case TaskTrackItemType::Direction:
        w = max(w, DPI(660));
        break;
    case TaskTrackItemType::RankOrder:
    case TaskTrackItemType::HierarchySelect:
    case TaskTrackItemType::Curve:
        w = max(w, DPI(720));
        break;
    case TaskTrackItemType::SingleChoice:
    case TaskTrackItemType::MultiChoice:
        if(item.choices.GetCount() > 8)
            w = max(w, DPI(700));
        break;
    default:
        break;
    }
    return w;
}

int AgentPackedItemHeight(const TaskTrackDocument& doc, int columns)
{
    columns = max(1, columns);
    int total = 0;
    for(int i = 0; i < doc.items.GetCount(); i += columns) {
        int row_height = 0;
        for(int c = 0; c < columns && i + c < doc.items.GetCount(); ++c)
            row_height = max(row_height, AgentItemEstimatedHeight(doc.items[i + c]));
        if(total > 0)
            total += DPI(10); // matches the task-flow vertical gutter
        total += row_height;
    }
    return total;
}

void EstimateAgentDialog(const TaskTrackDocument& doc, Size& size, Size& min_size,
                         int& task_min_height)
{
    int target_width = DPI(620);
    for(const TaskTrackItem& item : doc.items)
        target_width = max(target_width, AgentItemEstimatedWidth(item));

    const int count = doc.items.GetCount();
    if(count >= 2)
        target_width = max(target_width, DPI(700));
    if(count >= 3)
        target_width = max(target_width, DPI(760));
    if(count >= 6)
        target_width = max(target_width, DPI(900));
    target_width = max(DPI(620), min(DPI(1080), target_width));

    // TaskTrackQuestionFlow uses 350px columns with a 10px gutter. Estimate
    // the same packed rows rather than summing every question vertically.
    // One-question dialogs also keep a narrower width and a much smaller task
    // area instead of inheriting workspace-sized empty space.
    int columns = count > 1 && target_width >= DPI(700) ? 2 : 1;
    int item_height = AgentPackedItemHeight(doc, columns);

    bool category_strip = TaskTrackCategories(doc).GetCount() > 1;
    int chrome = DPI(category_strip ? 170 : (count <= 1 ? 105 : 110));
    int target_height = chrome + item_height;
    int min_height = count <= 1 ? DPI(260) : DPI(310);
    target_height = max(min_height, min(DPI(640), target_height));

    size = Size(target_width, target_height);
    min_size = Size(min(target_width, DPI(560)), min(target_height, DPI(250)));
    task_min_height = max(DPI(100), min(DPI(350), item_height));
}

} // namespace

void TaskTrackWindow::PrepareAgentLaunch()
{
    if(!loaded_)
        return;

    agent_launch_mode_ = true;
    ApplyAgentCompactLayout();
    ArmAgentReminderGrace();

    // An MCP-launched window is a decision dialog, not a second application
    // workspace. Terminal human actions persist first and then close it.
    WhenClose = [=] { CloseAgentTaskAndExit(); };
    exit_button_.WhenAction = [=] { CloseAgentTaskAndExit(); };
    complete_button_.WhenAction = [=] { CompleteAgentTaskAndClose(); };

    // Accepting the only required recommendation is itself an explicit human
    // finalization action. Do not make the human press Submit as a second step.
    KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
    Ptr<TaskTrackWindow> self = this;
    SetTimeCallback(-150, [self] {
        if(!self || self->closing_ || !self->loaded_ || self->document_.items.GetCount() != 1)
            return;
        const TaskTrackItem& item = self->document_.items[0];
        if(item.answer.answered && item.answer.status == "accepted" && TaskTrackCanComplete(self->document_)) {
            self->KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
            self->CompleteAgentTaskAndClose();
        }
    }, TASKTRACK_AGENT_COMPLETE_TIMER_ID);

    BringAgentWindowForward();
}

void TaskTrackWindow::ApplyAgentCompactLayout()
{
    const int count = document_.items.GetCount();
    const bool one_item = count == 1;

    // The footer is the single progress authority in the compact dialog.
    state_label_.Hide();
    objective_progress_.Hide();
    header_layout_.ItemAt(2).Fixed(0).MinMain(0).MinCross(0);
    header_layout_.ItemAt(3).Fixed(0).MinMain(0).MinCross(0);

    UiLabel::Style progress_style = AgentProgressStyle();
    progress_label_.SetCustomStyle(progress_style);
    progress_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    // Keep the short-lived agent dialog focused on the decision itself. The
    // richer pause/nudge controls remain available in the standalone console.
    paused_reminder_button_.Hide();
    agent_nudge_button_.Hide();
    header_layout_.ItemAt(6).Fixed(0).MinMain(0).MinCross(0);
    header_layout_.ItemAt(7).Fixed(0).MinMain(0).MinCross(0);

    complete_button_.SetText("Submit");

    if(one_item) {
        // A single decision should look and behave like a dialog, not a full
        // workspace. Reminder behaviour still runs in the background if the
        // task genuinely remains open for its configured interval.
        pause_button_.Hide();
        reminder_dropdown_.Hide();
        header_layout_.ItemAt(4).Fixed(0).MinMain(0).MinCross(0);
        header_layout_.ItemAt(5).Fixed(0).MinMain(0).MinCross(0);
    }
    else {
        pause_button_.SetText("Pause");
        header_layout_.ItemAt(4).Fixed(DPI(58)).MinMain(DPI(58));
        header_layout_.ItemAt(5).Fixed(DPI(100)).MinMain(DPI(100));
    }

    // The small header X was ambiguous beside Pause/reminder controls. Reuse
    // the same semantic exit action in the footer with an explicit label.
    int header_exit = header_layout_.FindItem(exit_button_);
    if(header_exit >= 0)
        header_layout_.RemoveItem(header_exit);
    int footer_exit = footer_layout_.FindItem(exit_button_);
    if(footer_exit >= 0)
        footer_layout_.RemoveItem(footer_exit);
    int footer_complete = footer_layout_.FindItem(complete_button_);
    if(footer_complete >= 0)
        footer_layout_.RemoveItem(footer_complete);
    exit_button_.SetText("Cancel task").Tip("Close this task without completing it");
    footer_layout_.Add(exit_button_).Fixed(DPI(96)).MinCross(DPI(28));
    footer_layout_.Add(complete_button_).Fixed(DPI(92)).MinCross(DPI(28));

    // Export/save is useful in the standalone console, but it is noise in the
    // ordinary agent dialog. TaskTrack still autosaves every human edit.
    save_button_.Hide();
    footer_layout_.ItemAt(1).Fixed(0).MinMain(0).MinCross(0);

    bool has_recommendation = false;
    for(const TaskTrackItem& item : document_.items)
        has_recommendation = has_recommendation || !item.recommended.IsEmpty();
    if(!has_recommendation) {
        accept_recommendations_button_.Hide();
        footer_layout_.ItemAt(2).Fixed(0).MinMain(0).MinCross(0);
    }
    else {
        footer_layout_.ItemAt(2).Fixed(DPI(132)).MinMain(DPI(132));
    }

    Size size;
    Size min_size;
    int task_min_height = DPI(120);
    EstimateAgentDialog(document_, size, min_size, task_min_height);

    // BuildUi gives the general console a generous task-area minimum. Agent
    // dialogs instead reserve the packed semantic-model estimate and let
    // scrolling take over once further growth would make the window excessive.
    main_box_.ItemAt(2).Expand(1).MinMain(task_min_height);

    Rect work = Ctrl::GetPrimaryWorkArea();
    int max_w = max(DPI(520), work.Width() - DPI(24));
    int max_h = max(DPI(260), work.Height() - DPI(24));
    size.cx = min(size.cx, max_w);
    size.cy = min(size.cy, max_h);
    SetMinSize(Size(min(min_size.cx, size.cx), min(min_size.cy, size.cy)));
    SetRect(work.CenterRect(size));

    header_layout_.RefreshLayout();
    footer_layout_.RefreshLayout();
    main_box_.RefreshLayout();
}

void TaskTrackWindow::ArmAgentReminderGrace()
{
    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    if(!loaded_ || document_.reminder_minutes <= 0)
        return;

    // Start the first reminder interval when the dialog is actually presented,
    // rather than inheriting time spent creating/launching the task.
    TouchHumanActivity();
    SaveProgress(false);
    int delay_ms = max(1000, document_.reminder_minutes * 60 * 1000);
    SetTimeCallback(delay_ms, [=] {
        if(closing_ || !loaded_)
            return;
        ArmReminderTimer();
        CheckReminder();
    }, TASKTRACK_AGENT_REMINDER_TIMER_ID);
}

void TaskTrackWindow::BringAgentWindowForward()
{
    if(!IsOpen())
        Open();
    Show();
    SetForeground();
    SetFocus();

#ifdef PLATFORM_WIN32
    if(HWND hwnd = GetHWND()) {
        ::ShowWindow(hwnd, SW_RESTORE);
        ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ::SetForegroundWindow(hwnd);
    }
#endif

    Urgent(true);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    Ptr<TaskTrackWindow> self = this;
    SetTimeCallback(120, [self] {
        if(!self || self->closing_ || !self->IsOpen())
            return;
        self->SetForeground();
#ifdef PLATFORM_WIN32
        if(HWND hwnd = self->GetHWND())
            ::SetForegroundWindow(hwnd);
#endif
        self->Urgent(false);
    }, TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
}

void TaskTrackWindow::CompleteAgentTaskAndClose()
{
    if(!loaded_ || closing_)
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

    // Be explicit about what succeeded. At this point the durable answer is
    // complete; the still-active MCP create_task call will return it directly.
    progress_label_.SetText("Answer saved — returning to agent…");
    progress_label_.Refresh();

    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);

    // Keep the acknowledgement visible briefly without requiring another click.
    Ptr<TaskTrackWindow> self = this;
    SetTimeCallback(160, [self] {
        if(!self || self->closing_)
            return;
        self->closing_ = true;
        self->Close();
    }, TASKTRACK_AGENT_COMPLETE_TIMER_ID);
}

void TaskTrackWindow::CloseAgentTaskAndExit()
{
    if(closing_)
        return;

    if(!loaded_) {
        KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
        closing_ = true;
        Close();
        return;
    }

    if(document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed) {
        SaveProgress(false);
        KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
        KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
        KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
        closing_ = true;
        Close();
        return;
    }

    // If the user already supplied all required evidence, closing the dialog is
    // equivalent to Submit. Otherwise it is an explicit cancellation and must
    // not manufacture an answer from defaults/recommendations.
    if(TaskTrackCanComplete(document_)) {
        CompleteAgentTaskAndClose();
        return;
    }

    document_.state = TaskTrackState::Closed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;

    progress_label_.SetText("Task closed — returning no human evidence…");
    progress_label_.Refresh();
    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
    closing_ = true;
    Close();
}

} // namespace Upp
