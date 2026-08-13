#include "TaskTrackApp.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

namespace Upp {

namespace {

static const int TASKTRACK_AGENT_REMINDER_TIMER_ID = 1;
static const int TASKTRACK_AGENT_FOREGROUND_TIMER_ID = 3;

UiLabel::Style AgentProgressStyle()
{
    UiLabel::Style style = UiTheme::ResolveLabel(UiTheme::GetContext(), UiRole::Accent, UiTextSize::Body);
    style.font = SansSerifZ(9).Bold();
    return style;
}

} // namespace

void TaskTrackWindow::PrepareAgentLaunch()
{
    if(!loaded_)
        return;

    agent_launch_mode_ = true;
    ApplyAgentCompactLayout();
    ArmAgentReminderGrace();

    // In an MCP-launched task, closing the human dialog is terminal: if all
    // required evidence is already present it is submitted, otherwise the task
    // is explicitly closed with no fabricated human answer.
    WhenClose = [=] { CloseAgentTaskAndExit(); };
    exit_button_.WhenAction = [=] { CloseAgentTaskAndExit(); };
    complete_button_.WhenAction = [=] { CompleteAgentTaskAndClose(); };

    BringAgentWindowForward();
}

void TaskTrackWindow::ApplyAgentCompactLayout()
{
    const int count = document_.items.GetCount();

    // The footer is the single progress authority in the compact dialog.
    state_label_.Hide();
    objective_progress_.Hide();
    header_layout_.ItemAt(2).Fixed(0).MinMain(0).MinCross(0);
    header_layout_.ItemAt(3).Fixed(0).MinMain(0).MinCross(0);

    UiLabel::Style progress_style = AgentProgressStyle();
    progress_label_.SetCustomStyle(progress_style);
    progress_label_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    // Preserve the controls, but keep secondary lifecycle options subordinate.
    paused_reminder_button_.SetText("Paused");
    agent_nudge_button_.SetText("Nudge");
    exit_button_.SetText("×").Tip("Close task");
    complete_button_.SetText("Submit");

    header_layout_.ItemAt(4).Fixed(DPI(58)).MinMain(DPI(58));
    header_layout_.ItemAt(5).Fixed(DPI(100)).MinMain(DPI(100));
    header_layout_.ItemAt(6).Fixed(DPI(66)).MinMain(DPI(66));
    header_layout_.ItemAt(7).Fixed(DPI(62)).MinMain(DPI(62));
    header_layout_.ItemAt(8).Fixed(DPI(32)).MinMain(DPI(32));

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
    if(count <= 1) {
        size = Size(DPI(760), DPI(430));
        min_size = Size(DPI(640), DPI(360));
    }
    else if(count <= 4) {
        size = Size(DPI(920), DPI(570));
        min_size = Size(DPI(720), DPI(460));
    }
    else {
        size = Size(DPI(1180), DPI(780));
        min_size = Size(DPI(760), DPI(540));
    }

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
    SetTimeCallback(120, [=] {
        if(closing_ || !IsOpen())
            return;
        SetForeground();
#ifdef PLATFORM_WIN32
        if(HWND hwnd = GetHWND())
            ::SetForegroundWindow(hwnd);
#endif
        Urgent(false);
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

    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    closing_ = true;
    Close();
}

void TaskTrackWindow::CloseAgentTaskAndExit()
{
    if(closing_)
        return;

    if(!loaded_) {
        closing_ = true;
        Close();
        return;
    }

    if(document_.state == TaskTrackState::Completed || document_.state == TaskTrackState::Closed) {
        SaveProgress(false);
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

    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    closing_ = true;
    Close();
}

} // namespace Upp
