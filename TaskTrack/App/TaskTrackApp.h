#ifndef _TaskTrack_App_TaskTrackApp_h_
#define _TaskTrack_App_TaskTrackApp_h_

/*
    TaskTrack GUI
    =============

    Compact U++ human-in-the-loop verification console. The visual shell follows
    the proven UiTitleCard / UiPanel / wrapped UiBoxLayout composition used by
    the current Ui applications while keeping TaskTrack's model independent of
    the GUI.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackCore.h>

namespace Upp {

class TaskTrackItemCtrl : public UiPanel {
public:
    typedef TaskTrackItemCtrl CLASSNAME;

    TaskTrackItemCtrl();

    void Bind(TaskTrackDocument& document, int item_index);
    int  GetItemIndex() const { return item_index_; }

    Event<> WhenChanged;

private:
    void Configure();
    void SyncFromModel();
    void Commit();
    void SyncCheckButton();
    void FillStatusOptions();
    bool ParseExpectedColor(Color& color) const;

    TaskTrackDocument* document_ = nullptr;
    int item_index_ = -1;
    bool syncing_ = false;

    UiBoxLayout body_ { UiDirection::V };
    UiTitleCard heading_;
    UiLabel expected_label_;
    UiBoxLayout response_row_ { UiDirection::H };

    UiCheckBox check_button_;
    UiDropdown status_dropdown_;
    UiLineEdit value_edit_;
    UiMultiEdit multiline_edit_;
    UiCompositeColor expected_color_;
    UiLineEdit note_edit_;
};

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
    UiLabel version_label_;
    UiLabel state_label_;
    UiButton pause_button_;
    UiDropdown reminder_dropdown_;
    UiButton paused_reminder_button_;
    UiButton agent_nudge_button_;
    UiButton exit_button_;

    UiPanel objective_panel_;
    UiBoxLayout objective_layout_ { UiDirection::H };
    UiTitleCard objective_card_;
    UiLabel objective_progress_;

    UiPanel categories_panel_;
    UiBoxLayout categories_base_ { UiDirection::V };
    UiTitleCard categories_heading_;
    UiScrollPanel categories_scroll_;
    UiBoxLayout categories_flow_ { UiDirection::H };
    Array<UiButton> category_buttons_;

    UiPanel task_panel_;
    UiBoxLayout task_base_ { UiDirection::V };
    UiTitleCard task_heading_;
    UiScrollPanel task_scroll_;
    UiBoxLayout task_flow_ { UiDirection::H };
    Array<TaskTrackItemCtrl> item_controls_;
    UiLabel empty_label_;

    UiBoxLayout footer_layout_ { UiDirection::H };
    UiLabel progress_label_;
    UiSplitButton save_button_;
    UiButton complete_button_;
};

} // namespace Upp

#endif
