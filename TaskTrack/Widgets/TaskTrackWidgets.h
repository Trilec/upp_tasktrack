#ifndef _TaskTrack_Widgets_TaskTrackWidgets_h_
#define _TaskTrack_Widgets_TaskTrackWidgets_h_

/*
    TaskTrack Widgets
    =================

    Data-driven U++ renderer for TaskTrack's semantic human questions plus the
    small specialist selectors that are not already represented directly by
    the Ui package.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackCore.h>

namespace Upp {

class TaskTrackPosition9 : public ParentCtrl {
public:
    typedef TaskTrackPosition9 CLASSNAME;

    TaskTrackPosition9();

    TaskTrackPosition9& SetValue(const String& value);
    String GetValue() const { return value_; }
    Size GetMinSize() const override;
    void Layout() override;

    Event<> WhenAction;

private:
    void Sync();
    String ValueAt(int index) const;

    Array<UiButton> buttons_;
    String value_;
};

class TaskTrackDirection8 : public ParentCtrl {
public:
    typedef TaskTrackDirection8 CLASSNAME;

    TaskTrackDirection8();

    TaskTrackDirection8& SetValue(const String& value);
    String GetValue() const { return value_; }
    Size GetMinSize() const override;
    void Layout() override;

    Event<> WhenAction;

private:
    void Sync();
    String ValueAt(int index) const;

    Array<UiButton> buttons_;
    String value_;
};

// Legacy implementation kept source-compatible for this checkpoint while the
// active question renderer has moved to TaskTrackRangeField/UiRangeSliderEdit.
// It is no longer instantiated by TaskTrackQuestionCtrl.
class TaskTrackRangeSelector : public Ctrl {
public:
    typedef TaskTrackRangeSelector CLASSNAME;

    TaskTrackRangeSelector();

    TaskTrackRangeSelector& SetRange(double mn, double mx);
    TaskTrackRangeSelector& SetStep(double step);
    TaskTrackRangeSelector& SetValues(double low, double high);
    TaskTrackRangeSelector& SetUnit(const String& unit);

    double GetLow() const { return low_; }
    double GetHigh() const { return high_; }

    Size GetMinSize() const override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;
    void LeftUp(Point p, dword flags) override;
    void MouseMove(Point p, dword flags) override;
    void LostFocus() override;
    bool Key(dword key, int count) override;

    Event<> WhenChanging;
    Event<> WhenAction;

private:
    double ClampSnap(double value) const;
    int XFromValue(double value) const;
    double ValueFromX(int x) const;
    int HitThumb(Point p) const;
    void SetDraggedValue(Point p, bool fire_changing);

    double mn_ = 0.0;
    double mx_ = 100.0;
    double step_ = 1.0;
    double low_ = 25.0;
    double high_ = 75.0;
    String unit_;
    int dragging_ = -1;
    int selected_ = 0;
};

// Semantic range adapter over the current Ui composition. It owns no duplicate
// interval state: UiRangeSliderEdit remains authoritative and supplies the
// themed anti-aliased slider plus direct lower/upper numeric fields.
class TaskTrackRangeField : public UiRangeSliderEdit {
public:
    TaskTrackRangeField()
    {
        SetFieldWidth(DPI(52));
        SetGap(DPI(5));
        SetInset(DPI(5));
        // Endpoint markers compete with the two direct numeric fields in this
        // compact card composition. The handles themselves remain visible.
        Slider().ShowEndpointMarkers(false);
    }

    TaskTrackRangeField& SetRange(double mn, double mx)
    {
        UiRangeSliderEdit::SetRange(mn, mx);
        return *this;
    }

    TaskTrackRangeField& SetStep(double step)
    {
        UiRangeSliderEdit::SetStep(step);
        // UiFloatEdit::Precision feeds FormatDouble significant precision;
        // zero turns ordinary values such as 375 into scientific 4e2 display.
        SetPrecision(12);
        return *this;
    }

    TaskTrackRangeField& SetValues(double low, double high)
    {
        UiRangeSliderEdit::SetValues(low, high);
        return *this;
    }

    TaskTrackRangeField& SetUnit(const String&) { return *this; }
    double GetLow() const { return GetLowerValue(); }
    double GetHigh() const { return GetUpperValue(); }
};

class TaskTrackGradientSelector : public Ctrl {
public:
    typedef TaskTrackGradientSelector CLASSNAME;

    TaskTrackGradientSelector();

    TaskTrackGradientSelector& SetOptions(const Vector<TaskTrackGradientOption>& options);
    TaskTrackGradientSelector& SetValue(const String& value);
    String GetValue() const { return value_; }

    Size GetMinSize() const override;
    void Paint(Draw& w) override;
    void LeftDown(Point p, dword flags) override;

    Event<> WhenAction;

private:
    Rect OptionRect(int index) const;
    int HitOption(Point p) const;
    void PaintGradient(Draw& w, const Rect& r, Color a, Color b) const;
    Color ParseColor(const String& text, Color fallback) const;

    Vector<TaskTrackGradientOption> options_;
    String value_;
};

// UiCompositeColor was retired from upp_Ui. TaskTrack keeps no replacement
// colour state or picker: this narrow renderer adapter delegates both entirely
// to the current first-class UiColorMatrix. The V0.2 renderer's obsolete
// label/value layout hints are intentionally absorbed because UiColorMatrix now
// owns that presentation.
class TaskTrackColorField : public UiColorMatrix {
public:
    TaskTrackColorField& SetLabelStyle(const UiLabel::Style&) { return *this; }
    TaskTrackColorField& SetValueStyle(const UiLabel::Style&) { return *this; }
    TaskTrackColorField& SetLabel(const String& text)
    {
        SetColorLabel(0, text);
        return *this;
    }
    TaskTrackColorField& SetColorCount(int count)
    {
        UiColorMatrix::SetColorCount(count);
        return *this;
    }
    TaskTrackColorField& ShowValue(bool = true) { return *this; }
    TaskTrackColorField& SetLabelWidth(int) { return *this; }
    TaskTrackColorField& SetValueWidth(int) { return *this; }
    TaskTrackColorField& SetColor(int index, Color color, bool fire = false)
    {
        UiColorMatrix::SetColor(index, color, fire);
        return *this;
    }
    TaskTrackColorField& SetValueText(const String&) { return *this; }
};

class TaskTrackQuestionCtrl : public UiGroupPanel {
public:
    typedef TaskTrackQuestionCtrl CLASSNAME;

    TaskTrackQuestionCtrl();

    ~TaskTrackQuestionCtrl() override { destroying_ = true; }

    void Bind(TaskTrackDocument& document, int item_index);
    int GetItemIndex() const { return item_index_; }

    void SetNeedsAttention(bool on)
    {
        if(needs_attention_ == on)
            return;
        needs_attention_ = on;
        RefreshVisualState();
    }

    bool NeedsAttention() const { return needs_attention_; }

    void RefreshVisualState()
    {
        if(destroying_)
            return;
        applied_visual_state_ = -1;
        recommendation_header_state_ = -1;
        recommendation_styles_applied_ = false;
        RefreshLayout();
        Refresh();
    }

    void Layout() override
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
        ApplyVisualStatePresentation();
        ApplyRecommendationPresentation();
        UiGroupPanel::Layout();
    }

    Event<> WhenChanged;

private:
    enum VisualState {
        VISUAL_NEUTRAL = 0,
        VISUAL_SUGGESTED,
        VISUAL_ANSWERED,
        VISUAL_ATTENTION,
    };

    void Configure();
    void ClearDynamicControls();
    void BuildConfirm();
    void BuildSingleChoice();
    void BuildMultiChoice();
    void BuildSelect();
    void BuildListSelect();
    void BuildText();
    void BuildNotes();
    void BuildNumber();
    void BuildAmount();
    void BuildRange();
    void BuildRating();
    void BuildColor();
    void BuildGradient();
    void BuildPosition();
    void BuildDirection();
    void BuildRankOrder();
    void BuildHierarchy();
    void BuildCurve();

    void SyncFromModel();
    void CommitSimpleValue(const String& status, const String& value, const Value& data);
    void CommitCurrent();
    void CommitRankOrder(bool confirmed);
    void CommitHierarchy();
    void CommitCurve(bool confirmed);
    void TouchAnswer(TaskTrackAnswer& answer, bool answered);

    void PopulateList(UiList& list, const Vector<String>& choices);
    void PopulateHierarchy();
    void AddHierarchyChildren(const String& parent_id, UiTreeNodeRef parent);
    Color ParseColor(const String& text, Color fallback = Black()) const;
    String ColorToText(Color color) const;

    static Color AnsweredGreen() { return Color(45, 142, 77); }

    UiLabel::Style MakeStatusLabelStyle(VisualState state) const
    {
        UiRole role = state == VISUAL_ATTENTION ? UiRole::Alert
                    : state == VISUAL_SUGGESTED ? UiRole::Accent
                    : UiRole::Standard;
        UiLabel::Style style = UiTheme::ResolveLabel(UiTheme::GetContext(), role, UiTextSize::Body);
        style.font = SansSerifZ(8);
        if(state == VISUAL_ANSWERED) {
            Color green = AnsweredGreen();
            for(int st = 0; st < 4; ++st)
                style.palette.ink[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 100) : green;
        }
        return style;
    }

    UiButton::Style MakeAnsweredButtonStyle() const
    {
        UiButton::Style style = UiTheme::ResolveButton(UiTheme::GetContext(), UiRole::Standard);
        style.font = SansSerifZ(8).Bold();
        style.metrics.use_text_font = false;
        style.metrics.frame_enabled = true;
        style.metrics.face_enabled = true;
        style.metrics.frame_width = DPI(2);
        Color green = AnsweredGreen();
        for(int st = 0; st < 4; ++st) {
            style.palette.frame[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 100) : green;
            style.palette.face[st] = UiFill::Solid(Blend(SColorPaper(), green, st == ST_DISABLED ? 16 : 28));
            style.palette.ink[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 120) : green;
        }
        return style;
    }

    VisualState ResolveVisualState() const
    {
        if(!document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
            return VISUAL_NEUTRAL;
        const TaskTrackItem& item = document_->items[item_index_];
        if(item.answer.answered)
            return VISUAL_ANSWERED;
        if(needs_attention_ && item.required)
            return VISUAL_ATTENTION;
        if(!item.recommended.IsEmpty())
            return VISUAL_SUGGESTED;
        return VISUAL_NEUTRAL;
    }

    void ApplyVisualStatePresentation()
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
        style.metrics.frame_width = DPI(1);

        if(state == VISUAL_SUGGESTED) {
            UiGroupPanel::Style accent = UiTheme::ResolveGroupPanel(UiRole::Accent);
            for(int st = 0; st < 4; ++st)
                style.palette.frame[st] = accent.palette.frame[st];
        }
        else if(state == VISUAL_ANSWERED) {
            Color green = AnsweredGreen();
            style.metrics.frame_width = DPI(2);
            for(int st = 0; st < 4; ++st) {
                style.palette.face[st] = UiFill::Solid(Blend(SColorPaper(), green, st == ST_DISABLED ? 14 : 24));
                style.palette.frame[st] = st == ST_DISABLED ? Blend(green, SColorDisabled(), 100) : green;
            }
        }
        else if(state == VISUAL_ATTENTION) {
            UiGroupPanel::Style alert = UiTheme::ResolveGroupPanel(UiRole::Alert);
            style.palette = alert.palette;
            style.metrics.frame_width = DPI(2);
        }

        SetCustomStyle(style);
    }

    void EnsureRecommendationHeader()
    {
        if(recommendation_header_ready_)
            return;
        recommendation_header_ready_ = true;

        recommendation_header_.SetDirection(UiDirection::H)
            .SetGap(DPI(4))
            .SetInset(0)
            .SetAlignItems(UiCrossAlign::Center);

        recommendation_header_label_.SetCustomStyle(MakeStatusLabelStyle(VISUAL_SUGGESTED));
        recommendation_header_label_.SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

        UiButton::Style accept_style = UiTheme::ResolveButton(UiTheme::GetContext(), UiRole::Accent);
        accept_style.font = SansSerifZ(8).Bold();
        accept_style.metrics.content_margin = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
        recommendation_accept_.SetCustomStyle(accept_style);
        recommendation_accept_.SetText("Accept").SetContentInset(DPI(2));

        recommendation_header_.Add(recommendation_header_label_).Fit().MinMain(DPI(58)).MinCross(DPI(23));
        recommendation_header_.Add(recommendation_accept_).Fixed(DPI(50)).MinCross(DPI(23));
        recommendation_accept_.WhenAction = [=] { AcceptRecommendation(); };
    }

    void ApplyRecommendedChoiceStyles(const TaskTrackItem& item)
    {
        if(item.recommended.IsEmpty())
            return;

        bool show_recommendation = !item.answer.answered;

        if(item.type == TaskTrackItemType::Confirm && radios_.GetCount() >= 2) {
            String labels[2] = { "Yes", "No" };
            if(item.choices.GetCount() == 2) {
                labels[0] = item.choices[0];
                labels[1] = item.choices[1];
            }
            String lower = ToLower(TrimBoth(item.recommended));
            for(int i = 0; i < 2; ++i) {
                bool recommended = show_recommendation && (item.recommended == labels[i] ||
                                   (i == 0 && (lower == "yes" || lower == "true" || lower == "1")) ||
                                   (i == 1 && (lower == "no" || lower == "false" || lower == "0")));
                UiRadioButton::Style style = UiTheme::ResolveRadioButton(UiTheme::GetContext(),
                    recommended ? UiRole::Accent : UiRole::Standard, UIRADIOVIS_PILLS);
                style.font = SansSerifZ(9);
                radios_[i].SetCustomStyle(style);
            }
        }
        else if(item.type == TaskTrackItemType::SingleChoice && !radios_.IsEmpty()) {
            for(int i = 0; i < radios_.GetCount() && i < item.choices.GetCount(); ++i) {
                bool recommended = show_recommendation && TaskTrackRecommendationContains(item, item.choices[i]);
                UiRadioButton::Style style = UiTheme::ResolveRadioButton(UiTheme::GetContext(),
                    recommended ? UiRole::Accent : UiRole::Standard, UIRADIOVIS_PILLS);
                style.font = SansSerifZ(9);
                radios_[i].SetCustomStyle(style);
            }
        }
        else if(item.type == TaskTrackItemType::MultiChoice) {
            for(int i = 0; i < checks_.GetCount() && i < item.choices.GetCount(); ++i) {
                bool recommended = show_recommendation && TaskTrackRecommendationContains(item, item.choices[i]);
                UiCheckBox::Style style = UiTheme::ResolveCheckBox(UiTheme::GetContext(),
                    recommended ? UiRole::Accent : UiRole::Standard, UICHECKVIS_CLASSIC);
                style.font = SansSerifZ(9);
                checks_[i].SetCustomStyle(style);
            }
        }
        else if(item.type == TaskTrackItemType::Rating) {
            int first = (int)floor(item.min_value + 0.5);
            for(int i = 0; i < choice_buttons_.GetCount(); ++i) {
                String value = AsString(first + i);
                UiButton::Style style = UiTheme::ResolveButton(UiTheme::GetContext(),
                    show_recommendation && TrimBoth(item.recommended) == value ? UiRole::Accent : UiRole::Subtle);
                style.font = SansSerifZ(9);
                style.metrics.use_text_font = false;
                choice_buttons_[i].SetCustomStyle(style);
            }
        }
        else if(item.type == TaskTrackItemType::Color && show_recommendation) {
            UiButton::Style accent = UiTheme::ResolveButton(UiTheme::GetContext(), UiRole::Accent);
            for(int i = 0; i < color_buttons_.GetCount() && i < item.colors.GetCount(); ++i) {
                if(ToUpper(item.colors[i]) != ToUpper(TrimBoth(item.recommended)))
                    continue;
                UiButton::Style style = color_buttons_[i].GetStyle();
                for(int st = 0; st < 4; ++st)
                    style.palette.frame[st] = accent.palette.frame[st];
                style.metrics.frame_enabled = true;
                style.metrics.frame_width = DPI(2);
                color_buttons_[i].SetCustomStyle(style);
            }
        }
    }

    void ApplyRecommendationPresentation()
    {
        if(destroying_ || !document_ || item_index_ < 0 || item_index_ >= document_->items.GetCount())
            return;

        TaskTrackItem& item = document_->items[item_index_];
        VisualState state = ResolveVisualState();
        bool show_header = state != VISUAL_NEUTRAL;

        if(!show_header) {
            if(recommendation_header_attached_) {
                ClearHeaderContent();
                recommendation_header_attached_ = false;
            }
            return;
        }

        EnsureRecommendationHeader();
        int header_state = (int)state;
        if(recommendation_header_state_ != header_state) {
            recommendation_header_state_ = header_state;
            recommendation_header_label_.SetCustomStyle(MakeStatusLabelStyle(state));

            if(state == VISUAL_ANSWERED) {
                recommendation_header_label_.SetText("Answered");
                recommendation_accept_.SetCustomStyle(MakeAnsweredButtonStyle());
                recommendation_accept_.SetText("Done").Disable();
                recommendation_header_.ItemAt(1).Fixed(DPI(50)).MinMain(DPI(50));
            }
            else if(state == VISUAL_ATTENTION) {
                recommendation_header_label_.SetText("Needs input");
                recommendation_accept_.Disable();
                recommendation_header_.ItemAt(1).Fixed(0).MinMain(0);
            }
            else {
                recommendation_header_label_.SetText("Suggested: " + TaskTrackRecommendationSummary(item));
                UiButton::Style accept_style = UiTheme::ResolveButton(UiTheme::GetContext(), UiRole::Accent);
                accept_style.font = SansSerifZ(8).Bold();
                accept_style.metrics.content_margin = Rect(DPI(6), DPI(3), DPI(6), DPI(3));
                recommendation_accept_.SetCustomStyle(accept_style);
                recommendation_accept_.SetText("Accept").Enable();
                recommendation_header_.ItemAt(1).Fixed(DPI(50)).MinMain(DPI(50));
            }
        }

        if(!recommendation_header_attached_) {
            // Mark attached before any layout-affecting setter: those trigger a
            // synchronous re-layout, which re-enters ApplyRecommendationPresentation.
            recommendation_header_attached_ = true;
            SetHeaderContent(recommendation_header_);
            SetHeaderContentAlign(UiAlign::RIGHT, UiAlign::CENTER);
        }

        // V0.2 placed `Agent suggests:` as a separate body row. Keep that
        // source-compatible label alive, but collapse its layout item now that
        // the GroupPanel header-content slot owns the semantic state UI.
        if(!recommendation_body_collapsed_ && !item.recommended.IsEmpty() && content_.GetItemCount() > 0) {
            recommendation_.Hide();
            content_.ItemAt(0).Fixed(0).MinMain(0).MinCross(0);
            recommendation_body_collapsed_ = true;
        }

        if(!recommendation_styles_applied_) {
            ApplyRecommendedChoiceStyles(item);
            recommendation_styles_applied_ = true;
        }
    }

    void AcceptRecommendation()
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

    TaskTrackDocument* document_ = nullptr;
    int item_index_ = -1;
    bool syncing_ = false;
    bool workspace_typography_applied_ = false;
    bool recommendation_header_ready_ = false;
    bool recommendation_header_attached_ = false;
    bool recommendation_body_collapsed_ = false;
    bool recommendation_styles_applied_ = false;
    bool needs_attention_ = false;
    bool destroying_ = false;
    int applied_visual_state_ = -1;
    int recommendation_header_state_ = -1;

    UiBoxLayout content_ { UiDirection::V };
    UiLabel recommendation_;
    UiBoxLayout recommendation_header_ { UiDirection::H };
    UiLabel recommendation_header_label_;
    UiButton recommendation_accept_;
    UiBoxLayout response_ { UiDirection::H };

    Array<UiRadioButton> radios_;
    Array<UiCheckBox> checks_;
    Array<UiButton> choice_buttons_;
    Array<UiButton> color_buttons_;

    UiDropdown dropdown_;
    UiList list_;
    UiLineEdit text_;
    UiMultiEdit notes_;
    UiFloatEdit number_;
    UiSliderEdit amount_;
    UiLabel unit_label_;
    TaskTrackRangeField range_;
    TaskTrackGradientSelector gradient_;
    TaskTrackPosition9 position_;
    TaskTrackDirection8 direction_;
    UiList rank_;
    UiButton rank_confirm_;
    UiTree tree_;
    UiBezierCurveField curve_;
    UiButton curve_confirm_;
    TaskTrackColorField custom_color_;
};

} // namespace Upp

#endif
