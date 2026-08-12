#include "TaskTrackWidgets.h"

namespace Upp {

namespace {

static const int TASKTRACK_AGENT_POLL_TIMER_ID = 3;

Color DisabledWorkflowColor(Color c)
{
    return Blend(c, SColorDisabled(), 105);
}

} // namespace

TaskTrackQuestionCtrl::~TaskTrackQuestionCtrl()
{
    destroying_ = true;
    KillTimeCallback(TASKTRACK_AGENT_POLL_TIMER_ID);
}

Color TaskTrackQuestionCtrl::SuggestedGrey()
{
    return Blend(SColorShadow(), SColorText(), 105);
}

Color TaskTrackQuestionCtrl::PendingOrange()
{
    return Color(205, 126, 28);
}

Color TaskTrackQuestionCtrl::AnsweredGreen()
{
    return Color(45, 142, 77);
}

Color TaskTrackQuestionCtrl::EscalatedRed()
{
    return Color(190, 48, 48);
}

void TaskTrackQuestionCtrl::SetNeedsAttention(bool on)
{
    if(needs_attention_ == on)
        return;
    needs_attention_ = on;
    RefreshVisualState();
}

void TaskTrackQuestionCtrl::RefreshVisualState()
{
    if(destroying_)
        return;
    applied_visual_state_ = -1;
    recommendation_header_state_ = -1;
    recommendation_styles_applied_ = false;
    RefreshLayout();
    Refresh();
}

void TaskTrackQuestionCtrl::Layout()
{
    if(destroying_)
        return;

    if(!workspace_typography_applied_) {
        workspace_typography_applied_ = true;
        UiGroupPanel::Style style = GetStyle();
        style.title_font = SansSerifZ(12).Bold();
        style.subtitle_font = SansSerifZ(9);
        style.title_subtitle_gap = DPI(2);
        SetCustomStyle(style);
    }

    CheckAgentChannel(false);
    ApplyVisualStatePresentation();
    ApplyRecommendationPresentation();
    UiGroupPanel::Layout();
}

UiLabel::Style TaskTrackQuestionCtrl::MakeStatusLabelStyle(VisualState state) const
{
    UiLabel::Style style = UiTheme::ResolveLabel(UiTheme::GetContext(), UiRole::Standard, UiTextSize::Body);
    style.font = SansSerifZ(8);
    Color ink = SuggestedGrey();
    if(state == VISUAL_REQUIRED_PENDING)
        ink = PendingOrange();
    else if(state == VISUAL_ANSWERED)
        ink = AnsweredGreen();
    else if(state == VISUAL_ATTENTION)
        ink = EscalatedRed();
    for(int st = 0; st < 4; ++st)
        style.palette.ink[st] = st == ST_DISABLED ? DisabledWorkflowColor(ink) : ink;
    return style;
}

UiButton::Style TaskTrackQuestionCtrl::MakeStateButtonStyle(VisualState state) const
{
    UiButton::Style style = UiTheme::ResolveButton(UiTheme::GetContext(), UiRole::Standard);
    style.font = SansSerifZ(8).Bold();
    style.metrics.use_text_font = false;
    style.metrics.face_enabled = true;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = state == VISUAL_SUGGESTED ? DPI(1) : DPI(2);

    Color c = SuggestedGrey();
    int face_mix = 12;
    if(state == VISUAL_REQUIRED_PENDING) { c = PendingOrange(); face_mix = 24; }
    else if(state == VISUAL_ANSWERED) { c = AnsweredGreen(); face_mix = 28; }
    else if(state == VISUAL_ATTENTION) { c = EscalatedRed(); face_mix = 24; }

    for(int st = 0; st < 4; ++st) {
        Color state_color = st == ST_DISABLED ? DisabledWorkflowColor(c) : c;
        style.palette.frame[st] = state_color;
        style.palette.face[st] = UiFill::Solid(Blend(SColorPaper(), c, st == ST_DISABLED ? max(8, face_mix / 2) : face_mix));
        style.palette.ink[st] = state_color;
    }
    style.metrics.content_margin = Rect(DPI(5), DPI(3), DPI(5), DPI(3));
    return style;
}

TaskTrackQuestionCtrl::VisualState TaskTrackQuestionCtrl::ResolveVisualState() const
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return VISUAL_SUGGESTED;
    const TaskTrackItem& item = document_->items[item_index_];
    if(item.answer.answered)
        return VISUAL_ANSWERED;
    if(needs_attention_ && item.required)
        return VISUAL_ATTENTION;
    if(item.required && item.recommended.IsEmpty())
        return VISUAL_REQUIRED_PENDING;
    return VISUAL_SUGGESTED;
}

