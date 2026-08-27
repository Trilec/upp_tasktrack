#ifndef _TaskTrack_App_TaskTrackResponsiveFlow_h_
#define _TaskTrack_App_TaskTrackResponsiveFlow_h_

/*
    TaskTrack responsive flow
    =========================

    Width-aware category and question-card layout used by the native TaskTrack
    window. The question flow measures the controls it actually lays out so the
    window and its cards share one geometry path.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Ui/Ui.h>

namespace Upp {

// Responsive flow used by the category strip. Buttons stay compact and equal
// within the current row and wrap deterministically when width becomes tight.
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

    int DesiredHeightForWidth(int total_width) const
    {
        int count = GetItemCount();
        if(count <= 0)
            return 0;
        int available = max(1, total_width - inset_ * 2);
        int columns = ResolveColumns(available);
        int rows = (count + columns - 1) / columns;
        return inset_ * 2 + rows * category_row_height_ + max(0, rows - 1) * gap_y_;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(1, GetSize().cx - inset_ * 2);
            int columns = ResolveColumns(available);
            int width = ResolveItemWidth(available, columns);

            adapting_ = true;
            PauseLayout();
            UiBoxLayout::SetFixedColumn(0);
            for(int i = 0; i < GetItemCount(); ++i)
                ItemAt(i).MinMaxMain(width, width).Fit().MinCross(category_row_height_).AlignSelf(UiCrossAlign::Center);
            ResumeLayout(false);
            adapting_ = false;
        }
        UiBoxLayout::Layout();
    }

private:
    int ResolveColumns(int available) const
    {
        int count = max(1, GetItemCount());
        return min(count, max(1, (available + gap_x_) / (DPI(120) + gap_x_)));
    }

    int ResolveItemWidth(int available, int columns) const
    {
        int width = columns > 0 ? (available - gap_x_ * max(0, columns - 1)) / columns : DPI(120);
        return minmax(width, DPI(120), DPI(180));
    }

    int gap_x_ = DPI(6);
    int gap_y_ = DPI(6);
    int inset_ = 0;
    int category_row_height_ = DPI(30);
    bool adapting_ = false;
};

// Keep the normal UiGroupPanel chrome, but size the category body from the
// actual current shell width. During RebuildCategories() the flow is briefly
// empty while the old buttons are removed and the new set is being assembled.
// Preserve the last settled non-empty body height across that transient phase
// so the parent never remeasures the GroupPanel against an artificial zero-row
// state and then keeps the too-small height after the buttons return.
class TaskTrackCategoryPanel : public UiGroupPanel {
public:
    TaskTrackCategoryPanel& SetContent(Ctrl& ctrl)
    {
        flow_ = dynamic_cast<TaskTrackCategoryFlow *>(&ctrl);
        UiGroupPanel::SetContent(ctrl);
        return *this;
    }

    Size GetMinSize() const override
    {
        int width = GetSize().cx;
        if(width <= 0 && GetParent())
            width = GetParent()->GetSize().cx;
        width = max(DPI(160), width);

        const UiGroupPanel::Style& style = GetStyle();
        int flow_width = max(1, width - style.inset.left - style.inset.right);
        int body_height = DPI(30);
        if(flow_) {
            int measured = flow_->DesiredHeightForWidth(flow_width);
            if(measured > 0) {
                settled_body_height_ = measured;
                body_height = measured;
            }
            else if(settled_body_height_ > 0)
                body_height = settled_body_height_;
        }
        int title_height = GetTextSize("Categories", style.title_font).cy;
        int header_height = style.header_inset.top + title_height + style.header_inset.bottom;
        int content_height = header_height + style.header_gap
                           + style.inset.top + body_height + style.inset.bottom
                           + DPI(8); // lower breathing room inside the rounded frame
        Size outer = UiStyledOuterSizeFromContent(Size(DPI(160), content_height),
                                                  style.metrics, style.skin);
        return Size(DPI(160), max(DPI(64), outer.cy));
    }

private:
    TaskTrackCategoryFlow *flow_ = nullptr;
    mutable int settled_body_height_ = DPI(30);
};

// Responsive question workspace. Every row uses one canonical card width and
// equal row height. Height is measured from the fully assembled controls, not
// inferred from question type. This keeps the dialog and the actual card layout
// on one deterministic geometry path.
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

    TaskTrackQuestionFlow& SetMaxColumns(int columns)
    {
        max_columns_ = max(1, columns);
        RefreshLayout();
        return *this;
    }

    // Preferred width and live layout share ResolveColumns(): keep as many
    // columns as fit at the minimum width, prefer the canonical card width,
    // and shrink only when the available work area requires it.
    int PreferredWidthForItems(int maximum_width) const
    {
        int available_max = max(1, maximum_width - inset_ * 2);
        int columns = ResolveColumns(available_max);
        int preferred_inner = columns * preferred_column_width_
                            + max(0, columns - 1) * gap_x_;
        int inner = min(available_max, preferred_inner);
        return inset_ * 2 + max(1, inner);
    }

    // Measure the same assembled card geometry that Layout() will use. Each
    // card is briefly laid out at its assigned width so its real content and
    // GroupPanel chrome determine the row height.
    int DesiredHeightForWidth(int total_width)
    {
        int count = GetItemCount();
        if(count <= 0)
            return 0;

        int available = max(1, total_width - inset_ * 2);
        int columns = ResolveColumns(available);
        int width = ResolveItemWidth(available, columns);
        int total = inset_ * 2;
        int row_height = 0;
        int index = 0;

        for(Ctrl *q = GetFirstChild(); q && index < count; q = q->GetNext(), ++index) {
            row_height = max(row_height, MeasureItemHeight(*q, width));
            bool row_end = ((index + 1) % columns) == 0 || index + 1 == count;
            if(row_end) {
                if(total > inset_ * 2)
                    total += gap_y_;
                total += row_height;
                row_height = 0;
            }
        }
        return total;
    }

    void Layout() override
    {
        if(!adapting_ && GetItemCount() > 0) {
            int available = max(1, GetSize().cx - inset_ * 2);
            int columns = ResolveColumns(available);
            int width = ResolveItemWidth(available, columns);

            adapting_ = true;
            PauseLayout();
            UiBoxLayout::SetFixedColumn(0);
            int index = 0;
            for(Ctrl *q = GetFirstChild(); q && index < GetItemCount(); q = q->GetNext(), ++index) {
                int height = MeasureItemHeight(*q, width);
                ItemAt(index).MinMaxMain(width, width)
                             .Fit()
                             .MinMaxCross(height, height)
                             .AlignSelf(UiCrossAlign::Stretch);
            }
            ResumeLayout(false);
            adapting_ = false;
        }
        UiBoxLayout::Layout();
    }

private:
    int ResolveColumns(int available) const
    {
        int count = max(1, GetItemCount());
        int by_width = max(1, (available + gap_x_) / (minimum_column_width_ + gap_x_));
        return min(count, min(max_columns_, by_width));
    }

    int ResolveItemWidth(int available, int columns) const
    {
        columns = max(1, columns);
        int fitted = max(1, (available - gap_x_ * max(0, columns - 1)) / columns);
        return min(preferred_column_width_, fitted);
    }

    int MeasureItemHeight(Ctrl& ctrl, int width)
    {
        Rect old = ctrl.GetRect();
        const int probe_height = DPI(4096);
        ctrl.SetRect(0, 0, max(1, width), probe_height);

        int measured = max(1, ctrl.GetMinSize().cy);
        if(UiGroupPanel *panel = dynamic_cast<UiGroupPanel *>(&ctrl)) {
            panel->Layout();
            Rect body = panel->GetBodyRect();
            if(UiBoxLayout *content = dynamic_cast<UiBoxLayout *>(panel->GetContent())) {
                content->Layout();
                int bottom_chrome = max(0, probe_height - body.bottom);
                measured = max(1, body.top + content->GetContentSize().cy + bottom_chrome);
            }
        }

        ctrl.SetRect(old);
        return measured;
    }

    int gap_x_ = DPI(10);
    int gap_y_ = DPI(10);
    int inset_ = 0;
    int preferred_column_width_ = DPI(350);
    int minimum_column_width_ = DPI(280);
    int max_columns_ = 3;
    bool adapting_ = false;
};

} // namespace Upp

#endif
