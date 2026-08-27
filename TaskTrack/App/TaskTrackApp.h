#ifndef _TaskTrack_App_TaskTrackApp_h_
#define _TaskTrack_App_TaskTrackApp_h_

/*
    TaskTrack GUI
    =============

    Compact U++ human-in-the-loop verification console. Semantic questions are
    rendered by TaskTrack/Widgets; this package owns only the application shell,
    filtering, persistence cadence, reminder lifecycle, and export actions.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the GNU General Public License, version 3. See LICENSE.
*/

#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackCore.h>
#include <TaskTrack/Widgets/TaskTrackWidgets.h>
#include "TaskTrackResponsiveFlow.h"

namespace Upp {

class TaskTrackWindow : public TopWindow {
public:
    typedef TaskTrackWindow CLASSNAME;

    TaskTrackWindow();
    ~TaskTrackWindow() override;

    bool LoadTask(const String& path, String& error);

    // MCP-launched tasks should behave like a focused human decision dialog:
    // compact for small requests, foregrounded, no immediate reminder, and
    // terminal actions close the window after durable state is saved.
    void PrepareAgentLaunch();

private:
    enum ReminderResult {
        REMINDER_CONTINUE = 1,
        REMINDER_PAUSE,
        REMINDER_CLOSE,
        REMINDER_ACCEPT,
    };

    enum ExitResult {
        EXIT_KEEP_WORKING = 0,
        EXIT_ACCEPT,
        EXIT_LEAVE,
    };

    void BuildUi();
    void BuildHeader();
    void BuildCategories();
    void BuildTaskArea();
    void BuildFooter();

    void RefreshAll();
    void RebuildCategories();
    void RebuildItems();
    void RefreshHeaderState();
    void RefreshProgress();
    void RefreshQuestionVisualStates();
    void SelectCategory(const String& category);

    int CountMissingRequired(const String& category = String()) const;
    String FirstMissingRequiredCategory() const;
    void ActivateRequiredReview();

    bool SaveProgress(bool touch_human = true);
    void ScheduleAutosave();
    void OnItemChanged();
    void TogglePause();
    void AcceptRecommendations();
    void CompleteTask();
    void CloseTask();
    void ExportMarkdown();
    void ExportJson();
    void SaveCopy();

    void ArmReminderTimer();
    void CheckReminder();
    int  RunReminderPrompt(bool agent_poll_triggered);
    void TouchHumanActivity();
    bool ApplyRecommendation(TaskTrackItem& item);
    int  TaskTrackExitPrompt();
    void RequestExit();

    // Agent-launch lifecycle helpers. Durable JSON remains internal storage;
    // completion/closure is exposed to the agent only through MCP task state.
    void CompleteAgentTaskAndClose();
    void CloseAgentTaskAndExit();
    void CloseAgentTaskAfterDelegation();
    void ArmAgentReminderGrace();
    void BringAgentWindowForward();
    void ApplyAgentCompactLayout();

    UiTitleCard::Style MakeTitleStyle(Font title_font, Font subtitle_font, UiRole role = UiRole::Standard) const;
    UiButton::Style MakeCategoryButtonStyle(bool selected, bool needs_attention = false) const;
    UiGroupPanel::Style MakeCategoryGroupStyle() const;

private:
    String task_path_;
    TaskTrackDocument document_;
    String selected_category_ = "All";
    bool loaded_ = false;
    bool rebuilding_ = false;
    bool reminder_showing_ = false;
    bool closing_ = false;
    bool review_required_ = false;
    bool agent_launch_mode_ = false;
    int64 last_seen_agent_poll_epoch_ = 0;

    UiBoxLayout main_box_ { UiDirection::V };

    UiBoxLayout header_layout_ { UiDirection::H };
    UiTitleCard app_heading_;
    UiTitleCard objective_card_;
    UiLabel state_label_;
    UiLabel objective_progress_;
    UiButton pause_button_;
    UiDropdown reminder_dropdown_;
    UiButton paused_reminder_button_;
    UiButton agent_nudge_button_;
    UiButton exit_button_;

    TaskTrackCategoryPanel categories_group_;
    UiBoxLayout::ItemRef categories_item_;
    TaskTrackCategoryFlow categories_flow_ { UiDirection::H };
    Array<UiButton> category_buttons_;

    UiScrollPanel task_scroll_;
    TaskTrackQuestionFlow task_flow_ { UiDirection::H };
    Array<TaskTrackQuestionCtrl> question_controls_;
    UiLabel empty_label_;

    UiBoxLayout footer_layout_ { UiDirection::H };
    UiLabel progress_label_;
    UiSplitButton save_button_;
    UiButton accept_recommendations_button_;
    UiButton complete_button_;
};

} // namespace Upp

#endif
