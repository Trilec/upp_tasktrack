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
#include <TaskTrack/Core/TaskTrackAgent.h>

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

// Legacy implementation kept source-compatible while the active question
// renderer uses TaskTrackRangeField/UiRangeSliderEdit.
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

class TaskTrackRangeField : public UiRangeSliderEdit {
public:
    TaskTrackRangeField()
    {
        SetFieldWidth(DPI(52));
        SetGap(DPI(5));
        SetInset(DPI(5));
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

// UiCompositeColor was retired from upp_Ui. This adapter delegates state and
// picker behaviour to UiColorMatrix without recreating composite state.
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
    ~TaskTrackQuestionCtrl() override;

    void Bind(TaskTrackDocument& document, int item_index);
    int GetItemIndex() const { return item_index_; }

    void SetNeedsAttention(bool on);
    bool NeedsAttention() const { return needs_attention_; }
    void RefreshVisualState();
    void Layout() override;

    Event<> WhenChanged;

private:
    enum VisualState {
        VISUAL_SUGGESTED = 0,      // grey: agent proposal / normal baseline
        VISUAL_REQUIRED_PENDING,   // orange: required, no proposal
        VISUAL_ANSWERED,           // green: human resolved
        VISUAL_ATTENTION,          // red: unresolved after attempted continuation
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

    static Color SuggestedGrey();
    static Color PendingOrange();
    static Color AnsweredGreen();
    static Color EscalatedRed();

    UiLabel::Style MakeStatusLabelStyle(VisualState state) const;
    UiButton::Style MakeStateButtonStyle(VisualState state) const;
    VisualState ResolveVisualState() const;
    void ApplyVisualStatePresentation();
    void EnsureRecommendationHeader();
    void ApplyRecommendedChoiceStyles(const TaskTrackItem& item);
    void ApplyRecommendationPresentation();
    void AcceptRecommendation();

    bool ResolveTaskPath(String& path) const;
    void QueueAgentRequest(const String& action, const String& mode = String());
    void ArmAgentPoll();
    void CheckAgentChannel(bool show_clarification = true);
    void ApplyAgentChannel(const TaskTrackAgentChannel& channel, bool show_clarification);
    void ShowClarification(const String& text);

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
    bool proposal_pending_ = false;
    bool clarification_pending_ = false;
    bool clarification_attached_ = false;
    int applied_visual_state_ = -1;
    int recommendation_header_state_ = -1;
    String last_clarification_;
    String last_seen_agent_update_;

    UiBoxLayout content_ { UiDirection::V };
    UiLabel recommendation_;
    UiLabel clarification_;
    UiBoxLayout recommendation_header_ { UiDirection::H };
    UiLabel recommendation_header_label_;
    UiButton recommendation_accept_;
    UiButton recommendation_help_;
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