void TaskTrackQuestionCtrl::ApplyVisualStatePresentation()
{
    if(destroying_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    VisualState state = ResolveVisualState();
    if(applied_visual_state_ == (int)state)
        return;
    applied_visual_state_ = (int)state;

    UiGroupPanel::Style style = GetStyle();
    UiGroupPanel::Style standard = UiTheme::ResolveGroupPanel(UiRole::Standard);
    style.palette = standard.palette;
    style.metrics.face_enabled = true;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = state == VISUAL_SUGGESTED ? DPI(1) : DPI(2);

    Color frame = SuggestedGrey();
    int face_mix = 0;
    if(state == VISUAL_REQUIRED_PENDING) { frame = PendingOrange(); face_mix = 18; }
    else if(state == VISUAL_ANSWERED) { frame = AnsweredGreen(); face_mix = 24; }
    else if(state == VISUAL_ATTENTION) { frame = EscalatedRed(); face_mix = 20; }

    for(int st = 0; st < 4; ++st) {
        style.palette.frame[st] = st == ST_DISABLED ? DisabledWorkflowColor(frame) : frame;
        if(face_mix > 0)
            style.palette.face[st] = UiFill::Solid(Blend(SColorPaper(), frame, st == ST_DISABLED ? max(8, face_mix / 2) : face_mix));
    }

    SetCustomStyle(style);
}

void TaskTrackQuestionCtrl::EnsureRecommendationHeader()
{
    if(recommendation_header_ready_)
        return;
    recommendation_header_ready_ = true;

    recommendation_header_.SetDirection(UiDirection::H)
        .SetGap(DPI(4))
        .SetInset(0)
        .SetAlignItems(UiCrossAlign::Center);

    recommendation_header_label_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    recommendation_accept_.SetContentInset(DPI(2));
    recommendation_help_.SetCustomStyle(MakeStateButtonStyle(VISUAL_SUGGESTED));
    recommendation_help_.SetText("?").SetContentInset(DPI(1));

    recommendation_header_.Add(recommendation_header_label_).Fit().MinMain(DPI(58)).MinCross(DPI(23));
    recommendation_header_.Add(recommendation_accept_).Fixed(DPI(58)).MinCross(DPI(23));
    recommendation_header_.Add(recommendation_help_).Fixed(DPI(26)).MinCross(DPI(23));

    recommendation_accept_.WhenAction = [=] {
        if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
            return;
        const TaskTrackItem& item = document_->items[item_index_];
        if(item.recommended.IsEmpty())
            QueueAgentRequest("propose_answer");
        else
            AcceptRecommendation();
    };
    recommendation_help_.WhenAction = [=] { QueueAgentRequest("clarify", "simplify"); };
}

void TaskTrackQuestionCtrl::ApplyRecommendedChoiceStyles(const TaskTrackItem& item)
{
    if(item.recommended.IsEmpty())
        return;

    bool show = !item.answer.answered;
    if(item.type == TaskTrackItemType::Confirm && radios_.GetCount() >= 2) {
        String labels[2] = { "Yes", "No" };
        if(item.choices.GetCount() == 2) { labels[0] = item.choices[0]; labels[1] = item.choices[1]; }
        String lower = ToLower(TrimBoth(item.recommended));
        for(int i = 0; i < 2; ++i) {
            bool recommended = show && (item.recommended == labels[i] ||
                (i == 0 && (lower == "yes" || lower == "true" || lower == "1")) ||
                (i == 1 && (lower == "no" || lower == "false" || lower == "0")));
            UiRadioButton::Style style = UiTheme::ResolveRadioButton(UiTheme::GetContext(),
                recommended ? UiRole::Subtle : UiRole::Standard, UIRADIOVIS_PILLS);
            style.font = SansSerifZ(9);
            radios_[i].SetCustomStyle(style);
        }
    }
    else if(item.type == TaskTrackItemType::SingleChoice && !radios_.IsEmpty()) {
        for(int i = 0; i < radios_.GetCount() && i < item.choices.GetCount(); ++i) {
            bool recommended = show && TaskTrackRecommendationContains(item, item.choices[i]);
            UiRadioButton::Style style = UiTheme::ResolveRadioButton(UiTheme::GetContext(),
                recommended ? UiRole::Subtle : UiRole::Standard, UIRADIOVIS_PILLS);
            style.font = SansSerifZ(9);
            radios_[i].SetCustomStyle(style);
        }
    }
    else if(item.type == TaskTrackItemType::MultiChoice) {
        for(int i = 0; i < checks_.GetCount() && i < item.choices.GetCount(); ++i) {
            bool recommended = show && TaskTrackRecommendationContains(item, item.choices[i]);
            UiCheckBox::Style style = UiTheme::ResolveCheckBox(UiTheme::GetContext(),
                recommended ? UiRole::Subtle : UiRole::Standard, UICHECKVIS_CLASSIC);
            style.font = SansSerifZ(9);
            checks_[i].SetCustomStyle(style);
        }
    }
    else if(item.type == TaskTrackItemType::Rating) {
        int first = (int)floor(item.min_value + 0.5);
        for(int i = 0; i < choice_buttons_.GetCount(); ++i) {
            bool recommended = show && TrimBoth(item.recommended) == AsString(first + i);
            UiButton::Style style = UiTheme::ResolveButton(UiTheme::GetContext(), recommended ? UiRole::Standard : UiRole::Subtle);
            style.font = SansSerifZ(9);
            style.metrics.use_text_font = false;
            if(recommended) {
                Color grey = SuggestedGrey();
                style.metrics.frame_enabled = true;
                style.metrics.frame_width = DPI(2);
                for(int st = 0; st < 4; ++st)
                    style.palette.frame[st] = st == ST_DISABLED ? DisabledWorkflowColor(grey) : grey;
            }
            choice_buttons_[i].SetCustomStyle(style);
        }
    }
    else if(item.type == TaskTrackItemType::Color && show) {
        Color grey = SuggestedGrey();
        for(int i = 0; i < color_buttons_.GetCount() && i < item.colors.GetCount(); ++i) {
            if(ToUpper(item.colors[i]) != ToUpper(TrimBoth(item.recommended)))
                continue;
            UiButton::Style style = color_buttons_[i].GetStyle();
            style.metrics.frame_enabled = true;
            style.metrics.frame_width = DPI(2);
            for(int st = 0; st < 4; ++st)
                style.palette.frame[st] = st == ST_DISABLED ? DisabledWorkflowColor(grey) : grey;
            color_buttons_[i].SetCustomStyle(style);
        }
    }
}

void TaskTrackQuestionCtrl::ApplyRecommendationPresentation()
{
    if(destroying_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    TaskTrackItem& item = document_->items[item_index_];
    VisualState state = ResolveVisualState();
    EnsureRecommendationHeader();

    int header_state = (int)state;
    if(recommendation_header_state_ != header_state) {
        recommendation_header_state_ = header_state;
        recommendation_header_label_.SetCustomStyle(MakeStatusLabelStyle(state));
        recommendation_accept_.SetCustomStyle(MakeStateButtonStyle(state));

        if(state == VISUAL_ANSWERED) {
            recommendation_header_label_.SetText("Answered");
            recommendation_accept_.SetText("Done").Disable();
            recommendation_header_.ItemAt(1).Fixed(DPI(50)).MinMain(DPI(50));
            recommendation_help_.Disable();
            recommendation_header_.ItemAt(2).Fixed(0).MinMain(0);
        }
        else if(state == VISUAL_REQUIRED_PENDING || state == VISUAL_ATTENTION) {
            if(!item.recommended.IsEmpty()) {
                recommendation_header_label_.SetText("Suggested: " + TaskTrackRecommendationSummary(item));
                recommendation_accept_.SetText("Accept").Enable();
                recommendation_header_.ItemAt(1).Fixed(DPI(50)).MinMain(DPI(50));
            }
            else {
                recommendation_header_label_.SetText(state == VISUAL_ATTENTION ? "Required" : "Needs decision");
                recommendation_accept_.SetText(proposal_pending_ ? "Waiting" : "Suggest");
                recommendation_accept_.Enable(!proposal_pending_);
                recommendation_header_.ItemAt(1).Fixed(DPI(58)).MinMain(DPI(58));
            }
            recommendation_help_.SetText(clarification_pending_ ? "…" : "?");
            recommendation_help_.Enable(!clarification_pending_);
            recommendation_header_.ItemAt(2).Fixed(DPI(26)).MinMain(DPI(26));
        }
        else {
            if(item.recommended.IsEmpty()) {
                recommendation_header_label_.SetText(item.required ? "Needs decision" : "Optional");
                recommendation_header_.ItemAt(1).Fixed(0).MinMain(0);
            }
            else {
                recommendation_header_label_.SetText("Suggested: " + TaskTrackRecommendationSummary(item));
                recommendation_accept_.SetText("Accept").Enable();
                recommendation_header_.ItemAt(1).Fixed(DPI(50)).MinMain(DPI(50));
            }
            recommendation_help_.SetText(clarification_pending_ ? "…" : "?");
            recommendation_help_.Enable(!clarification_pending_);
            recommendation_header_.ItemAt(2).Fixed(DPI(26)).MinMain(DPI(26));
        }
    }

    if(!recommendation_header_attached_) {
        recommendation_header_attached_ = true;
        SetHeaderContent(recommendation_header_);
        SetHeaderContentAlign(UiAlign::RIGHT, UiAlign::CENTER);
    }

    if(!recommendation_body_collapsed_) {
        int index = content_.FindItem(recommendation_);
        if(index >= 0) {
            recommendation_.Hide();
            content_.ItemAt(index).Fixed(0).MinMain(0).MinCross(0);
        }
        recommendation_body_collapsed_ = true;
    }

    if(!recommendation_styles_applied_) {
        ApplyRecommendedChoiceStyles(item);
        recommendation_styles_applied_ = true;
    }
}

void TaskTrackQuestionCtrl::AcceptRecommendation()
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;
    TaskTrackItem& item = document_->items[item_index_];
    if(!TaskTrackApplyRecommendation(item))
        return;
    SyncFromModel();
    RefreshVisualState();
    WhenChanged();
}

bool TaskTrackQuestionCtrl::ResolveTaskPath(String& path) const
{
    path.Clear();
    if(!document_ || document_->task_id.IsEmpty())
        return false;
    String error;
    return TaskTrackResolveTaskPath(document_->task_id, String(), path, error);
}

void TaskTrackQuestionCtrl::QueueAgentRequest(const String& action, const String& mode)
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;
    if(document_->state == TaskTrackState::Completed || document_->state == TaskTrackState::Closed)
        return;

    String path;
    if(!ResolveTaskPath(path)) {
        Exclamation("TaskTrack could not locate the durable task for agent assistance.");
        return;
    }

    String request_id, error;
    const TaskTrackItem& item = document_->items[item_index_];
    if(!TaskTrackQueueAgentRequest(path, document_->task_id, item.id, action, mode, request_id, error)) {
        Exclamation("TaskTrack could not request agent assistance.\n" + error);
        return;
    }

    if(action == "propose_answer")
        proposal_pending_ = true;
    else if(action == "clarify")
        clarification_pending_ = true;
    recommendation_header_state_ = -1;
    RefreshLayout();
    ArmAgentPoll();
}

