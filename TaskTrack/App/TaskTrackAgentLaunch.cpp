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

bool AgentItemNeedsRoom(const TaskTrackItem& item)
{
    switch(item.type) {
    case TaskTrackItemType::ListSelect:
    case TaskTrackItemType::Notes:
    case TaskTrackItemType::Range:
    case TaskTrackItemType::Color:
    case TaskTrackItemType::Gradient:
    case TaskTrackItemType::Position:
    case TaskTrackItemType::Direction:
    case TaskTrackItemType::RankOrder:
    case TaskTrackItemType::HierarchySelect:
    case TaskTrackItemType::Curve:
        return true;
    case TaskTrackItemType::MultiChoice:
    case TaskTrackItemType::SingleChoice:
        return item.choices.GetCount() > 6;
    default:
        return false;
    }
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
    const bool roomy_one_item = one_item && AgentItemNeedsRoom(document_.items[0]);

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

    exit_button_.SetText("×").Tip("Close task");
    complete_button_.SetText("Submit");
    header_layout_.ItemAt(8).Fixed(DPI(32)).MinMain(DPI(32));

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
    footer_layout_.ItemAt(3).Fixed(DPI(92)).MinMain(DPI(92));

    Size size;
    Size min_size;
    int task_min_height;
    if(one_item && !roomy_one_item) {
        size = Size(DPI(660), DPI(350));
        min_size = Size(DPI(580), DPI(320));
        task_min_height = DPI(180);
    }
    else if(one_item) {
        size = Size(DPI(720), DPI(440));
        min_size = Size(DPI(620), DPI(380));
        task_min_height = DPI(250);
    }
    else if(count <= 4) {
        size = Size(DPI(880), DPI(560));
        min_size = Size(DPI(720), DPI(460));
        task_min_height = DPI(300);
    }
    else {
        size = Size(DPI(1080), DPI(720));
        min_size = Size(DPI(760), DPI(540));
        task_min_height = DPI(300);
    }

    // BuildUi gives the general console a generous task-area minimum. Reduce
    // that only for the deliberately compact agent dialog.
    main_box_.ItemAt(2).Expand(1).MinMain(task_min_height);

    Rect work = Ctrl::GetPrimaryWorkArea();
    size.cx = min(size.cx, work.Width());
    size.cy = min(size.cy, work.Height());
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
