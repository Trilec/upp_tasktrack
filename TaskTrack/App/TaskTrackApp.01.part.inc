#include "TaskTrackApp.h"

namespace Upp {

namespace {

static const int TASKTRACK_REMINDER_TIMER_ID = 7011;
static const int TASKTRACK_AUTOSAVE_TIMER_ID = 7012;

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

class TaskTrackReminderDialog : public TopWindow {
public:
    TaskTrackReminderDialog(bool paused, const String& task_title, bool agent_poll)
    {
        Title("TaskTrack Reminder");
        SetRect(0, 0, DPI(500), DPI(196));
        SetMinSize(Size(DPI(440), DPI(180)));

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

        buttons_.SetDirection(UiDirection::H).SetGap(DPI(7)).SetInset(0).SetAlignItems(UiCrossAlign::Center);

        continue_.SetCustomStyle(CompactButtonStyle(UiRole::Accent));
        continue_.SetText(paused ? "Resume" : "Continue").SetContentInset(DPI(4));
        pause_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
        pause_.SetText(paused ? "Keep paused" : "Pause").SetContentInset(DPI(4));
        close_.SetCustomStyle(CompactButtonStyle(UiRole::Alert));
        close_.SetText("Close task").SetContentInset(DPI(4));

        box_.Add(heading_).Fit().MinCross(DPI(46)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.Add(detail_).Fit().MinCross(DPI(22)).AlignSelf(UiBoxLayout::Align::Stretch);
        box_.AddSpacer(1).Expand(1).MinMain(DPI(6));
        box_.Add(buttons_).Fit().MinCross(DPI(32)).AlignSelf(UiBoxLayout::Align::Stretch);
        buttons_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        buttons_.Add(continue_).Fixed(DPI(94)).MinCross(DPI(28));
        buttons_.Add(pause_).Fixed(DPI(104)).MinCross(DPI(28));
        buttons_.Add(close_).Fixed(DPI(96)).MinCross(DPI(28));

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

TaskTrackWindow::TaskTrackWindow()
{
    Title("TaskTrack");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(1180), DPI(780));
    SetMinSize(Size(DPI(760), DPI(540)));
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

UiButton::Style TaskTrackWindow::MakeCategoryButtonStyle(bool selected) const
{
    UiButton::Style style = UiTheme::ResolveButton(selected ? UiRole::Accent : UiRole::Subtle);
    style.font = SansSerifZ(9);
    style.metrics.use_text_font = false;
    style.metrics.radius = DPI(5);
    style.metrics.frame_width = DPI(1);
    return style;
}

UiGroupPanel::Style TaskTrackWindow::MakeCategoryGroupStyle() const
{
    UiGroupPanel::Style style = UiTheme::ResolveGroupPanel(UiRole::Subtle);
    style.title_font = SansSerifZ(9).Bold();
    style.subtitle_font = SansSerifZ(8);
    style.metrics.radius = DPI(6);
    style.metrics.frame_width = DPI(1);
    style.inset = Rect(DPI(6), DPI(5), DPI(6), DPI(6));
    style.header_inset = Rect(DPI(6), DPI(3), DPI(6), DPI(2));
    style.header_gap = DPI(1);
    style.title_subtitle_gap = 0;
    style.header_band_enabled = false;
    style.line_enabled = false;
    return style;
}

void TaskTrackWindow::BuildUi()
{
    Add(main_box_.SizePos());
    main_box_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(7));

    BuildHeader();
    BuildObjective();
    BuildCategories();
    BuildTaskArea();
    BuildFooter();

    main_box_.Add(header_layout_).Fit().MinCross(DPI(34)).AlignSelf(UiBoxLayout::Align::Stretch);
    main_box_.Add(objective_layout_).Fit().MinCross(DPI(48)).AlignSelf(UiBoxLayout::Align::Stretch);
    categories_item_ = main_box_.Add(categories_group_);
    categories_item_.Fit().MinMain(DPI(50)).AlignSelf(UiBoxLayout::Align::Stretch);
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

    app_heading_.SetCustomStyle(MakeTitleStyle(SansSerifZ(11).Bold(), SansSerifZ(8), UiRole::Accent));
    app_heading_.SetTitle("TaskTrack")
        .SetSubTitle("Human input")
        .SetContentInset(0)
        .SetMediaReserve(DPI(22))
        .SetMediaMin(DPI(16))
        .SetMediaGap(DPI(5))
        .SetMediaAutoFit(false)
        .SetMediaSide(UiAlign::LEFT);
    app_heading_.SetMedia(ICON_DESIGN_CHECK_SMALL_48(), Size(DPI(16), DPI(16)));

    state_label_.SetCustomStyle(CompactLabelStyle(UiRole::Accent));
    state_label_.SetText("No task");

    pause_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    pause_button_.SetText("Pause").SetContentInset(DPI(4));

    reminder_dropdown_.SetCustomStyle(CompactDropdownStyle());
    reminder_dropdown_.SetSizeMin(DPI(108), DPI(27));
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
    paused_reminder_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    paused_reminder_button_.SetText("Paused remind").SetContentInset(DPI(4));

    agent_nudge_button_.SetCheckable(true);
    agent_nudge_button_.SetCustomStyle(CompactButtonStyle(UiRole::Subtle));
    agent_nudge_button_.SetText("Agent nudge").SetContentInset(DPI(4));