void TaskTrackQuestionCtrl::ArmAgentPoll()
{
    KillTimeCallback(TASKTRACK_AGENT_POLL_TIMER_ID);
    SetTimeCallback(-750, [=] { CheckAgentChannel(true); }, TASKTRACK_AGENT_POLL_TIMER_ID);
}

void TaskTrackQuestionCtrl::CheckAgentChannel(bool show_clarification)
{
    if(destroying_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    String path;
    if(!ResolveTaskPath(path))
        return;

    TaskTrackAgentChannel channel;
    String error;
    if(!TaskTrackLoadAgentChannel(path, document_->task_id, channel, error))
        return;

    const TaskTrackItem& item = document_->items[item_index_];
    bool pending_proposal = TaskTrackAgentHasPending(channel, item.id, "propose_answer");
    bool pending_clarify = TaskTrackAgentHasPending(channel, item.id, "clarify");
    bool changed = channel.updated_at != last_seen_agent_update_ ||
                   pending_proposal != proposal_pending_ || pending_clarify != clarification_pending_;
    if(!changed)
        return;

    last_seen_agent_update_ = channel.updated_at;
    proposal_pending_ = pending_proposal;
    clarification_pending_ = pending_clarify;
    ApplyAgentChannel(channel, show_clarification);

    if(!proposal_pending_ && !clarification_pending_)
        KillTimeCallback(TASKTRACK_AGENT_POLL_TIMER_ID);
}

void TaskTrackQuestionCtrl::ApplyAgentChannel(const TaskTrackAgentChannel& channel, bool show_clarification)
{
    if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
        return;

    TaskTrackItem& item = document_->items[item_index_];
    String recommendation = TaskTrackAgentLatestRecommendation(channel, item.id);
    bool recommendation_changed = !recommendation.IsEmpty() && recommendation != item.recommended;
    if(recommendation_changed)
        item.recommended = recommendation;

    String clarification = TaskTrackAgentLatestClarification(channel, item.id);
    if(!clarification.IsEmpty() && clarification != last_clarification_) {
        last_clarification_ = clarification;
        ShowClarification(clarification);
        if(show_clarification)
            PromptOK("Agent clarification\n\n" + clarification);
    }

    if(recommendation_changed) {
        String path;
        String error;
        if(ResolveTaskPath(path)) {
            document_->updated_at = TaskTrackNowIso();
            TaskTrackSave(path, *document_, error);
        }
        SyncFromModel();
    }

    recommendation_header_state_ = -1;
    recommendation_styles_applied_ = false;
    applied_visual_state_ = -1;
    RefreshLayout();
    Refresh();

    if(recommendation_changed)
        WhenChanged();
}

void TaskTrackQuestionCtrl::ShowClarification(const String& text)
{
    if(text.IsEmpty())
        return;

    clarification_.SetText("Simpler: " + text);
    UiLabel::Style style = UiTheme::ResolveLabel(UiTheme::GetContext(), UiRole::Subtle, UiTextSize::Body);
    style.font = SansSerifZ(8);
    clarification_.SetCustomStyle(style);
    clarification_.SetAlign(UiAlign::LEFT, UiAlign::CENTER);

    if(clarification_attached_ && content_.FindItem(clarification_) < 0)
        clarification_attached_ = false;
    if(!clarification_attached_) {
        clarification_attached_ = true;
        content_.Add(clarification_).Fit().MinCross(DPI(20)).AlignSelf(UiBoxLayout::Align::Stretch);
    }
    clarification_.Show();
    content_.RefreshLayout();
}

} // namespace Upp
