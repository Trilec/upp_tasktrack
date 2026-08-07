#ifndef _TaskTrack_App_TaskTrackApp_h_
#define _TaskTrack_App_TaskTrackApp_h_

/*
    TaskTrack GUI
    =============

    Compact U++ human-in-the-loop verification console. Semantic questions are
    rendered by TaskTrack/Widgets; this package owns only the application shell,
    filtering, persistence cadence, reminder lifecycle, and export actions.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackCore.h>
#include <TaskTrack/Widgets/TaskTrackWidgets.h>

namespace Upp {

class TaskTrackWindow : public TopWindow {
public:
    typedef TaskTrackWindow CLASSNAME;

    TaskTrackWindow();
    ~TaskTrackWindow() override;

    bool LoadTask(const String& path, String& error);

private:
    enum ReminderResult {
        REMINDER_CONTINUE = 1,
        REMINDER_PAUSE,
        REMINDER_CLOSE,
    };

    void BuildUi();
    void BuildHeader();
    void BuildObjective();
    void BuildCategories();
    void BuildTaskArea();
    void BuildFooter();

    void RefreshAll();
    void RebuildCategories();
    void RebuildItems();
    void RefreshHeaderState();
    void RefreshProgress();
    void SelectCategory(const String& category);

    bool SaveProgress(bool touch_human = true);
    void ScheduleAutosave();
    void OnItemChanged();
    void TogglePause();
    void CompleteTask();
    void CloseTask();
    void ExportMarkdown();
    void ExportJson();
    void SaveCopy();

    void ArmReminderTimer();
    void CheckReminder();
    int  RunReminderPrompt(bool agent_poll_triggered);
    void TouchHumanActivity();

    UiTitleCard::Style MakeTitleStyle(Font title_font, Font subtitle_font, UiRole role = UiRole::Standard) const;
    UiButton::Style MakeCategoryButtonStyle(bool selected) const;
    UiGroupPanel::Style MakeCategoryGroupStyle() const;

private:
    String task_path_;
    TaskTrackDocument document_;
    String selected_category_ = "All";
    bool loaded_ = false;
    bool rebuilding_ = false;
    int64 last_seen_agent_poll_epoch_ = 0;

    UiBoxLayout main_box_ { UiDirection::V };

    UiBoxLayout header_layout_ { UiDirection::H };
    UiTitleCard app_heading_;
    UiLabel state_label_;
    UiButton pause_button_;
    UiDropdown reminder_dropdown_;
    UiButton paused_reminder_button_;
    UiButton agent_nudge_button_;
    UiButton exit_button_;

    UiBoxLayout objective_layout_ { UiDirection::H };
    UiTitleCard objective_card_;
    UiLabel objective_progress_;

    UiGroupPanel categories_group_;
    UiBoxLayout::ItemRef categories_item_;
    UiBoxLayout categories_flow_ { UiDirection::H };
    Array<UiButton> category_buttons_;

    UiScrollPanel task_scroll_;
    UiBoxLayout task_flow_ { UiDirection::H };
    Array<TaskTrackQuestionCtrl> question_controls_;
    UiLabel empty_label_;

    UiBoxLayout footer_layout_ { UiDirection::H };
    UiLabel progress_label_;
    UiSplitButton save_button_;
    UiButton complete_button_;
};

} // namespace Upp

#endif
