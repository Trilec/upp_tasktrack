#ifndef _TaskTrack_App_TaskTrackResponsiveFlow_h_
#define _TaskTrack_App_TaskTrackResponsiveFlow_h_

#include <Ui/Ui.h>

namespace Upp {

// Responsive flow used by the category strip. Buttons stay compact and equal
// within the current row, wrap when the available width becomes too small,
// and explicitly propagate a changed wrapped height back to the shell.
class TaskTrackCategoryFlow : public UiBoxLayout {
public:
    TaskTrackCategoryFlow(UiDirection d = UiDirection::H)
        : UiBoxLayout(d)
    {
    }

    TaskTrackCategoryFlow& SetDirection(UiDirection d)
    {
        UiBoxLayout::SetDirection(d);
        return *this;
    }

    TaskTrackCategoryFlow& SetGap(int x, int y)
    {
        gap_x_ = max(DPI(6), x);
        gap_y_ = max(DPI(6), y);
        UiBoxLayout::SetGap(gap_x_, gap_y_);
        return *this;
    }

    TaskTrackCategoryFlow& SetInset(int all)
    {
        inset_ = max(0, all);
        UiBoxLayout::SetInset(inset_);
        return *this;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(0, GetSize().cx - inset_ * 2);
            int minimum = DPI(120);
            int maximum = DPI(180);
            int columns = min(GetItemCount(), max(1, (available + gap_x_) / (minimum + gap_x_)));
            int width = columns > 0 ? (available - gap_x_ * max(0, columns - 1)) / columns : minimum;
            width = minmax(width, minimum, maximum);

            adapting_ = true;
            PauseLayout();
            for(int i = 0; i < GetItemCount(); ++i)
                ItemAt(i).MinMaxMain(width, width).Fit().MinCross(DPI(26)).AlignSelf(UiCrossAlign::Center);
            ResumeLayout(false);
            adapting_ = false;
        }

        UiBoxLayout::Layout();

        int used_height = GetUsedSize().cy;
        if(!notifying_parent_ && used_height != last_used_height_) {
            last_used_height_ = used_height;
            notifying_parent_ = true;
            Ctrl *parent = GetParent();
            if(parent) {
                parent->RefreshLayout();
                if(parent->GetParent())
                    parent->GetParent()->RefreshLayout();
            }
            notifying_parent_ = false;
        }
    }

private:
    int gap_x_ = DPI(6);
    int gap_y_ = DPI(6);
    int inset_ = 0;
    int last_used_height_ = -1;
    bool adapting_ = false;
    bool notifying_parent_ = false;
};

// Responsive question workspace. Every row is an ordered grid-like flow with
// one deterministic card width, at most three columns, and equal row height.
// The last partial row keeps the same column width instead of stretching into
// unrelated masonry widths.
class TaskTrackQuestionFlow : public UiBoxLayout {
public:
    TaskTrackQuestionFlow(UiDirection d = UiDirection::H)
        : UiBoxLayout(d)
    {
    }

    TaskTrackQuestionFlow& SetDirection(UiDirection d)
    {
        UiBoxLayout::SetDirection(d);
        return *this;
    }

    TaskTrackQuestionFlow& SetGap(int x, int y)
    {
        gap_x_ = max(DPI(10), x);
        gap_y_ = max(DPI(10), y);
        UiBoxLayout::SetGap(gap_x_, gap_y_);
        return *this;
    }

    TaskTrackQuestionFlow& SetInset(int all)
    {
        inset_ = max(0, all);
        UiBoxLayout::SetInset(inset_);
        return *this;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(0, GetSize().cx - inset_ * 2);
            int minimum = DPI(280);
            int columns = min(3, max(1, (available + gap_x_) / (minimum + gap_x_)));
            int width = columns > 0 ? (available - gap_x_ * max(0, columns - 1)) / columns : minimum;
            width = max(DPI(220), width);

            adapting_ = true;
            PauseLayout();
            for(int i = 0; i < GetItemCount(); ++i)
                ItemAt(i).MinMaxMain(width, width).Fit().MinCross(DPI(92)).AlignSelf(UiCrossAlign::Stretch);
            ResumeLayout(false);
            adapting_ = false;
        }

        UiBoxLayout::Layout();
    }

private:
    int gap_x_ = DPI(10);
    int gap_y_ = DPI(10);
    int inset_ = 0;
    bool adapting_ = false;
};

} // namespace Upp

#endif
