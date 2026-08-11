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

    void Bind(TaskTrackDocument& document, int item_index);
    int GetItemIndex() const { return item_index_; }

    Event<> WhenChanged;

private:
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

    TaskTrackDocument* document_ = nullptr;
    int item_index_ = -1;
    bool syncing_ = false;

    UiBoxLayout content_ { UiDirection::V };
    UiLabel recommendation_;
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
    TaskTrackRangeSelector range_;
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
