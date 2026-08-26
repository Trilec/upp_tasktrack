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

bool AgentHasAnsweredDelegation(const TaskTrackAgentChannel& channel, const String& item_id)
{
    for(int i = channel.requests.GetCount() - 1; i >= 0; --i) {
        const TaskTrackAgentRequest& request = channel.requests[i];
        if(request.item_id == item_id && request.action == "continue_with_judgement" &&
           request.status == "answered")
            return true;
    }
    return false;
}

bool AgentDelegationResolvesHumanWork(const TaskTrackDocument& doc,
                                      const TaskTrackAgentChannel& channel)
{
    bool has_delegation = false;
    for(const TaskTrackItem& item : doc.items) {
        if(!item.required || item.answer.answered)
            continue;
        if(!AgentHasAnsweredDelegation(channel, item.id))
            return false;
        has_delegation = true;
    }
    return has_delegation;
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

    // This watcher handles the two explicit one-step finalization paths:
    // accepting the only required recommendation, and acknowledged delegation
    // of every still-unanswered required item. Delegation ends the human task
    // without manufacturing answer.data.
    KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
    Ptr<TaskTrackWindow> self = this;
    SetTimeCallback(-150, [self] {
        if(!self || self->closing_ || !self->loaded_)
            return;

        TaskTrackAgentChannel channel;
        String channel_error;
        if(TaskTrackLoadAgentChannel(self->task_path_, self->document_.task_id,
                                     channel, channel_error) &&
           AgentDelegationResolvesHumanWork(self->document_, channel)) {
            self->KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);
            self->CloseAgentTaskAfterDelegation();
            return;
        }

        if(self->document_.items.GetCount() != 1)
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
        // A single decision does not need pause/reminder chrome. This is a
        // composition choice only; dialog sizing below follows the same measured
        // geometry path for every question count.
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

    // Agent dialogs use the same card geometry regardless of item count. The
    // flow has one canonical 350px card width and at most two compact columns;
    // every card height is measured from its fully assembled controls. There
    // are no per-question-type or one-vs-many size guesses in this path.
    task_flow_.SetMaxColumns(2);

    Rect work = Ctrl::GetPrimaryWorkArea();
    const int outer_margin = DPI(24);
    const int shell_inset = DPI(7); // BuildUi main_box_ inset
    const int section_gap = DPI(6); // BuildUi main_box_ gap
    int max_w = max(DPI(360), work.Width() - outer_margin);
    int max_h = max(DPI(240), work.Height() - outer_margin);

    // Measure UiScrollPanel's actual vertical gutter instead of carrying a
    // guessed scrollbar allowance in TaskTrack. The scroll mode always owns
    // the vertical bar; its style decides the exact viewport loss.
    Rect old_scroll = task_scroll_.GetRect();
    int scroll_probe_w = min(max_w, DPI(800));
    task_scroll_.SetRect(0, 0, scroll_probe_w, DPI(160));
    task_scroll_.Layout();
    int scroll_gutter = max(0, scroll_probe_w - task_scroll_.GetViewportRect().GetWidth());
    task_scroll_.SetRect(old_scroll);

    // Width is the maximum natural requirement of the already-assembled shell
    // and the canonical question grid. This prevents a single card from forcing
    // unrelated header/footer wrapping, while spare shell width never stretches
    // the card beyond its canonical width.
    int flow_max_w = max(1, max_w - shell_inset * 2 - scroll_gutter);
    int task_preferred_w = task_flow_.PreferredWidthForItems(flow_max_w) + scroll_gutter;
    int shell_preferred_w = max(header_layout_.GetPreferredSize().cx,
                                footer_layout_.GetPreferredSize().cx);
    int target_w = min(max_w, max(task_preferred_w, shell_preferred_w) + shell_inset * 2);
    int inner_w = max(1, target_w - shell_inset * 2);

    // Probe the final scroll-panel width once so card measurement uses the
    // exact viewport width that the live panel will expose at this shell size.
    old_scroll = task_scroll_.GetRect();
    task_scroll_.SetRect(0, 0, inner_w, DPI(160));
    task_scroll_.Layout();
    int task_view_w = max(1, task_scroll_.GetViewportRect().GetWidth());
    task_scroll_.SetRect(old_scroll);

    // Header/footer are width-dependent layouts too, so measure their actual
    // wrapped heights at the selected shell width instead of adding fixed chrome.
    int header_h = header_layout_.MeasureHeightForWidth(inner_w);
    int footer_h = footer_layout_.MeasureHeightForWidth(inner_w);
    int category_h = 0;
    if(categories_group_.IsShown()) {
        Rect old_category = categories_group_.GetRect();
        categories_group_.SetRect(0, 0, inner_w, DPI(1024));
        category_h = categories_group_.GetMinSize().cy;
        categories_group_.SetRect(old_category);
    }
    int task_h = task_flow_.DesiredHeightForWidth(task_view_w);

    // main_box_ always owns four slots (header, categories, task, footer).
    // Hidden categories are a zero-height slot, while the configured section
    // gaps remain deterministic in UiBoxLayout.
    int non_task_h = shell_inset * 2 + header_h + category_h + footer_h + section_gap * 3;
    int desired_h = non_task_h + task_h;
    int target_h = min(max_h, desired_h);
    int task_slot_h = max(1, target_h - non_task_h);

    // When measured content exceeds the work area, only the task viewport is
    // shortened; UiScrollPanel is the explicit overflow path. Otherwise the
    // task slot is exactly the packed-card height reported by the same flow
    // that performs the live layout.
    main_box_.ItemAt(2).Expand(1).MinMain(min(task_h, task_slot_h));

    Size size(target_w, target_h);
    SetMinSize(size);
    SetRect(work.CenterRect(size));

    header_layout_.RefreshLayout();
    footer_layout_.RefreshLayout();
    task_flow_.RefreshLayout();
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

void TaskTrackWindow::CloseAgentTaskAfterDelegation()
{
    if(!loaded_ || closing_ || document_.state == TaskTrackState::Completed ||
       document_.state == TaskTrackState::Closed)
        return;

    // Delegation is a human authorization, but not a human answer. Close the
    // human-facing task only after the agent has acknowledged every still-
    // required delegated item. The sidecar preserves the delegation evidence;
    // answer.data remains untouched.
    document_.state = TaskTrackState::Closed;
    TouchHumanActivity();
    if(!SaveProgress(false))
        return;

    progress_label_.SetText("Judgement delegated — returning to agent…");
    progress_label_.Refresh();
    KillTimeCallback(TASKTRACK_AGENT_REMINDER_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_FOREGROUND_TIMER_ID);
    KillTimeCallback(TASKTRACK_AGENT_COMPLETE_TIMER_ID);

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